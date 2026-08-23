#include "FrameRewind.h"

#include <d3dcompiler.h>

#include "10_Ggraphic/10_Device/DirectX/DirectX12.h"

// 巻き戻り演出用シェーダー(キャプチャ縮小コピーと逆再生表示で共用).
namespace {

	constexpr TCHAR REWIND_SHADER_PATH[] = _T("Data\\Shader\\Rewind\\Rewind.hlsl");
	constexpr TCHAR REWIND_VS_CSO[] = _T("Data\\Shader\\Rewind\\Rewind_VS.cso");
	constexpr TCHAR REWIND_PS_CSO[] = _T("Data\\Shader\\Rewind\\Rewind_PS.cso");

}

FrameRewind::FrameRewind(DirectX12& Dx12)
	: m_Dx12(Dx12)
{
}

void FrameRewind::Create()
{
	// ---- リングテクスチャ(R8G8B8A8_UNORM固定の縮小解像度) ----
	m_Ring.resize(REWIND_FRAME_COUNT);
	for (UINT i = 0; i < REWIND_FRAME_COUNT; ++i)
	{
		CreateRingTexture(i);
	}

	// ---- RTVヒープ(リング1枚につき1ビュー) ----
	D3D12_DESCRIPTOR_HEAP_DESC rtv_heap_desc = {};
	rtv_heap_desc.NumDescriptors = REWIND_FRAME_COUNT;
	rtv_heap_desc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	MyAssert::IsFailed(
		_T("巻き戻りRTVヒープの作成"),
		&ID3D12Device::CreateDescriptorHeap, m_Dx12.GetDevice(),
		&rtv_heap_desc, IID_PPV_ARGS(m_pRtvHeap.ReleaseAndGetAddressOf()));

	D3D12_RENDER_TARGET_VIEW_DESC rtv_desc = {};
	rtv_desc.Format        = DXGI_FORMAT_R8G8B8A8_UNORM;
	rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	auto rtv_cpu = m_pRtvHeap->GetCPUDescriptorHandleForHeapStart();
	for (UINT i = 0; i < REWIND_FRAME_COUNT; ++i)
	{
		m_Dx12.GetDevice()->CreateRenderTargetView(m_Ring[i].Get(), &rtv_desc, rtv_cpu);
		rtv_cpu.ptr += m_Dx12.GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

	// ---- SRVヒープ(先頭2個=バックバッファ, 続くN個=リング) ----
	D3D12_DESCRIPTOR_HEAP_DESC srv_heap_desc = {};
	srv_heap_desc.NumDescriptors = 2 + REWIND_FRAME_COUNT; // バックバッファ数はスワップチェーンと同一.
	srv_heap_desc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srv_heap_desc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	MyAssert::IsFailed(
		_T("巻き戻りSRVヒープの作成"),
		&ID3D12Device::CreateDescriptorHeap, m_Dx12.GetDevice(),
		&srv_heap_desc, IID_PPV_ARGS(m_pSrvHeap.ReleaseAndGetAddressOf()));

	m_SrvDescriptorSize = m_Dx12.GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	auto srv_cpu = m_pSrvHeap->GetCPUDescriptorHandleForHeapStart();
	srv_cpu.ptr += static_cast<UINT64>(m_SrvDescriptorSize) * 2; // リング領域の先頭(バックバッファ2枚分の後ろ)へ.

	for (UINT i = 0; i < REWIND_FRAME_COUNT; ++i)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC ring_srv_desc = {};
		ring_srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		ring_srv_desc.ViewDimension           = D3D12_SRV_DIMENSION_TEXTURE2D;
		ring_srv_desc.Texture2D.MipLevels     = 1;
		m_Dx12.GetDevice()->CreateShaderResourceView(m_Ring[i].Get(), &ring_srv_desc, srv_cpu);
		srv_cpu.ptr += m_SrvDescriptorSize;
	}

	CreatePipeline();
	RefreshBackBufferSRVs();
}

void FrameRewind::CreateRingTexture(UINT Index)
{
	D3D12_HEAP_PROPERTIES default_heap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	D3D12_RESOURCE_DESC ring_desc = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_R8G8B8A8_UNORM, REWIND_WIDTH, REWIND_HEIGHT);

	MyAssert::IsFailed(
		_T("巻き戻りリングテクスチャの作成"),
		&ID3D12Device::CreateCommittedResource, m_Dx12.GetDevice(),
		&default_heap, D3D12_HEAP_FLAG_NONE, &ring_desc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, nullptr,
		IID_PPV_ARGS(m_Ring[Index].ReleaseAndGetAddressOf()));
}

void FrameRewind::CreatePipeline()
{
	// ---- ルートシグネチャ(t0+スタティックサンプラーs0のみ) ----
	CD3DX12_DESCRIPTOR_RANGE srv_range = {};
	srv_range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

	CD3DX12_ROOT_PARAMETER root_param = {};
	root_param.InitAsDescriptorTable(1, &srv_range, D3D12_SHADER_VISIBILITY_PIXEL);

	CD3DX12_STATIC_SAMPLER_DESC sampler = {};
	sampler.Init(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

	CD3DX12_ROOT_SIGNATURE_DESC rootsig_desc = {};
	rootsig_desc.Init(1, &root_param, 1, &sampler,
		D3D12_ROOT_SIGNATURE_FLAG_NONE); // 頂点バッファ無しのためIALフラグは不要.

	MyComPtr<ID3DBlob> rootsig_blob(nullptr);
	MyComPtr<ID3DBlob> rootsig_error(nullptr);
	MyAssert::IsFailed(
		_T("巻き戻りルートシグネチャのシリアライズ"),
		&D3D12SerializeRootSignature,
		&rootsig_desc, D3D_ROOT_SIGNATURE_VERSION_1,
		rootsig_blob.GetAddressOf(), rootsig_error.GetAddressOf());

	MyAssert::IsFailed(
		_T("巻き戻りルートシグネチャの作成"),
		&ID3D12Device::CreateRootSignature, m_Dx12.GetDevice(),
		0, rootsig_blob->GetBufferPointer(), rootsig_blob->GetBufferSize(),
		IID_PPV_ARGS(m_pRootSignature.ReleaseAndGetAddressOf()));

	// ---- パイプライン(VS/PSともData\Shader\Rewind\Rewind.hlsl) ----
	MyComPtr<ID3DBlob> vs_blob(nullptr);
	MyComPtr<ID3DBlob> ps_blob(nullptr);
#if _DEBUG
	MyAssert::IsFailed(
		_T("巻き戻り頂点シェーダーのコンパイル"),
		&D3DCompileFromFile,
		REWIND_SHADER_PATH, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"VS", "vs_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0,
		vs_blob.GetAddressOf(), nullptr);
	MyAssert::IsFailed(
		_T("巻き戻りピクセルシェーダーのコンパイル"),
		&D3DCompileFromFile,
		REWIND_SHADER_PATH, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"PS", "ps_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0,
		ps_blob.GetAddressOf(), nullptr);
#else
	MyAssert::IsFailed(_T("巻き戻り頂点シェーダー(.cso)の読み込み"), D3DReadFileToBlob, REWIND_VS_CSO, vs_blob.GetAddressOf());
	MyAssert::IsFailed(_T("巻き戻りピクセルシェーダー(.cso)の読み込み"), D3DReadFileToBlob, REWIND_PS_CSO, ps_blob.GetAddressOf());
#endif

	D3D12_GRAPHICS_PIPELINE_STATE_DESC pipe_desc = {};
	pipe_desc.pRootSignature      = m_pRootSignature.Get();
	pipe_desc.VS                  = CD3DX12_SHADER_BYTECODE(vs_blob.Get());
	pipe_desc.PS                  = CD3DX12_SHADER_BYTECODE(ps_blob.Get());
	pipe_desc.BlendState          = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	pipe_desc.RasterizerState     = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	pipe_desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE; // ワインディングを気にしない1枚三角形.
	pipe_desc.DepthStencilState.DepthEnable = false;
	pipe_desc.SampleMask          = D3D12_DEFAULT_SAMPLE_MASK;
	pipe_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipe_desc.NumRenderTargets    = 1;
	pipe_desc.RTVFormats[0]       = DXGI_FORMAT_R8G8B8A8_UNORM;
	pipe_desc.SampleDesc.Count    = 1;

	MyAssert::IsFailed(
		_T("巻き戻りパイプラインの作成"),
		&ID3D12Device::CreateGraphicsPipelineState, m_Dx12.GetDevice(),
		&pipe_desc, IID_PPV_ARGS(m_pPipelineState.ReleaseAndGetAddressOf()));
}

void FrameRewind::RefreshBackBufferSRVs()
{
	if (!m_pSrvHeap.Get()) { return; }

	auto srv_cpu = m_pSrvHeap->GetCPUDescriptorHandleForHeapStart();

	D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
	srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srv_desc.ViewDimension           = D3D12_SRV_DIMENSION_TEXTURE2D;

	for (UINT i = 0; i < 2; ++i)
	{
		ID3D12Resource* p_backbuffer = m_Dx12.GetBackBuffer(i);
		if (!p_backbuffer) { continue; }
		srv_desc.Format              = p_backbuffer->GetDesc().Format;
		srv_desc.Texture2D.MipLevels = p_backbuffer->GetDesc().MipLevels;
		m_Dx12.GetDevice()->CreateShaderResourceView(p_backbuffer, &srv_desc, srv_cpu);
		srv_cpu.ptr += m_SrvDescriptorSize;
	}
}

void FrameRewind::Capture()
{
	// 逆再生中は保存しない(再生中のフレームが履歴に混入するのを防ぐ).
	if (m_State != RewindState::Capture || !m_pPipelineState.Get()) { return; }

	ID3D12GraphicsCommandList* cmd_list = m_Dx12.GetCommandList().Get();
	// キャプチャ対象は「現在のシーン描画先」(ポストプロセス有効時はオフスクリーンのシーンカラー).
	ID3D12Resource* p_backbuffer = m_Dx12.GetCurrentSceneTarget();
	if (!p_backbuffer) { return; }

	// バックバッファをサンプリング可能状態へ一時遷移させる.
	auto to_srv = CD3DX12_RESOURCE_BARRIER::Transition(p_backbuffer,
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	cmd_list->ResourceBarrier(1, &to_srv);

	cmd_list->SetPipelineState(m_pPipelineState.Get());
	cmd_list->SetGraphicsRootSignature(m_pRootSignature.Get());

	ID3D12DescriptorHeap* pp_heaps[] = { m_pSrvHeap.Get() };
	cmd_list->SetDescriptorHeaps(_countof(pp_heaps), pp_heaps);
	cmd_list->SetGraphicsRootDescriptorTable(0, m_pSrvHeap->GetGPUDescriptorHandleForHeapStart());

	D3D12_VIEWPORT viewport{ 0.0f, 0.0f, static_cast<float>(REWIND_WIDTH), static_cast<float>(REWIND_HEIGHT), 0.0f, 1.0f };
	D3D12_RECT scissor{ 0, 0, static_cast<LONG>(REWIND_WIDTH), static_cast<LONG>(REWIND_HEIGHT) };
	cmd_list->RSSetViewports(1, &viewport);
	cmd_list->RSSetScissorRects(1, &scissor);

	auto ring_rtv = m_pRtvHeap->GetCPUDescriptorHandleForHeapStart();
	ring_rtv.ptr += static_cast<UINT64>(m_WriteIndex) *
		m_Dx12.GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	cmd_list->OMSetRenderTargets(1, &ring_rtv, false, nullptr);

	cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmd_list->DrawInstanced(3, 1, 0, 0); // SV_VertexIDによるフルスクリーントライアングル.

	// メインパス続行のためバックバッファをレンダーターゲットへ戻す.
	auto to_rtv = CD3DX12_RESOURCE_BARRIER::Transition(p_backbuffer,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
	cmd_list->ResourceBarrier(1, &to_rtv);

	// 通常描画用のビューポート・シザー・OMを復帰させる.
	m_Dx12.RestoreMainRenderTargets();

	m_WriteIndex = (m_WriteIndex + 1) % REWIND_FRAME_COUNT;
	m_ValidCount = (m_ValidCount < REWIND_FRAME_COUNT) ? (m_ValidCount + 1) : REWIND_FRAME_COUNT;
}

bool FrameRewind::StartPlayback()
{
	if (m_ValidCount == 0 || !m_pPipelineState.Get()) { return false; }

	m_State       = RewindState::Playing;
	m_Finished    = false;
	m_PlayIndex   = (m_WriteIndex + REWIND_FRAME_COUNT - 1) % REWIND_FRAME_COUNT; // 最新フレームから.
	m_FramesShown = 0;

	return true;
}

void FrameRewind::DrawPlaybackFrame()
{
	if (!IsActive() || !m_pPipelineState.Get()) { return; }

	ID3D12GraphicsCommandList* cmd_list = m_Dx12.GetCommandList().Get();

	cmd_list->SetPipelineState(m_pPipelineState.Get());
	cmd_list->SetGraphicsRootSignature(m_pRootSignature.Get());

	ID3D12DescriptorHeap* pp_heaps[] = { m_pSrvHeap.Get() };
	cmd_list->SetDescriptorHeaps(_countof(pp_heaps), pp_heaps);

	// SRVスロット = 先頭2枚(バックバッファ)の後ろにあるリング領域.
	D3D12_GPU_DESCRIPTOR_HANDLE srv_gpu = m_pSrvHeap->GetGPUDescriptorHandleForHeapStart();
	srv_gpu.ptr += static_cast<UINT64>(2 + m_PlayIndex) * m_SrvDescriptorSize;
	cmd_list->SetGraphicsRootDescriptorTable(0, srv_gpu);

	// OM/ビューポートはBeginDraw()が設定した現在のレンダーターゲット(バックバッファ)を使う.
	cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmd_list->DrawInstanced(3, 1, 0, 0);

	// 古い方向へ再生位置を進める(2倍速. カクつきは演出として許容).
	m_PlayIndex   = (m_PlayIndex + REWIND_FRAME_COUNT - REWIND_PLAYBACK_SPEED) % REWIND_FRAME_COUNT;
	m_FramesShown += REWIND_PLAYBACK_SPEED;

	if (m_FramesShown >= m_ValidCount)
	{
		// 最古フレームまで到達. 保存を再開し、次フレームから通常フロー(LOSE表示)へ復帰する.
		m_Finished = true;
		m_State    = RewindState::Capture;
	}
}
