#include "SpriteRenderer.h"

#include <cstring>
#include <d3dcompiler.h>

#include "10_Ggraphic/10_Device/DirectX/DirectX12.h"

namespace {

	// Sprite2D用頂点(NDC座標をCPU側で計算して渡す).
	struct Sprite2DVertex
	{
		DirectX::XMFLOAT2 Pos;
		DirectX::XMFLOAT2 Uv;
		DirectX::XMFLOAT4 Color;
	};

	// Sprite3D用頂点(展開は頂点シェーダー側で行う).
	struct Sprite3DVertex
	{
		DirectX::XMFLOAT3 Center;
		DirectX::XMFLOAT2 Corner; // -0.5..0.5.
		DirectX::XMFLOAT2 Size;
		DirectX::XMFLOAT2 Uv;
		DirectX::XMFLOAT4 Color;
	};

	constexpr UINT kSceneCbvAlignedSize = 512; // SceneBuffer(b0)用CBVサイズ(アライン済み).

} // namespace

SpriteRenderer::SpriteRenderer(DirectX12& Dx12)
	: m_Dx12(Dx12)
{
	CreateCommonResources();
	CreatePipeline(false);
	CreatePipeline(true);

	for (UINT f = 0; f < 2; ++f)
	{
		const UINT vb_size = MAX_SPRITES * 4 * sizeof(Sprite2DVertex);

		D3D12_HEAP_PROPERTIES upload_heap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		D3D12_RESOURCE_DESC buf_desc = CD3DX12_RESOURCE_DESC::Buffer(vb_size);

		auto create_buffer = [&](MyComPtr<ID3D12Resource>& Out, void** ppMapped) {
			MyAssert::IsFailed(
				_T("スプライト頂点バッファの作成"),
				&ID3D12Device::CreateCommittedResource, m_Dx12.GetDevice(),
				&upload_heap, D3D12_HEAP_FLAG_NONE, &buf_desc,
				D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
				IID_PPV_ARGS(Out.ReleaseAndGetAddressOf()));
			MyAssert::IsFailed(
				_T("スプライト頂点バッファのマップ"),
				&ID3D12Resource::Map, Out.Get(),
				0, nullptr, ppMapped);
		};

		create_buffer(m_Buffer2D[f].pVertexBuffer, &m_Buffer2D[f].pMapped);
		m_Buffer2D[f].View.BufferLocation = m_Buffer2D[f].pVertexBuffer->GetGPUVirtualAddress();
		m_Buffer2D[f].View.StrideInBytes  = sizeof(Sprite2DVertex);
		m_Buffer2D[f].View.SizeInBytes    = vb_size;

		const UINT vb_size_3d = MAX_SPRITES * 4 * sizeof(Sprite3DVertex);
		buf_desc = CD3DX12_RESOURCE_DESC::Buffer(vb_size_3d);
		create_buffer(m_Buffer3D[f].pVertexBuffer, &m_Buffer3D[f].pMapped);
		m_Buffer3D[f].View.BufferLocation = m_Buffer3D[f].pVertexBuffer->GetGPUVirtualAddress();
		m_Buffer3D[f].View.StrideInBytes  = sizeof(Sprite3DVertex);
		m_Buffer3D[f].View.SizeInBytes    = vb_size_3d;
	}

	// 四角形共用インデックス(頂点順: 左上→右上→右下→左下).
	UINT indices[MAX_SPRITES * 6];
	for (UINT i = 0; i < MAX_SPRITES; ++i)
	{
		const UINT v = i * 4;
		indices[i * 6 + 0] = v + 0;
		indices[i * 6 + 1] = v + 1;
		indices[i * 6 + 2] = v + 2;
		indices[i * 6 + 3] = v + 0;
		indices[i * 6 + 4] = v + 2;
		indices[i * 6 + 5] = v + 3;
	}

	const UINT ib_size = sizeof(indices);
	D3D12_RESOURCE_DESC ib_desc = CD3DX12_RESOURCE_DESC::Buffer(ib_size);
	MyComPtr<ID3D12Resource> p_ib_upload;
	D3D12_HEAP_PROPERTIES upload_heap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD); // ループ外スコープのため再宣言.
	MyAssert::IsFailed(
		_T("スプライトインデックスバッファの作成"),
		&ID3D12Device::CreateCommittedResource, m_Dx12.GetDevice(),
		&upload_heap, D3D12_HEAP_FLAG_NONE, &ib_desc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(p_ib_upload.ReleaseAndGetAddressOf()));

	UINT* p_mapped_indices = nullptr;
	MyAssert::IsFailed(
		_T("スプライトインデックスバッファのマップ"),
		&ID3D12Resource::Map, p_ib_upload.Get(),
		0, nullptr, reinterpret_cast<void**>(&p_mapped_indices));
	std::memcpy(p_mapped_indices, indices, ib_size);
	p_ib_upload->Unmap(0, nullptr);

	m_IndexBufferView.BufferLocation = p_ib_upload->GetGPUVirtualAddress();
	m_IndexBufferView.SizeInBytes    = ib_size;
	m_IndexBufferView.Format         = DXGI_FORMAT_R32_UINT;

	// Viewだけ残すとローカルのアップロードバッファが解放されGPU VAが無効化されるため、本体をメンバで保持する.
	m_pIndexBuffer = std::move(p_ib_upload);
}

SpriteRenderer::~SpriteRenderer()
{
	for (UINT f = 0; f < 2; ++f)
	{
		if (m_Buffer2D[f].pMapped) { m_Buffer2D[f].pVertexBuffer->Unmap(0, nullptr); }
		if (m_Buffer3D[f].pMapped) { m_Buffer3D[f].pVertexBuffer->Unmap(0, nullptr); }
	}
}

// 共通リソース(ルートシグネチャ・ヒープ・インデックスバッファ)の生成.
void SpriteRenderer::CreateCommonResources()
{
	// ルートシグネチャ: t0(テクスチャSRV) + b0(SceneBuffer CBV. 3Dのビルボード展開で使用).
	CD3DX12_DESCRIPTOR_RANGE srv_range = {};
	srv_range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

	CD3DX12_DESCRIPTOR_RANGE cbv_range = {};
	cbv_range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);

	CD3DX12_ROOT_PARAMETER params[2] = {};
	params[0].InitAsDescriptorTable(1, &srv_range, D3D12_SHADER_VISIBILITY_PIXEL);
	params[1].InitAsDescriptorTable(1, &cbv_range, D3D12_SHADER_VISIBILITY_VERTEX);

	CD3DX12_STATIC_SAMPLER_DESC sampler = {};
	sampler.Init(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

	CD3DX12_ROOT_SIGNATURE_DESC rootsig_desc = {};
	rootsig_desc.Init(_countof(params), params, 1, &sampler,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	MyComPtr<ID3DBlob> rootsig_blob(nullptr);
	MyComPtr<ID3DBlob> rootsig_error(nullptr);
	MyAssert::IsFailed(
		_T("スプライトルートシグネチャのシリアライズ"),
		&D3D12SerializeRootSignature,
		&rootsig_desc, D3D_ROOT_SIGNATURE_VERSION_1,
		rootsig_blob.GetAddressOf(), rootsig_error.GetAddressOf());

	MyAssert::IsFailed(
		_T("スプライトルートシグネチャの作成"),
		&ID3D12Device::CreateRootSignature, m_Dx12.GetDevice(),
		0, rootsig_blob->GetBufferPointer(), rootsig_blob->GetBufferSize(),
		IID_PPV_ARGS(m_pRootSignature.ReleaseAndGetAddressOf()));

	// SRVヒープ(先頭=SceneBuffer CBV, 以降=テクスチャSRV).
	D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
	heap_desc.NumDescriptors = 33;
	heap_desc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heap_desc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	MyAssert::IsFailed(
		_T("スプライトSRVヒープの作成"),
		&ID3D12Device::CreateDescriptorHeap, m_Dx12.GetDevice(),
		&heap_desc, IID_PPV_ARGS(m_pSrvHeap.ReleaseAndGetAddressOf()));

	m_SrvDescriptorSize = m_Dx12.GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// 先頭スロットへScene定数バッファのCBVを作成(3Dビルボードのview/proj用).
	if (ID3D12Resource* p_scene_cb = m_Dx12.GetSceneConstantBuffer())
	{
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc = {};
		cbv_desc.BufferLocation = p_scene_cb->GetGPUVirtualAddress();
		cbv_desc.SizeInBytes    = kSceneCbvAlignedSize;
		m_Dx12.GetDevice()->CreateConstantBufferView(&cbv_desc,
			m_pSrvHeap->GetCPUDescriptorHandleForHeapStart());
	}
}

// 種別ごとのパイプラインと頂点バッファを生成する.
void SpriteRenderer::CreatePipeline(bool Is3D)
{
	MyComPtr<ID3DBlob> vs_blob(nullptr);
	MyComPtr<ID3DBlob> ps_blob(nullptr);
#if _DEBUG
	const wchar_t* p_shader_path = Is3D ? L"Data\\Shader\\Sprite\\Sprite3D.hlsl" : L"Data\\Shader\\Sprite\\Sprite2D.hlsl";
	MyAssert::IsFailed(
		_T("スプライト頂点シェーダーのコンパイル"),
		&D3DCompileFromFile,
		p_shader_path, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"VS", "vs_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0,
		vs_blob.GetAddressOf(), nullptr);
	MyAssert::IsFailed(
		_T("スプライトピクセルシェーダーのコンパイル"),
		&D3DCompileFromFile,
		p_shader_path, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"PS", "ps_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0,
		ps_blob.GetAddressOf(), nullptr);
#else
	const wchar_t* p_vs_cso = Is3D ? L"Data\\Shader\\Sprite\\Sprite3D_VS.cso" : L"Data\\Shader\\Sprite\\Sprite2D_VS.cso";
	const wchar_t* p_ps_cso = Is3D ? L"Data\\Shader\\Sprite\\Sprite3D_PS.cso" : L"Data\\Shader\\Sprite\\Sprite2D_PS.cso";
	MyAssert::IsFailed(_T("スプライトVS(.cso)読込"), D3DReadFileToBlob, p_vs_cso, vs_blob.GetAddressOf());
	MyAssert::IsFailed(_T("スプライトPS(.cso)読込"), D3DReadFileToBlob, p_ps_cso, ps_blob.GetAddressOf());
#endif

	// 頂点レイアウト.
	D3D12_INPUT_ELEMENT_DESC layout[5];
	UINT element_count = 0;

	if (Is3D)
	{
		layout[0] = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
		layout[1] = { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
		layout[2] = { "TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
		layout[3] = { "TEXCOORD", 2, DXGI_FORMAT_R32G32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
		layout[4] = { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
		element_count = 5;
	}
	else
	{
		layout[0] = { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
		layout[1] = { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
		layout[2] = { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
		element_count = 3;
	}

	D3D12_GRAPHICS_PIPELINE_STATE_DESC pipe_desc = {};
	pipe_desc.pRootSignature      = m_pRootSignature.Get();
	pipe_desc.VS                  = CD3DX12_SHADER_BYTECODE(vs_blob.Get());
	pipe_desc.PS                  = CD3DX12_SHADER_BYTECODE(ps_blob.Get());
	pipe_desc.BlendState          = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	pipe_desc.BlendState.RenderTarget[0].BlendEnable    = TRUE;
	pipe_desc.BlendState.RenderTarget[0].SrcBlend       = D3D12_BLEND_SRC_ALPHA;
	pipe_desc.BlendState.RenderTarget[0].DestBlend      = D3D12_BLEND_INV_SRC_ALPHA;
	pipe_desc.BlendState.RenderTarget[0].BlendOp        = D3D12_BLEND_OP_ADD;
	pipe_desc.BlendState.RenderTarget[0].SrcBlendAlpha  = D3D12_BLEND_ONE;
	pipe_desc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
	pipe_desc.SampleMask          = D3D12_DEFAULT_SAMPLE_MASK;
	pipe_desc.RasterizerState     = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	pipe_desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	pipe_desc.DepthStencilState.DepthEnable   = Is3D; // UI(2D)は深度無視. ビルボードは深度テストのみ.
	pipe_desc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	pipe_desc.DepthStencilState.DepthFunc     = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	pipe_desc.DSVFormat            = DXGI_FORMAT_D32_FLOAT;
	pipe_desc.InputLayout.pInputElementDescs = layout;
	pipe_desc.InputLayout.NumElements        = element_count;
	pipe_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipe_desc.NumRenderTargets = 1;
	pipe_desc.RTVFormats[0]    = DXGI_FORMAT_R8G8B8A8_UNORM;
	pipe_desc.SampleDesc.Count = 1;

	MyComPtr<ID3D12PipelineState>& p_pso = Is3D ? m_pPipeline3D : m_pPipeline2D;
	MyAssert::IsFailed(
		_T("スプライトパイプラインの作成"),
		&ID3D12Device::CreateGraphicsPipelineState, m_Dx12.GetDevice(),
		&pipe_desc, IID_PPV_ARGS(p_pso.ReleaseAndGetAddressOf()));
}

// テクスチャをSRVヒープへ登録し、GPUハンドルを返す.
D3D12_GPU_DESCRIPTOR_HANDLE SpriteRenderer::RegisterTexture(ID3D12Resource* pTexture)
{
	const auto it = m_TextureTable.find(pTexture);
	if (it != m_TextureTable.end()) { return it->second; }

	auto cpu_handle = m_pSrvHeap->GetCPUDescriptorHandleForHeapStart();
	cpu_handle.ptr += static_cast<UINT64>(m_NextSrvSlot) * m_SrvDescriptorSize;

	D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
	srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srv_desc.ViewDimension           = D3D12_SRV_DIMENSION_TEXTURE2D;
	srv_desc.Texture2D.MipLevels     = pTexture->GetDesc().MipLevels;
	m_Dx12.GetDevice()->CreateShaderResourceView(pTexture, &srv_desc, cpu_handle);

	D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle = m_pSrvHeap->GetGPUDescriptorHandleForHeapStart();
	gpu_handle.ptr += static_cast<UINT64>(m_NextSrvSlot) * m_SrvDescriptorSize;
	++m_NextSrvSlot;

	return m_TextureTable.emplace(pTexture, gpu_handle).first->second;
}

// 種別共通の1枚分の描画コマンドを積む.
void SpriteRenderer::DrawQuad(bool Is3D, const void* pVertices, UINT VertexSize, ID3D12Resource* pTexture)
{
	if (!pTexture || !pVertices) { return; }

	const UINT frame = m_Dx12.GetFrameIndex();
	SpriteBuffer& buffer = Is3D ? m_Buffer3D[frame] : m_Buffer2D[frame];

	static UINT s_Count2D[2] = {};
	static UINT s_Count3D[2] = {};
	UINT& count = Is3D ? s_Count3D[frame] : s_Count2D[frame];
	static UINT s_LastFrame[2] = { UINT_MAX, UINT_MAX };
	UINT& last_frame = Is3D ? s_LastFrame[1] : s_LastFrame[0];
	if (last_frame != frame) { count = 0; last_frame = frame; } // フレームが変わったら書き込み位置を先頭へ戻す.

	if (count >= MAX_SPRITES) { return; } // 上限到達分は破棄(デモ用途).

	// 頂点4つ分をフレーム用バッファへ書き込む.
	BYTE* p_dst = static_cast<BYTE*>(buffer.pMapped) + static_cast<UINT64>(count) * 4 * VertexSize;
	std::memcpy(p_dst, pVertices, 4 * VertexSize);

	ID3D12GraphicsCommandList* cmd_list = m_Dx12.GetCommandList().Get();

	cmd_list->SetPipelineState((Is3D ? m_pPipeline3D : m_pPipeline2D).Get());
	cmd_list->SetGraphicsRootSignature(m_pRootSignature.Get());

	ID3D12DescriptorHeap* pp_heaps[] = { m_pSrvHeap.Get() };
	cmd_list->SetDescriptorHeaps(_countof(pp_heaps), pp_heaps);

	D3D12_GPU_DESCRIPTOR_HANDLE srv_gpu = m_pSrvHeap->GetGPUDescriptorHandleForHeapStart();
	srv_gpu.ptr += static_cast<UINT64>(RegisterTexture(pTexture).ptr -
		m_pSrvHeap->GetGPUDescriptorHandleForHeapStart().ptr);
	cmd_list->SetGraphicsRootDescriptorTable(0, srv_gpu);

	// b0(SceneBuffer)は先頭スロットのCBV.
	D3D12_GPU_DESCRIPTOR_HANDLE cbv_gpu = m_pSrvHeap->GetGPUDescriptorHandleForHeapStart();
	cmd_list->SetGraphicsRootDescriptorTable(1, cbv_gpu);

	D3D12_VERTEX_BUFFER_VIEW vb_view = buffer.View;
	vb_view.BufferLocation += static_cast<UINT64>(count) * 4 * VertexSize;
	vb_view.SizeInBytes     = 4 * VertexSize;
	cmd_list->IASetVertexBuffers(0, 1, &vb_view);
	cmd_list->IASetIndexBuffer(&m_IndexBufferView);
	cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	cmd_list->DrawIndexedInstanced(6, 1, count * 6, 0, 0);
	++count;
}

// 画面座標(ピクセル)にテクスチャ矩形を描画する.
void SpriteRenderer::DrawSprite2D(
	ID3D12Resource* pTexture,
	float PosX, float PosY, float Width, float Height,
	const DirectX::XMFLOAT4& Color)
{
	const float screen_w = static_cast<float>(m_Dx12.GetBackBufferWidth());
	const float screen_h = static_cast<float>(m_Dx12.GetBackBufferHeight());
	if (screen_w <= 0.0f || screen_h <= 0.0f) { return; }

	const auto to_ndc_x = [screen_w](float x) { return (x / screen_w) * 2.0f - 1.0f; };
	const auto to_ndc_y = [screen_h](float y) { return 1.0f - (y / screen_h) * 2.0f; }; // 上原点.

	const float x0 = to_ndc_x(PosX);
	const float y0 = to_ndc_y(PosY);
	const float x1 = to_ndc_x(PosX + Width);
	const float y1 = to_ndc_y(PosY + Height);

	Sprite2DVertex verts[4] = {
		{ { x0, y0 }, { 0.0f, 0.0f }, Color },
		{ { x1, y0 }, { 1.0f, 0.0f }, Color },
		{ { x1, y1 }, { 1.0f, 1.0f }, Color },
		{ { x0, y1 }, { 0.0f, 1.0f }, Color },
	};

	DrawQuad(false, verts, sizeof(Sprite2DVertex), pTexture);
}

// UV矩形を指定して画面座標へ描画する.
void SpriteRenderer::DrawSprite2DUV(
	ID3D12Resource* pTexture,
	float PosX, float PosY, float Width, float Height,
	float U0, float V0, float U1, float V1,
	const DirectX::XMFLOAT4& Color)
{
	const float screen_w = static_cast<float>(m_Dx12.GetBackBufferWidth());
	const float screen_h = static_cast<float>(m_Dx12.GetBackBufferHeight());
	if (screen_w <= 0.0f || screen_h <= 0.0f) { return; }

	const auto to_ndc_x = [screen_w](float x) { return (x / screen_w) * 2.0f - 1.0f; };
	const auto to_ndc_y = [screen_h](float y) { return 1.0f - (y / screen_h) * 2.0f; };

	const float x0 = to_ndc_x(PosX);
	const float y0 = to_ndc_y(PosY);
	const float x1 = to_ndc_x(PosX + Width);
	const float y1 = to_ndc_y(PosY + Height);

	Sprite2DVertex verts[4] = {
		{ { x0, y0 }, { U0, V0 }, Color },
		{ { x1, y0 }, { U1, V0 }, Color },
		{ { x1, y1 }, { U1, V1 }, Color },
		{ { x0, y1 }, { U0, V1 }, Color },
	};

	DrawQuad(false, verts, sizeof(Sprite2DVertex), pTexture);
}

// ワールド座標にカメラ向きのビルボード矩形を描画する.
void SpriteRenderer::DrawSprite3D(
	ID3D12Resource* pTexture,
	const DirectX::XMFLOAT3& Center, float Width, float Height,
	const DirectX::XMFLOAT4& Color)
{
	Sprite3DVertex verts[4];
	const DirectX::XMFLOAT2 corners[4] = {
		{ -0.5f,  0.5f }, // 左上.
		{  0.5f,  0.5f }, // 右上.
		{  0.5f, -0.5f }, // 右下.
		{ -0.5f, -0.5f }, // 左下.
	};
	const DirectX::XMFLOAT2 uvs[4] = {
		{ 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f },
	};

	for (int i = 0; i < 4; ++i)
	{
		verts[i].Center = Center;
		verts[i].Corner = corners[i];
		verts[i].Size   = { Width, Height };
		verts[i].Uv     = uvs[i];
		verts[i].Color  = Color;
	}

	DrawQuad(true, verts, sizeof(Sprite3DVertex), pTexture);
}
