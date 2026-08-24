#include "PostProcessPipeline.h"

#include <algorithm>
#include <d3dcompiler.h>

#include "d3dx12.h"
#include "10_Device/DirectX/DirectX12.h"

namespace {
	// 中間ターゲットの解像度倍率(バックバッファの半分. Bloomは低解像度で十分).
	constexpr UINT kTargetSizeDivisor = 2;

	// ブラーのテクセルオフセット倍率(大きいほど滲む. 仮値).
	constexpr float kBlurRadiusScale = 2.0f;
}

bool PostProcessPipeline::Create(DirectX12& Dx12)
{
	m_pDx12 = &Dx12;

	CreateRootSignatureAndPso();

	m_pWhiteTex = MyComPtr<ID3D12Resource>(CreateWhiteTexture());
	if (!m_pWhiteTex) { return false; }

	return true;
}

ID3D12Resource* PostProcessPipeline::CreateWhiteTexture()
{
	constexpr UINT tex_size = 4;

	const auto resource_desc = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_R8G8B8A8_UNORM, tex_size, tex_size);
	const auto heap_prop = CD3DX12_HEAP_PROPERTIES(
		D3D12_CPU_PAGE_PROPERTY_WRITE_BACK, D3D12_MEMORY_POOL_L0);

	ID3D12Resource* buffer = nullptr;
	MyAssert::IsFailed(_T("PostProcess: 白テクスチャの作成"),
		&ID3D12Device::CreateCommittedResource, m_pDx12->GetDevice().Get(),
		&heap_prop, D3D12_HEAP_FLAG_NONE, &resource_desc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, nullptr,
		IID_PPV_ARGS(&buffer));

	std::vector<unsigned char> data(tex_size * tex_size * 4);
	std::fill(data.begin(), data.end(), 0xff);

	MyAssert::IsFailed(_T("PostProcess: テクスチャを白で塗りつぶし"),
		&ID3D12Resource::WriteToSubresource, buffer,
		0, nullptr, static_cast<void*>(data.data()),
		static_cast<UINT>(tex_size) * 4, static_cast<UINT>(data.size()));

	return buffer;
}

void PostProcessPipeline::CreateRootSignatureAndPso()
{
	ID3D12Device* const p_device = m_pDx12->GetDevice().Get();

	// ルートシグネチャ: t0/t1のSRV2枚+RootConstants(b0, float4x2)+静的サンプラー.
	D3D12_DESCRIPTOR_RANGE srv_ranges[2] = {};
	srv_ranges[0].NumDescriptors = 1;
	srv_ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	srv_ranges[0].BaseShaderRegister = 0;
	srv_ranges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	srv_ranges[1] = srv_ranges[0];
	srv_ranges[1].BaseShaderRegister = 1;

	D3D12_ROOT_PARAMETER root_params[3] = {};
	for (int i = 0; i < 2; ++i)
	{
		root_params[i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		root_params[i].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		root_params[i].DescriptorTable.pDescriptorRanges = &srv_ranges[i];
		root_params[i].DescriptorTable.NumDescriptorRanges = 1;
	}

	root_params[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	root_params[2].Constants.ShaderRegister = 0;
	root_params[2].Constants.Num32BitValues = 8; // Param0+Param1(float4 x2).
	root_params[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	D3D12_ROOT_SIGNATURE_DESC root_desc = {};
	root_desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE; // 頂点バッファを使わないためIAレイアウトフラグは不要.
	root_desc.pParameters = root_params;
	root_desc.NumParameters = _countof(root_params);

	CD3DX12_STATIC_SAMPLER_DESC sampler{};
	sampler.Init(0);

	root_desc.pStaticSamplers = &sampler;
	root_desc.NumStaticSamplers = 1;

	MyComPtr<ID3DBlob> signature(nullptr);
	MyComPtr<ID3DBlob> error(nullptr);
	MyAssert::IsFailed(_T("PostProcess: ルートシグネチャをシリアライズ"),
		&D3D12SerializeRootSignature, &root_desc,
		D3D_ROOT_SIGNATURE_VERSION_1, signature.GetAddressOf(), error.GetAddressOf());

	MyAssert::IsFailed(_T("PostProcess: ルートシグネチャの作成"),
		&ID3D12Device::CreateRootSignature, p_device, 0,
		signature->GetBufferPointer(), signature->GetBufferSize(),
		IID_PPV_ARGS(m_pRootSignature.ReleaseAndGetAddressOf()));

	// ===== シェーダーコンパイルとPSO作成 =====
	struct PassShader
	{
		LPCSTR Entry;
		std::wstring HlslPath;
		std::wstring CsoPath;
		MyComPtr<ID3DBlob> Blob;
	};

	PassShader passes[] = {
		{ "PS", L"Data\\Shader\\PostProcess\\BrightExtract_PS.hlsl", L"Data\\Shader\\PostProcess\\BrightExtract_PS.cso", {} },
		{ "PS", L"Data\\Shader\\PostProcess\\GaussianBlur_PS.hlsl",  L"Data\\Shader\\PostProcess\\GaussianBlur_PS.cso",  {} },
		{ "PS", L"Data\\Shader\\PostProcess\\Composite_PS.hlsl",     L"Data\\Shader\\PostProcess\\Composite_PS.cso",     {} },
	};

	MyComPtr<ID3DBlob> vs_blob(nullptr);
#if _DEBUG
	CompileShaderFromFile(L"Data\\Shader\\PostProcess\\Vertex.hlsl", "VS", "vs_5_0", vs_blob.ReleaseAndGetAddressOf());
#else
	LoadCompiledShader(L"Data\\Shader\\PostProcess\\Vertex.cso", vs_blob.ReleaseAndGetAddressOf());
#endif

	for (PassShader& pass : passes)
	{
#if _DEBUG
		CompileShaderFromFile(pass.HlslPath.c_str(), pass.Entry, "ps_5_0", pass.Blob.ReleaseAndGetAddressOf());
#else
		LoadCompiledShader(pass.CsoPath.c_str(), pass.Blob.ReleaseAndGetAddressOf());
#endif
	}

	auto make_pso = [&](ID3DBlob* PixelBlob, MyComPtr<ID3D12PipelineState>& OutState) {
		D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
		desc.pRootSignature = m_pRootSignature.Get();
		desc.VS = CD3DX12_SHADER_BYTECODE(vs_blob.Get());
		desc.PS = CD3DX12_SHADER_BYTECODE(PixelBlob);
		desc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
		desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
		desc.DepthStencilState.DepthEnable = false; // フルスクリーンパスに深度は不要.
		desc.DSVFormat = DXGI_FORMAT_UNKNOWN;
		desc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
		desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		desc.NumRenderTargets = 1;
		desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.SampleDesc.Count = 1;

		MyAssert::IsFailed(_T("PostProcess: パイプラインステートの作成"),
			&ID3D12Device::CreateGraphicsPipelineState, p_device,
			&desc, IID_PPV_ARGS(OutState.ReleaseAndGetAddressOf()));
	};

	make_pso(passes[0].Blob.Get(), m_pBrightExtractPso);
	make_pso(passes[1].Blob.Get(), m_pBlurPso);
	make_pso(passes[2].Blob.Get(), m_pCompositePso);
}

void PostProcessPipeline::CreateIntermediateTargets(UINT Width, UINT Height)
{
	ID3D12Device* const p_device = m_pDx12->GetDevice().Get();

	m_TargetWidth  = Width;
	m_TargetHeight = Height;

	// RTVヒープ(3枚分)を作り直す.
	D3D12_DESCRIPTOR_HEAP_DESC rtv_heap_desc = {};
	rtv_heap_desc.NumDescriptors = 3;
	rtv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	MyAssert::IsFailed(_T("PostProcess: RTVヒープの作成"),
		&ID3D12Device::CreateDescriptorHeap, p_device,
		&rtv_heap_desc, IID_PPV_ARGS(m_pRtvHeap.ReleaseAndGetAddressOf()));

	// SRVヒープ(scene/bright/A/B+白の5個)を作り直す.
	D3D12_DESCRIPTOR_HEAP_DESC srv_heap_desc = {};
	srv_heap_desc.NumDescriptors = 5;
	srv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	MyAssert::IsFailed(_T("PostProcess: SRVヒープの作成"),
		&ID3D12Device::CreateDescriptorHeap, p_device,
		&srv_heap_desc, IID_PPV_ARGS(m_pSrvHeap.ReleaseAndGetAddressOf()));

	const auto default_heap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	const auto tex_desc = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_R8G8B8A8_UNORM, Width, Height, 1, 0, 1, 0,
		D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

	float clear_value[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	const auto clear_props = CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R8G8B8A8_UNORM, clear_value);

	MyComPtr<ID3D12Resource>* targets[3] = { &m_pBrightTexture, &m_pBlurTempA, &m_pBlurTempB };
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(m_pRtvHeap->GetCPUDescriptorHandleForHeapStart());
	const UINT rtv_size = p_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	for (MyComPtr<ID3D12Resource>* target : targets)
	{
		target->Reset();
		MyAssert::IsFailed(_T("PostProcess: 中間ターゲットの作成"),
			&ID3D12Device::CreateCommittedResource, p_device,
			&default_heap, D3D12_HEAP_FLAG_NONE, &tex_desc,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &clear_props,
			IID_PPV_ARGS(target->ReleaseAndGetAddressOf()));

		p_device->CreateRenderTargetView(target->Get(), nullptr, rtv);
		rtv.Offset(rtv_size);
	}
}

void PostProcessPipeline::Apply()
{
	ID3D12GraphicsCommandList* cmd_list = m_pDx12->GetCommandList().Get();
	ID3D12Device* const p_device = m_pDx12->GetDevice().Get();

	// ===== 中間ターゲット整合チェック(リサイズ時はGPU完了待ちの上で作り直す) =====
	DXGI_SWAP_CHAIN_DESC1 swap_desc{};
	MyAssert::IsFailed(_T("PostProcess: スワップチェーン情報の取得"),
		&IDXGISwapChain4::GetDesc1, m_pDx12->GetSwapChain().Get(), &swap_desc);

	const UINT target_w = std::max(1u, swap_desc.Width / kTargetSizeDivisor);
	const UINT target_h = std::max(1u, swap_desc.Height / kTargetSizeDivisor);
	if (target_w != m_TargetWidth || target_h != m_TargetHeight)
	{
		// リサイズはレアイベントのため、GPU完了を待ってから作り直す(数フレームに1度のヒッチは許容).
		m_pDx12->WaitForGPU();
		CreateIntermediateTargets(target_w, target_h);
	}

	ID3D12Resource* const p_scene = m_pDx12->GetSceneColorBuffer();
	if (!p_scene) { return; }

	// ===== シーンカラー→SRV状態へ遷移し、毎フレームSRVを張り直す =====
	{
		const auto to_srv = CD3DX12_RESOURCE_BARRIER::Transition(p_scene,
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		cmd_list->ResourceBarrier(1, &to_srv);
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE srv_cpu(m_pSrvHeap->GetCPUDescriptorHandleForHeapStart());
	const UINT srv_size = p_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	const UINT rtv_size = p_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
	srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srv_desc.ViewDimension           = D3D12_SRV_DIMENSION_TEXTURE2D;
	srv_desc.Texture2D.MipLevels     = 1;

	srv_desc.Format              = p_scene->GetDesc().Format;
	srv_desc.Texture2D.MipLevels = p_scene->GetDesc().MipLevels;
	p_device->CreateShaderResourceView(p_scene, &srv_desc, srv_cpu);

	CD3DX12_CPU_DESCRIPTOR_HANDLE srv_bright(srv_cpu); srv_bright.Offset(1, srv_size);
	CD3DX12_CPU_DESCRIPTOR_HANDLE srv_blur_a(srv_cpu);  srv_blur_a.Offset(2, srv_size);
	CD3DX12_CPU_DESCRIPTOR_HANDLE srv_blur_b(srv_cpu);  srv_blur_b.Offset(3, srv_size);
	CD3DX12_CPU_DESCRIPTOR_HANDLE srv_white(srv_cpu);   srv_white.Offset(4, srv_size);

	// 中間ターゲット(bright/blurA/blurB)のSRV.
	// これを作らないとPass2以降が未初期化ディスクリプタをサンプルしてGPUフォルト→デバイスロストになる.
	MyComPtr<ID3D12Resource>* const intermediates[3] = { &m_pBrightTexture, &m_pBlurTempA, &m_pBlurTempB };
	const CD3DX12_CPU_DESCRIPTOR_HANDLE intermediate_srvs[3] = { srv_bright, srv_blur_a, srv_blur_b };
	for (int i = 0; i < 3; ++i)
	{
		ID3D12Resource* const p_target = intermediates[i]->Get();
		if (!p_target) { continue; }
		srv_desc.Format              = p_target->GetDesc().Format;
		srv_desc.Texture2D.MipLevels = 1; // 中間ターゲットはミップ無しで作成している.
		p_device->CreateShaderResourceView(p_target, &srv_desc, intermediate_srvs[i]);
	}

	// 白テクスチャのSRV(未使用スロットのダミー. 毎フレーム再作成で寿命問題を回避).
	srv_desc.Format              = m_pWhiteTex->GetDesc().Format;
	srv_desc.Texture2D.MipLevels = m_pWhiteTex->GetDesc().MipLevels;
	p_device->CreateShaderResourceView(m_pWhiteTex.Get(), &srv_desc, srv_white);

	// ===== バックバッファをレンダーターゲットへ(ImGui描画前の準備も兼ねる) =====
	{
		const auto to_rt = CD3DX12_RESOURCE_BARRIER::Transition(
			m_pDx12->GetBackBuffer(m_pDx12->GetFrameIndex()),
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		cmd_list->ResourceBarrier(1, &to_rt);
	}

	auto backbuffer_rtv = m_pDx12->GetBackBufferRtvHandle(m_pDx12->GetFrameIndex());

	// ===== 共通ステート =====
	cmd_list->SetGraphicsRootSignature(m_pRootSignature.Get());
	// MyComPtr::GetAddressOf()は中身をReleaseしてnullptrにする実装(標準のComPtrと挙動が違う)ため、
	// 読み取り目的でここへ渡すとSRVヒープが解放されて以降のGetGPUDescriptorHandleForHeapStart()が落ちる.
	// 他の描画クラスと同じく、ローカル配列にGet()した生ポインタを載せて渡す.
	ID3D12DescriptorHeap* pp_heaps[] = { m_pSrvHeap.Get() };
	cmd_list->SetDescriptorHeaps(_countof(pp_heaps), pp_heaps);
	cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	const float texel_w = 1.0f / static_cast<float>(m_TargetWidth);
	const float texel_h = 1.0f / static_cast<float>(m_TargetHeight);

	auto begin_pass = [&](ID3D12PipelineState* Pso, D3D12_CPU_DESCRIPTOR_HANDLE Rtv,
		UINT W, UINT H, D3D12_GPU_DESCRIPTOR_HANDLE T0, D3D12_GPU_DESCRIPTOR_HANDLE T1,
		float Threshold, float Intensity, float DirX, float DirY)
	{
		cmd_list->SetPipelineState(Pso);

		const D3D12_VIEWPORT viewport{ 0.0f, 0.0f, static_cast<float>(W), static_cast<float>(H), 0.0f, 1.0f };
		const D3D12_RECT scissor{ 0, 0, static_cast<LONG>(W), static_cast<LONG>(H) };
		cmd_list->RSSetViewports(1, &viewport);
		cmd_list->RSSetScissorRects(1, &scissor);

		cmd_list->OMSetRenderTargets(1, &Rtv, false, nullptr);
		const float clear_color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
		cmd_list->ClearRenderTargetView(Rtv, clear_color, 0, nullptr);

		D3D12_GPU_DESCRIPTOR_HANDLE tables[2] = { T0, T1 };
		cmd_list->SetGraphicsRootDescriptorTable(0, tables[0]);
		cmd_list->SetGraphicsRootDescriptorTable(1, tables[1]);

		const float params0[4] = { Threshold, Intensity, DirX, DirY };
		const float params1[4] = { texel_w, texel_h, 0.0f, 0.0f };
		cmd_list->SetGraphicsRoot32BitConstants(2, 4, params0, 0);
		cmd_list->SetGraphicsRoot32BitConstants(2, 4, params1, 4);

		cmd_list->DrawInstanced(3, 1, 0, 0);
	};

	// ===== GPU側ディスクリプタ(パス入力) =====
	const D3D12_GPU_DESCRIPTOR_HANDLE gpu_scene(m_pSrvHeap->GetGPUDescriptorHandleForHeapStart());
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpu_bright(gpu_scene); gpu_bright.Offset(1, srv_size);
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpu_blur_a(gpu_scene); gpu_blur_a.Offset(2, srv_size);
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpu_blur_b(gpu_scene); gpu_blur_b.Offset(3, srv_size);
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpu_white(gpu_scene);  gpu_white.Offset(4, srv_size);

	// ===== Pass 1: 輝度抽出(シーン→bright) =====
	{
		// 中間ターゲットはPIXEL_SHADER_RESOURCE状態で保持しているため、書き込む直前にRTへ遷移させる
		// (この遷移が無いままRTVとして描くと、後段の「RENDER_TARGETから戻す」バリアと状態が食い違う).
		const auto to_rt = CD3DX12_RESOURCE_BARRIER::Transition(m_pBrightTexture.Get(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
		cmd_list->ResourceBarrier(1, &to_rt);

		CD3DX12_CPU_DESCRIPTOR_HANDLE bright_rtv(m_pRtvHeap->GetCPUDescriptorHandleForHeapStart());
		begin_pass(m_pBrightExtractPso.Get(), bright_rtv, m_TargetWidth, m_TargetHeight,
			gpu_scene, gpu_white, m_Threshold, 0.0f, 0.0f, 0.0f);
	}

	// 次のパスがbrightをサンプルするため、書き終えた直後にSRVへ戻す
	// (書き込み対象のまま読むと同一リソースがRTVとSRVで同時バインドされ、GPUフォルトになる).
	{
		const auto to_srv = CD3DX12_RESOURCE_BARRIER::Transition(m_pBrightTexture.Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		cmd_list->ResourceBarrier(1, &to_srv);
	}

	// ===== Pass 2: ガウシアンブラー水平(bright→blurA) =====
	{
		const auto to_rt = CD3DX12_RESOURCE_BARRIER::Transition(m_pBlurTempA.Get(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
		cmd_list->ResourceBarrier(1, &to_rt);

		CD3DX12_CPU_DESCRIPTOR_HANDLE a_rtv(m_pRtvHeap->GetCPUDescriptorHandleForHeapStart());
		a_rtv.Offset(rtv_size);
		begin_pass(m_pBlurPso.Get(), a_rtv, m_TargetWidth, m_TargetHeight,
			gpu_bright, gpu_white, 1.0f * kBlurRadiusScale, 0.0f, 1.0f, 0.0f);
	}

	// Pass3がblurAをサンプルするためSRVへ戻す.
	{
		const auto to_srv = CD3DX12_RESOURCE_BARRIER::Transition(m_pBlurTempA.Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		cmd_list->ResourceBarrier(1, &to_srv);
	}

	// ===== Pass 3: ガウシアンブラー垂直(blurA→blurB) =====
	{
		const auto to_rt = CD3DX12_RESOURCE_BARRIER::Transition(m_pBlurTempB.Get(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
		cmd_list->ResourceBarrier(1, &to_rt);

		CD3DX12_CPU_DESCRIPTOR_HANDLE b_rtv(m_pRtvHeap->GetCPUDescriptorHandleForHeapStart());
		b_rtv.Offset(rtv_size * 2);
		begin_pass(m_pBlurPso.Get(), b_rtv, m_TargetWidth, m_TargetHeight,
			gpu_blur_a, gpu_white, 0.0f, 1.0f * kBlurRadiusScale, 0.0f, 1.0f);
	}

	// Pass4がblurBをサンプルするためSRVへ戻す(これで3枚とも次フレーム開始時の状態に揃う).
	{
		const auto to_srv = CD3DX12_RESOURCE_BARRIER::Transition(m_pBlurTempB.Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		cmd_list->ResourceBarrier(1, &to_srv);
	}

	// ===== Pass 4: 合成(シーン+Bloom→バックバッファ) =====
	{
		begin_pass(m_pCompositePso.Get(), backbuffer_rtv,
			swap_desc.Width, swap_desc.Height,
			gpu_scene, gpu_blur_b,
			0.0f, m_Intensity, 0.0f, 0.0f);
	}

	// メインパスのビューポート・シザーへ復帰させる(続くImGui描画用).
	if (const D3D12_VIEWPORT* main_viewport = m_pDx12->GetMainViewport())
	{
		cmd_list->RSSetViewports(1, main_viewport);
	}
	if (const D3D12_RECT* main_scissor = m_pDx12->GetMainScissorRect())
	{
		cmd_list->RSSetScissorRects(1, main_scissor);
	}
}

HRESULT PostProcessPipeline::CompileShaderFromFile(const std::wstring& FilePath, LPCSTR EntryPoint, LPCSTR Target, ID3DBlob** ShaderBlob)
{
	ID3DBlob* error_blob = nullptr;
	HRESULT result = D3DCompileFromFile(
		FilePath.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		EntryPoint, Target, D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0,
		ShaderBlob, &error_blob);

	MyAssert::ErrorBlob(result, error_blob);

	return result;
}

HRESULT PostProcessPipeline::LoadCompiledShader(const std::wstring& FilePath, ID3DBlob** ShaderBlob)
{
	MyAssert::IsFailed(_T("PostProcess: コンパイル済みシェーダー(.cso)の読み込み"),
		D3DReadFileToBlob, FilePath.c_str(), ShaderBlob);

	return S_OK;
}
