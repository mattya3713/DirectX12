#include "PMDRenderer.h"
#include<d3dx12.h>
#include<cassert>
#include<d3dcompiler.h>
#include"../DirectX/CDirectX12.h"
#include<string>
#include<algorithm>

constexpr size_t PMDTexWide = 4;

CPMDRenderer::CPMDRenderer(CDirectX12& dx12):m_pDx12(dx12)
{
	CreateRootSignature();
	CreateGraphicsPipelineForPMD();

	// PMD用汎用テクスチャの生成.
	m_pWhiteTex = MyComPtr<ID3D12Resource>(CreateWhiteTexture());
	m_pBlackTex = MyComPtr<ID3D12Resource>(CreateBlackTexture());
	m_pGradTex  = MyComPtr<ID3D12Resource>(CreateGrayGradationTexture());
}


CPMDRenderer::~CPMDRenderer()
{
}


void 
CPMDRenderer::Update() {

}
void 
CPMDRenderer::Draw() {

}

// テクスチャの汎用素材を作成.
ID3D12Resource* CPMDRenderer::CreateDefaultTexture(size_t Width, size_t Height) {

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
		D3D12_HEAP_FLAG_NONE,//特に指定なし
		&ResourceDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(&Buffer)
	);

	return Buffer;
}

// 白テクスチャ作成.
ID3D12Resource* CPMDRenderer::CreateWhiteTexture()
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
		data.data(), 4 * 4, data.size());

	return WhiteBuff;
}
ID3D12Resource*	CPMDRenderer::CreateBlackTexture() 
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
		data.data(), 4 * 4, data.size());

	return BlackBuff;
}
ID3D12Resource*	CPMDRenderer::CreateGrayGradationTexture() 
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
		data.data(), 4 * 4, data.size());

	return GradBuff;
}

//パイプライン初期化
void CPMDRenderer::CreateGraphicsPipelineForPMD() {

	MyComPtr<ID3DBlob> VSBlob(nullptr);		// 頂点シェーダーのブロブ.
	MyComPtr<ID3DBlob> PSBlob(nullptr);		// ピクセルシェーダーのブロブ.
	MyComPtr<ID3DBlob> ErrerBlob(nullptr);	// エラーのブロブ.
	HRESULT   result = S_OK;

	// 頂点シェーダーの読み込み.
	CompileShaderFromFile(
		L"Data\\Shader\\Basic\\BasicVertexShader.hlsl",
		"BasicVS", "vs_5_0",
		VSBlob.ReleaseAndGetAddressOf());


	CompileShaderFromFile(
		L"Data\\Shader\\Basic\\BasicPixelShader.hlsl",
		"BasicPS", "ps_5_0",
		PSBlob.ReleaseAndGetAddressOf());

	// TODO : 短くできそう.
	D3D12_INPUT_ELEMENT_DESC InputLayout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL"	, 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT	, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BONE_NO"	, 0, DXGI_FORMAT_R16G16_UINT	, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "WEIGHT"	, 0, DXGI_FORMAT_R8_UINT		, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		//{ "EDGE_FLG",0,DXGI_FORMAT_R8_UINT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpipeline = {};
	gpipeline.pRootSignature = m_pRootSignature.Get();
	gpipeline.VS = CD3DX12_SHADER_BYTECODE(VSBlob.Get());
	gpipeline.PS = CD3DX12_SHADER_BYTECODE(PSBlob.Get());

	gpipeline.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;//中身は0xffffffff


	gpipeline.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

	gpipeline.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	gpipeline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;//カリングしない

	gpipeline.DepthStencilState.DepthEnable = true;//深度バッファを使うぞ
	gpipeline.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;//全て書き込み
	gpipeline.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;//小さい方を採用
	gpipeline.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	gpipeline.DepthStencilState.StencilEnable = false;

	gpipeline.InputLayout.pInputElementDescs = InputLayout;//レイアウト先頭アドレス
	gpipeline.InputLayout.NumElements = _countof(InputLayout);//レイアウト配列数

	gpipeline.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;//ストリップ時のカットなし
	gpipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;//三角形で構成

	gpipeline.NumRenderTargets = 1;//今は１つのみ
	gpipeline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;//0～1に正規化されたRGBA

	gpipeline.SampleDesc.Count = 1;//サンプリングは1ピクセルにつき１
	gpipeline.SampleDesc.Quality = 0;//クオリティは最低

	MyAssert::IsFailed(
		_T("グラフィックパイプラインの作成"),
		&ID3D12Device::CreateGraphicsPipelineState, m_pDx12.GetDevice(),
		&gpipeline,
		IID_PPV_ARGS(m_pPipelineState.ReleaseAndGetAddressOf())
	);

}

//ルートシグネチャ初期化
void CPMDRenderer::CreateRootSignature() {
	//レンジ
	CD3DX12_DESCRIPTOR_RANGE  descTblRanges[4] = {};//テクスチャと定数の２つ
	descTblRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);//定数[b0](ビュープロジェクション用)
	descTblRanges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);//定数[b1](ワールド、ボーン用)
	descTblRanges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 2);//定数[b2](マテリアル用)
	descTblRanges[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 0);//テクスチャ４つ(基本とsphとspaとトゥーン)

	//ルートパラメータ
	CD3DX12_ROOT_PARAMETER rootParams[3] = {};
	rootParams[0].InitAsDescriptorTable(1, &descTblRanges[0]);//ビュープロジェクション変換
	rootParams[1].InitAsDescriptorTable(1, &descTblRanges[1]);//ワールド・ボーン変換
	rootParams[2].InitAsDescriptorTable(2, &descTblRanges[2]);//マテリアル周り

	CD3DX12_STATIC_SAMPLER_DESC samplerDescs[2] = {};
	samplerDescs[0].Init(0);
	samplerDescs[1].Init(1, D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	rootSignatureDesc.Init(3, rootParams, 2, samplerDescs, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	MyComPtr<ID3DBlob> RootSigBlob(nullptr);
	MyComPtr<ID3DBlob> ErrorBlob(nullptr);

	MyAssert::IsFailed(
		_T("ルートシグネクチャをシリアライズする"),
		&D3D12SerializeRootSignature,
		&rootSignatureDesc,
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
HRESULT CPMDRenderer::CompileShaderFromFile(
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

// PMD用のパイプラインステートを取得.
ID3D12PipelineState* CPMDRenderer::GetPipelineState()
{
	return m_pPipelineState.Get();
}

// PMD用のルート署名を取得.
ID3D12RootSignature* CPMDRenderer::GetRootSignature() 
{
	return m_pRootSignature.Get();
}