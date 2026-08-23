#include "MmdlRenderer.h"
#include <cassert>
#include <d3dcompiler.h>
#include <string>
#include <algorithm>
#include "d3dx12.h"
#include "10_Device/DirectX/DirectX12.h"

constexpr size_t PMDTexWide = 4;

// PMX用の入力レイアウト(メインパイプラインとシャドウ深度パイプラインで共用).
namespace {
	D3D12_INPUT_ELEMENT_DESC g_PMXInputLayout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

		// AdditionalUVs (4つ追加)
		{ "TEXCOORD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }, // AdditionalUV0
		{ "TEXCOORD", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }, // AdditionalUV1
		{ "TEXCOORD", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }, // AdditionalUV2
		{ "TEXCOORD", 4, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }, // AdditionalUV3

		{ "BLENDINDICES", 0, DXGI_FORMAT_R32G32B32A32_UINT,  0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BLENDWEIGHTS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

		// SDEF Data (セマンティクスはHLSLと一致させる)
		{ "TEXCOORD", 5, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }, // SDEF_C
		{ "TEXCOORD", 6, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }, // SDEF_R0
		{ "TEXCOORD", 7, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }, // SDEF_R1

		{ "BLENDFACTOR", 0, DXGI_FORMAT_R32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }, // Edge
	};
}

MmdlRenderer::MmdlRenderer(DirectX12& dx12)
	: m_pDx12	(dx12)
{
	CreateRootSignature();
	CreateGraphicsPipelineForPMX();
	CreateShadowResources();

	// PMX用汎用テクスチャの生成.
	m_pAlphaTex = MyComPtr<ID3D12Resource>(CreateAlphaTexture());
	m_pWhiteTex = MyComPtr<ID3D12Resource>(CreateWhiteTexture());
	m_pBlackTex = MyComPtr<ID3D12Resource>(CreateBlackTexture());
	m_pGradTex  = MyComPtr<ID3D12Resource>(CreateGrayGradationTexture());
}


MmdlRenderer::~MmdlRenderer()
{
}

void MmdlRenderer::BeforDraw()
{
	auto cmdList = m_pDx12.GetCommandList();
	cmdList->SetPipelineState(m_pPipelineState.Get());
	cmdList->SetGraphicsRootSignature(m_pRootSignature.Get());
}

// シャドウマップ用の深度テクスチャ・DSV・深度専用パイプラインの初期化.
void MmdlRenderer::CreateShadowResources()
{
	// ===== 光源視点深度バッファ(D32_FLOAT) =====
	D3D12_RESOURCE_DESC shadow_desc = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_D32_FLOAT,
		SHADOW_MAP_SIZE,
		SHADOW_MAP_SIZE,
		1, 0, 1, 0,
		D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

	D3D12_CLEAR_VALUE shadow_clear{};
	shadow_clear.Format          = DXGI_FORMAT_D32_FLOAT;
	shadow_clear.DepthStencil.Depth = 1.0f;

	auto default_heap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	MyAssert::IsFailed(
		_T("シャドウマップ深度バッファの作成"),
		&ID3D12Device::CreateCommittedResource, m_pDx12.GetDevice(),
		&default_heap, D3D12_HEAP_FLAG_NONE, &shadow_desc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE, &shadow_clear,
		IID_PPV_ARGS(m_pShadowMap.ReleaseAndGetAddressOf()));

	// ===== DSVヒープ =====
	D3D12_DESCRIPTOR_HEAP_DESC dsv_heap_desc = {};
	dsv_heap_desc.NumDescriptors = 1;
	dsv_heap_desc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	MyAssert::IsFailed(
		_T("シャドウマップDSVヒープの作成"),
		&ID3D12Device::CreateDescriptorHeap, m_pDx12.GetDevice(),
		&dsv_heap_desc, IID_PPV_ARGS(m_pShadowDSVHeap.ReleaseAndGetAddressOf()));

	m_pDx12.GetDevice()->CreateDepthStencilView(
		m_pShadowMap.Get(), nullptr,
		m_pShadowDSVHeap->GetCPUDescriptorHandleForHeapStart());

	// ===== 深度専用パイプライン(VS_Shadow+PS無し) =====
	MyComPtr<ID3DBlob> VSBlob(nullptr);
#if _DEBUG
	CompileShaderFromFile(
		L"Data\\Shader\\PMX\\ShadowVertex.hlsl",
		"VS", "vs_5_0",
		VSBlob.ReleaseAndGetAddressOf());
#else
	LoadCompiledShader(
		L"Data\\Shader\\PMX\\ShadowVertex.cso",
		VSBlob.ReleaseAndGetAddressOf());
#endif

	D3D12_GRAPHICS_PIPELINE_STATE_DESC shadow_pipeline = {};
	shadow_pipeline.pRootSignature = m_pRootSignature.Get(); // メインと同一(ActorのDraw()をそのまま流用するため).
	shadow_pipeline.VS = CD3DX12_SHADER_BYTECODE(VSBlob.Get());
	shadow_pipeline.PS = { nullptr, 0 }; // カラー出力なし.
	shadow_pipeline.SampleMask    = D3D12_DEFAULT_SAMPLE_MASK;
	shadow_pipeline.BlendState    = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	shadow_pipeline.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	shadow_pipeline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE; // 薄いジオメトリの影欠け防止のためカリングしない.
	shadow_pipeline.DepthStencilState.DepthEnable   = true;
	shadow_pipeline.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	shadow_pipeline.DepthStencilState.DepthFunc      = D3D12_COMPARISON_FUNC_LESS;
	shadow_pipeline.DSVFormat       = DXGI_FORMAT_D32_FLOAT;
	shadow_pipeline.InputLayout.pInputElementDescs = g_PMXInputLayout;
	shadow_pipeline.InputLayout.NumElements        = _countof(g_PMXInputLayout);
	shadow_pipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	shadow_pipeline.NumRenderTargets = 0; // RTV無し(深度のみ).
	shadow_pipeline.SampleDesc.Count = 1;

	MyAssert::IsFailed(
		_T("シャドウ深度パイプラインの作成"),
		&ID3D12Device::CreateGraphicsPipelineState, m_pDx12.GetDevice(),
		&shadow_pipeline,
		IID_PPV_ARGS(m_pShadowPipelineState.ReleaseAndGetAddressOf())
	);
}

// シャドウ深度パスを開始する.
void MmdlRenderer::BeginShadowPass()
{
	auto cmdList = m_pDx12.GetCommandList();

	if (m_ShadowMapInShaderResourceState)
	{
		// 前フレームでSRVとして使った状態から深度書き込みへ戻す.
		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_pShadowMap.Get(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);
		cmdList->ResourceBarrier(1, &barrier);
		m_ShadowMapInShaderResourceState = false;
	}

	cmdList->SetPipelineState(m_pShadowPipelineState.Get());
	cmdList->SetGraphicsRootSignature(m_pRootSignature.Get()); // メインと共通のためActor側のSetRootDescriptorTableがそのまま使える.

	D3D12_VIEWPORT viewport{ 0.0f, 0.0f, static_cast<float>(SHADOW_MAP_SIZE), static_cast<float>(SHADOW_MAP_SIZE), 0.0f, 1.0f };
	D3D12_RECT scissor{ 0, 0, static_cast<LONG>(SHADOW_MAP_SIZE), static_cast<LONG>(SHADOW_MAP_SIZE) };
	cmdList->RSSetViewports(1, &viewport);
	cmdList->RSSetScissorRects(1, &scissor);

	auto dsv_handle = m_pShadowDSVHeap->GetCPUDescriptorHandleForHeapStart();
	cmdList->ClearDepthStencilView(dsv_handle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	cmdList->OMSetRenderTargets(0, nullptr, false, &dsv_handle);
}

// シャドウ深度パスを終了する.
void MmdlRenderer::EndShadowPass()
{
	auto cmdList = m_pDx12.GetCommandList();

	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_pShadowMap.Get(),
		D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	cmdList->ResourceBarrier(1, &barrier);
	m_ShadowMapInShaderResourceState = true;

	// メインパスのレンダーターゲット・ビューポートを復帰させる(シャドウパスで書き換えたため).
	m_pDx12.RestoreMainRenderTargets();
}

// テクスチャの汎用素材を作成.
ID3D12Resource* MmdlRenderer::CreateDefaultTexture(size_t Width, size_t Height) {

	// リソースの設定.
	auto ResourceDesc = 
		CD3DX12_RESOURCE_DESC::Tex2D(
			DXGI_FORMAT_R8G8B8A8_UNORM,		// RGB8bitフォーマット.
			static_cast<UINT>(Width),		// 幅.
			static_cast<UINT>(Height));		// 高さ.

	// ヒープの設定.
	auto TexHeapProp = 
		CD3DX12_HEAP_PROPERTIES(
		D3D12_CPU_PAGE_PROPERTY_WRITE_BACK, // CPUから読み書き可能なメモリ領域に配置.
			D3D12_MEMORY_POOL_L0);          // パフォーマンス優先のメモリプール.

	ID3D12Resource* Buffer= nullptr;

	MyAssert::IsFailed(
		_T("基本テクスチャの作成"),
		&ID3D12Device::CreateCommittedResource, m_pDx12.GetDevice(),
		&TexHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&ResourceDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(&Buffer)
	);

	return Buffer;
}

ID3D12Resource* MmdlRenderer::CreateAlphaTexture()
{
	// テクスチャリソースの作成.
	ID3D12Resource* TransparentBuff = CreateDefaultTexture(PMDTexWide, PMDTexWide);

	// RGBA (R,G,B,A) = (255, 255, 255, 0) → 白だけど透明
	std::vector<unsigned char> data(PMDTexWide * PMDTexWide * 4);
	for (size_t i = 0; i < data.size(); i += 4)
	{
		data[i + 0] = 0xFF; // R
		data[i + 1] = 0xFF; // G
		data[i + 2] = 0xFF; // B
		data[i + 3] = 0x00; // A ←ここが透明
	}

	MyAssert::IsFailed(
		_T("テクスチャリソースを透明白で塗りつぶし"),
		&ID3D12Resource::WriteToSubresource, TransparentBuff,
		0, nullptr,
		static_cast<void*>(data.data()),
		static_cast<UINT>(PMDTexWide) * 4,
		static_cast<UINT>(data.size()));

	return TransparentBuff;
}

// 白テクスチャ作成.
ID3D12Resource* MmdlRenderer::CreateWhiteTexture()
{
	// テクスチャリソースの作成.
	ID3D12Resource* WhiteBuff = CreateDefaultTexture(PMDTexWide, PMDTexWide);
	
	// テクスチャの範囲の白データ作成.
	std::vector<unsigned char> data(PMDTexWide * PMDTexWide * 4);
	std::fill(data.begin(), data.end(), 0xff);

	MyAssert::IsFailed(
		_T("テクスチャリソースを白で塗りつぶし"),
		&ID3D12Resource::WriteToSubresource, WhiteBuff,
		0, nullptr,
		static_cast<void*>(data.data()),
		4 * 4,
		static_cast<UINT>(data.size()));

	return WhiteBuff;
}

ID3D12Resource* MmdlRenderer::CreateBlackTexture()
{
	// テクスチャリソースの作成.
	ID3D12Resource* BlackBuff = CreateDefaultTexture(PMDTexWide, PMDTexWide);

	// テクスチャの範囲の黒データを作成.
	std::vector<unsigned char> data(PMDTexWide * PMDTexWide * 4);
	std::fill(data.begin(), data.end(), 0x00);

	MyAssert::IsFailed(
		_T("テクスチャリソースを黒で塗りつぶし"),
		&ID3D12Resource::WriteToSubresource, BlackBuff,
		0, nullptr,
		static_cast<void*>(data.data()), 
		4 * 4, 
		static_cast<UINT>(data.size()));

	return BlackBuff;
}

ID3D12Resource* MmdlRenderer::CreateGrayGradationTexture()
{
	ID3D12Resource* GradBuff = CreateDefaultTexture(4, 256);

	// テクスチャの範囲の白<->黒グラデーションデータの作成.
	std::vector<unsigned int> data(4 * 256);
	auto it = data.begin();
	unsigned int c = 0xff;
	for (; it != data.end(); it += 4) {
		auto col = (0xff << 24) | RGB(c, c, c);//RGBAが逆並びしているためRGBマクロと0xff<<24を用いて表す。
		std::fill(it, it + 4, col);
		--c;
	}

	MyAssert::IsFailed(
		_T("テクスチャリソースに白<->黒グラデーションを書き込む"),
		&ID3D12Resource::WriteToSubresource, GradBuff,
		0, nullptr,
		static_cast<void*>(data.data()),
		4 * 4,
		static_cast<UINT>(data.size()));

	return GradBuff;
}

//パイプライン初期化
void MmdlRenderer::CreateGraphicsPipelineForPMX() {

	MyComPtr<ID3DBlob> VSBlob(nullptr);		// 頂点シェーダーのブロブ.
	MyComPtr<ID3DBlob> PSBlob(nullptr);		// ピクセルシェーダーのブロブ.
	MyComPtr<ID3DBlob> ErrerBlob(nullptr);	// エラーのブロブ.
	HRESULT   result = S_OK;

	// シェーダーの読み込み.
	// Debug: .hlslソースを実行時コンパイル(編集して即実行できるように).
	// Release: ビルド時にfxc.exeで事前コンパイルした.csoを読むだけ(配布物にソースを含めない).
#if _DEBUG
	CompileShaderFromFile(
		L"Data\\Shader\\PMX\\Vertex.hlsl",
		"VS", "vs_5_0",
		VSBlob.ReleaseAndGetAddressOf());

	CompileShaderFromFile(
		L"Data\\Shader\\PMX\\Pixel.hlsl",
		"PS", "ps_5_0",
		PSBlob.ReleaseAndGetAddressOf());
#else
	LoadCompiledShader(
		L"Data\\Shader\\PMX\\Vertex.cso",
		VSBlob.ReleaseAndGetAddressOf());

	LoadCompiledShader(
		L"Data\\Shader\\PMX\\Pixel.cso",
		PSBlob.ReleaseAndGetAddressOf());
#endif // _DEBUG.

	D3D12_GRAPHICS_PIPELINE_STATE_DESC GraphicPipeLine = {};
	GraphicPipeLine.pRootSignature = m_pRootSignature.Get();
	GraphicPipeLine.VS = CD3DX12_SHADER_BYTECODE(VSBlob.Get());
	GraphicPipeLine.PS = CD3DX12_SHADER_BYTECODE(PSBlob.Get());

	GraphicPipeLine.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;//中身は0xffffffff

	GraphicPipeLine.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

	GraphicPipeLine.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	GraphicPipeLine.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;//カリングしない

	GraphicPipeLine.DepthStencilState.DepthEnable = true;//深度バッファを使うぞ
	GraphicPipeLine.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;//全て書き込み
	GraphicPipeLine.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;//小さい方を採用
	GraphicPipeLine.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	GraphicPipeLine.DepthStencilState.StencilEnable = false;

	GraphicPipeLine.InputLayout.pInputElementDescs = g_PMXInputLayout;//レイアウト先頭アドレス
	GraphicPipeLine.InputLayout.NumElements = _countof(g_PMXInputLayout);//レイアウト配列数

	GraphicPipeLine.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;//ストリップ時のカットなし
	GraphicPipeLine.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;//三角形で構成

	GraphicPipeLine.NumRenderTargets = 1;//今は１つのみ
	GraphicPipeLine.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;//0～1に正規化されたRGBA

	GraphicPipeLine.SampleDesc.Count = 1;//サンプリングは1ピクセルにつき１
	GraphicPipeLine.SampleDesc.Quality = 0;//クオリティは最低

	D3D12_RENDER_TARGET_BLEND_DESC transparencyBlendDesc{};
	transparencyBlendDesc.BlendEnable = true;
	transparencyBlendDesc.LogicOpEnable = false;
	transparencyBlendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
	transparencyBlendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	transparencyBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
	transparencyBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
	transparencyBlendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
	transparencyBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	transparencyBlendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
	transparencyBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	D3D12_GRAPHICS_PIPELINE_STATE_DESC transparentPipeline = GraphicPipeLine;
	transparentPipeline.BlendState.RenderTarget[0] = transparencyBlendDesc;
	transparentPipeline.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	GraphicPipeLine.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

	MyAssert::IsFailed(
		_T("グラフィックパイプラインの作成"),
		&ID3D12Device::CreateGraphicsPipelineState, m_pDx12.GetDevice(),
		&GraphicPipeLine,
		IID_PPV_ARGS(m_pPipelineState.ReleaseAndGetAddressOf())
	);
	MyAssert::IsFailed(
		_T("半透明グラフィックパイプラインの作成"),
		&ID3D12Device::CreateGraphicsPipelineState, m_pDx12.GetDevice(),
		&transparentPipeline,
		IID_PPV_ARGS(m_pTransparentPipelineState.ReleaseAndGetAddressOf())
	);
}

//ルートシグネチャ初期化
void MmdlRenderer::CreateRootSignature() 
{
	// ディスクリプタレンジの作成.
	D3D12_DESCRIPTOR_RANGE DescRanges[6] = {};

	// 定数[b0](ビュープロジェクション用).
	DescRanges[0].NumDescriptors = 1;
	DescRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	DescRanges[0].BaseShaderRegister = 0;
	DescRanges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// 定数[b1](ワールド、ボーン用).
	DescRanges[1].NumDescriptors = 1;
	DescRanges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	DescRanges[1].BaseShaderRegister = 1;
	DescRanges[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// 定数[b2](マテリアル用).
	DescRanges[2].NumDescriptors = 1;
	DescRanges[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	DescRanges[2].BaseShaderRegister = 2;
	DescRanges[2].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// テクスチャ3つ(基本とsphとトゥーン).
	DescRanges[3].NumDescriptors = 3;
	DescRanges[3].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	DescRanges[3].BaseShaderRegister = 0; // t0, t1, t2 に対応
	DescRanges[3].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// ボーン座標.
	DescRanges[4].NumDescriptors = 1; // 1つのStructuredBuffer
	DescRanges[4].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; // シェーダーリソースビュー
	DescRanges[4].BaseShaderRegister = 3; // t3 レジスタに対応
	DescRanges[4].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// シャドウマップ(ピクセルシェーダー用のTexture2D. 頂点シェーダー用のt3とはステージが分離されているためレジスタを再利用).
	DescRanges[5].NumDescriptors = 1;
	DescRanges[5].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	DescRanges[5].BaseShaderRegister = 3;
	DescRanges[5].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// ルートパラメータの作成.
	D3D12_ROOT_PARAMETER Rootparams[5] = {};

	// ビュープロジェクション変換.
	Rootparams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	Rootparams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	Rootparams[0].DescriptorTable.pDescriptorRanges = &DescRanges[0];
	Rootparams[0].DescriptorTable.NumDescriptorRanges = 1;

	// ワールド・ボーン変換.
	Rootparams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	Rootparams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	Rootparams[1].DescriptorTable.pDescriptorRanges = &DescRanges[1];
	Rootparams[1].DescriptorTable.NumDescriptorRanges = 1;

	// マテリアル周り.	
	// DescRanges[2] (b2) と DescRanges[3] (t0, t1, t2) を含む配列を作成し、
	// それを Rootparams[2] に設定する。
	D3D12_DESCRIPTOR_RANGE MaterialAndTextureRanges[] = { DescRanges[2], DescRanges[3] };
	Rootparams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	Rootparams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	Rootparams[2].DescriptorTable.pDescriptorRanges = MaterialAndTextureRanges;
	Rootparams[2].DescriptorTable.NumDescriptorRanges = _countof(MaterialAndTextureRanges); // 配列のサイズを動的に取得

	Rootparams[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	Rootparams[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX; // 頂点シェーダーからのみ見える
	Rootparams[3].DescriptorTable.pDescriptorRanges = &DescRanges[4]; // DescRanges[4] を指す
	Rootparams[3].DescriptorTable.NumDescriptorRanges = 1;

	// シャドウマップ(ピクセルシェーダーからのみ見える).
	Rootparams[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	Rootparams[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	Rootparams[4].DescriptorTable.pDescriptorRanges = &DescRanges[5];
	Rootparams[4].DescriptorTable.NumDescriptorRanges = 1;

	// ルートシグネクチャの作成.
	D3D12_ROOT_SIGNATURE_DESC RootSignatureDesc = {};

	RootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	RootSignatureDesc.pParameters = Rootparams;
	RootSignatureDesc.NumParameters = _countof(Rootparams); // Rootparams のサイズを動的に取得

	// サンプラーの作成.
	CD3DX12_STATIC_SAMPLER_DESC SamplerDesc[3] = {}; // s0/s1に加えシャドウ比較用s2を追加.
	SamplerDesc[0].Init(0); // s0
	SamplerDesc[1].Init(1, D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // s1
	SamplerDesc[2].Init(
		2,                                        // s2
		D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, // PCF用のバイリニア比較フィルタ.
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,        // 範囲外はボーダー値(0.0=影)扱い.
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,
		0.0f,                                     // mipLODBias.
		16,                                       // maxAnisotropy.
		D3D12_COMPARISON_FUNC_LESS_EQUAL,         // SampleCmp用の比較関数.
		D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK);

	RootSignatureDesc.pStaticSamplers = SamplerDesc;
	RootSignatureDesc.NumStaticSamplers = _countof(SamplerDesc); // サンプラーの数を動的に取得

	MyComPtr<ID3DBlob> RootSigBlob(nullptr);
	MyComPtr<ID3DBlob> ErrorBlob(nullptr);

	MyAssert::IsFailed(
		_T("ルートシグネクチャをシリアライズする"),
		&D3D12SerializeRootSignature,
		&RootSignatureDesc,
		D3D_ROOT_SIGNATURE_VERSION_1,
		RootSigBlob.GetAddressOf(),
		ErrorBlob.GetAddressOf()
	);
	
	MyAssert::IsFailed(
		_T("ルートシグネクチャをシリアライズする"),
		&ID3D12Device::CreateRootSignature, m_pDx12.GetDevice(),
		0,
		RootSigBlob->GetBufferPointer(),
		RootSigBlob->GetBufferSize(),
		IID_PPV_ARGS(m_pRootSignature.ReleaseAndGetAddressOf())
	);

}

// シェーダーのコンパイル.
HRESULT MmdlRenderer::CompileShaderFromFile(
	const std::wstring& FilePath,
	LPCSTR EntryPoint,
	LPCSTR Target,
	ID3DBlob** ShaderBlob)
{
	ID3DBlob* ErrorBlob = nullptr;
	HRESULT Result = D3DCompileFromFile(
		FilePath.c_str(),
		nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		EntryPoint, Target,
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, // デバッグオプション.
		0, ShaderBlob, &ErrorBlob
	);

	// コンパイルエラー時にエラーハンドリングを行う.
	MyAssert::ErrorBlob(Result, ErrorBlob);

	return Result;
}

// 事前コンパイル済みシェーダー(.cso)の読み込み(Releaseビルド用).
HRESULT MmdlRenderer::LoadCompiledShader(
	const std::wstring& FilePath,
	ID3DBlob** ShaderBlob)
{
	MyAssert::IsFailed(_T("コンパイル済みシェーダー(.cso)の読み込み(Releaseビルドを最初からやり直してください)"),
		D3DReadFileToBlob, FilePath.c_str(), ShaderBlob);

	return S_OK;
}

// PMD用のパイプラインステートを取得.
ID3D12PipelineState* MmdlRenderer::GetPipelineState()
{
	return m_pPipelineState.Get();
}

ID3D12PipelineState* MmdlRenderer::GetTransparentPipelineState()
{
	return m_pTransparentPipelineState.Get();
}

// PMD用のルート署名を取得.
ID3D12RootSignature* MmdlRenderer::GetRootSignature()
{
	return m_pRootSignature.Get();
}

MyComPtr<ID3D12Resource>& MmdlRenderer::GetAlphaTex()
{
	return m_pAlphaTex;
}

MyComPtr<ID3D12Resource>& MmdlRenderer::GetWhiteTex()
{
	return m_pWhiteTex;
}

MyComPtr<ID3D12Resource>& MmdlRenderer::GetBlackTex()
{
	return m_pBlackTex;
}

MyComPtr<ID3D12Resource>& MmdlRenderer::GetGradTex()
{
	return m_pGradTex;
}
