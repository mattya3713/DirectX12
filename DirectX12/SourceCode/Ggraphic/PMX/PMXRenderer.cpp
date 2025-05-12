#include "PMXRenderer.h"
#include <cassert>
#include <d3dcompiler.h>
#include <string>
#include <algorithm>
#include "DirectXTex/DirectXTex/d3dx12.h"
#include "DirectX/DirectX12.h"
#include "Ggraphic/PMX/PMXActor.h"

constexpr size_t PMDTexWide = 4;

PMXRenderer::PMXRenderer(DirectX12& dx12)
	: m_pDx12	(dx12)
{
	CreateRootSignature();
	CreateGraphicsPipelineForPMX();

	// PMX用汎用テクスチャの生成.
	m_pAlphaTex = MyComPtr<ID3D12Resource>(CreateAlphaTexture());
	m_pWhiteTex = MyComPtr<ID3D12Resource>(CreateWhiteTexture());
	m_pBlackTex = MyComPtr<ID3D12Resource>(CreateBlackTexture());
	m_pGradTex  = MyComPtr<ID3D12Resource>(CreateGrayGradationTexture());
}


PMXRenderer::~PMXRenderer()
{
}


void PMXRenderer::Update() {

}

void PMXRenderer::BeforDraw()
{
	auto cmdList = m_pDx12.GetCommandList();
	cmdList->SetPipelineState(m_pPipelineState.Get());
	cmdList->SetGraphicsRootSignature(m_pRootSignature.Get());
	
	m_pDx12.SetScene();
}

void PMXRenderer::Draw() 
{
	for (auto& actor : m_pActors)
	{
		actor->Draw();
	}
}

void PMXRenderer::AddActor(std::shared_ptr<PMXActor> pActor)
{
}

// テクスチャの汎用素材を作成.
ID3D12Resource* PMXRenderer::CreateDefaultTexture(size_t Width, size_t Height) {

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

ID3D12Resource* PMXRenderer::CreateAlphaTexture()
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
		PMDTexWide * 4,
		static_cast<UINT>(data.size()));

	return TransparentBuff;
}

// 白テクスチャ作成.
ID3D12Resource* PMXRenderer::CreateWhiteTexture()
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

ID3D12Resource* PMXRenderer::CreateBlackTexture()
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

ID3D12Resource* PMXRenderer::CreateGrayGradationTexture()
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
void PMXRenderer::CreateGraphicsPipelineForPMX() {

	MyComPtr<ID3DBlob> VSBlob(nullptr);		// 頂点シェーダーのブロブ.
	MyComPtr<ID3DBlob> PSBlob(nullptr);		// ピクセルシェーダーのブロブ.
	MyComPtr<ID3DBlob> ErrerBlob(nullptr);	// エラーのブロブ.
	HRESULT   result = S_OK;

	// 頂点シェーダーの読み込み.
	CompileShaderFromFile(
		L"Data\\Shader\\PMX\\PMXBasicVertexShader.hlsl",
		"VS", "vs_5_0",
		VSBlob.ReleaseAndGetAddressOf());

	CompileShaderFromFile(
		L"Data\\Shader\\PMX\\PMXBasicPixelShader.hlsl",
		"PS", "ps_5_0",
		PSBlob.ReleaseAndGetAddressOf());

	D3D12_INPUT_ELEMENT_DESC PMXInputLayout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL"	, 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT	, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

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

	GraphicPipeLine.InputLayout.pInputElementDescs = PMXInputLayout;//レイアウト先頭アドレス
	GraphicPipeLine.InputLayout.NumElements = _countof(PMXInputLayout);//レイアウト配列数

	GraphicPipeLine.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;//ストリップ時のカットなし
	GraphicPipeLine.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;//三角形で構成

	GraphicPipeLine.NumRenderTargets = 1;//今は１つのみ
	GraphicPipeLine.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;//0～1に正規化されたRGBA

	GraphicPipeLine.SampleDesc.Count = 1;//サンプリングは1ピクセルにつき１
	GraphicPipeLine.SampleDesc.Quality = 0;//クオリティは最低

	D3D12_RENDER_TARGET_BLEND_DESC transparencyBlendDesc;
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
	GraphicPipeLine.BlendState.RenderTarget[0] = transparencyBlendDesc;

	MyAssert::IsFailed(
		_T("グラフィックパイプラインの作成"),
		&ID3D12Device::CreateGraphicsPipelineState, m_pDx12.GetDevice(),
		&GraphicPipeLine,
		IID_PPV_ARGS(m_pPipelineState.ReleaseAndGetAddressOf())
	);
}

//ルートシグネチャ初期化
void PMXRenderer::CreateRootSignature() 
{
	// ディスクリプタレンジの作成.
	D3D12_DESCRIPTOR_RANGE DescRanges[4] = {};

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
	DescRanges[3].BaseShaderRegister = 0;
	DescRanges[3].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND; 
	
	// ルートパラメータの作成.
	D3D12_ROOT_PARAMETER Rootparams[3] = {};

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
	std::array<D3D12_DESCRIPTOR_RANGE, 2> MaterialRanges = { DescRanges[2], DescRanges[3] };
	Rootparams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	Rootparams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	Rootparams[2].DescriptorTable.pDescriptorRanges = MaterialRanges.data();
	Rootparams[2].DescriptorTable.NumDescriptorRanges = static_cast<UINT>(MaterialRanges.size());
	
	// ルートシグネクチャの作成.
	D3D12_ROOT_SIGNATURE_DESC RootSignatureDesc = {};

	RootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	RootSignatureDesc.pParameters = Rootparams;
	RootSignatureDesc.NumParameters = 3; 
	
	// サンプラーの作成.
	CD3DX12_STATIC_SAMPLER_DESC SamplerDesc[3] = {};
	SamplerDesc[0].Init(0);
	SamplerDesc[1].Init(1, D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

	RootSignatureDesc.pStaticSamplers = SamplerDesc;
	RootSignatureDesc.NumStaticSamplers = 2;

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
HRESULT PMXRenderer::CompileShaderFromFile(
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
ID3D12PipelineState* PMXRenderer::GetPipelineState()
{
	return m_pPipelineState.Get();
}

// PMD用のルート署名を取得.
ID3D12RootSignature* PMXRenderer::GetRootSignature()
{
	return m_pRootSignature.Get();
}

MyComPtr<ID3D12Resource>& PMXRenderer::GetAlphaTex()
{
	return m_pAlphaTex;
}

MyComPtr<ID3D12Resource>& PMXRenderer::GetWhiteTex()
{
	return m_pWhiteTex;
}

MyComPtr<ID3D12Resource>& PMXRenderer::GetBlackTex()
{
	return m_pBlackTex;
}

MyComPtr<ID3D12Resource>& PMXRenderer::GetGradTex()
{
	return m_pGradTex;
}