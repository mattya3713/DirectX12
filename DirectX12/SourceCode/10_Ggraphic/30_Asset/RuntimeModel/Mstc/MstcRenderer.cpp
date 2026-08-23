#include "MstcRenderer.h"

#include <algorithm>
#include <d3dcompiler.h>

#include "d3dx12.h"
#include "10_Device/DirectX/DirectX12.h"

MstcRenderer::MstcRenderer(DirectX12& dx12)
	: m_Dx12(dx12)
{
	CreateRootSignature();
	CreateGraphicsPipeline();

	m_pWhiteTex = MyComPtr<ID3D12Resource>(CreateWhiteTexture());
}

MstcRenderer::~MstcRenderer()
{
}

void MstcRenderer::BeforDraw()
{
	auto command_list = m_Dx12.GetCommandList();
	command_list->SetPipelineState(m_pPipelineState.Get());
	command_list->SetGraphicsRootSignature(m_pRootSignature.Get());
}

ID3D12Resource* MstcRenderer::CreateWhiteTexture()
{
	constexpr size_t tex_size = 4;

	// リソースの設定.
	const auto resource_desc = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_R8G8B8A8_UNORM, static_cast<UINT>(tex_size), static_cast<UINT>(tex_size));

	// ヒープの設定(CPUから読み書き可能なメモリ領域).
	const auto heap_prop = CD3DX12_HEAP_PROPERTIES(
		D3D12_CPU_PAGE_PROPERTY_WRITE_BACK, D3D12_MEMORY_POOL_L0);

	ID3D12Resource* buffer = nullptr;
	MyAssert::IsFailed(
		_T("MstcRenderer: 白テクスチャの作成"),
		&ID3D12Device::CreateCommittedResource, m_Dx12.GetDevice(),
		&heap_prop, D3D12_HEAP_FLAG_NONE, &resource_desc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, nullptr,
		IID_PPV_ARGS(&buffer));

	std::vector<unsigned char> data(tex_size * tex_size * 4);
	std::fill(data.begin(), data.end(), 0xff);

	MyAssert::IsFailed(
		_T("MstcRenderer: テクスチャリソースを白で塗りつぶし"),
		&ID3D12Resource::WriteToSubresource, buffer,
		0, nullptr,
		static_cast<void*>(data.data()),
		static_cast<UINT>(tex_size) * 4,
		static_cast<UINT>(data.size()));

	return buffer;
}

void MstcRenderer::CreateGraphicsPipeline()
{
	MyComPtr<ID3DBlob> vs_blob(nullptr);
	MyComPtr<ID3DBlob> ps_blob(nullptr);

	// Debug: .hlslソースを実行時コンパイル. Release: 事前コンパイル済み.csoを読む.
#if _DEBUG
	CompileShaderFromFile(
		L"Data\\Shader\\Mstc\\Vertex.hlsl", "VS", "vs_5_0",
		vs_blob.ReleaseAndGetAddressOf());

	CompileShaderFromFile(
		L"Data\\Shader\\Mstc\\Pixel.hlsl", "PS", "ps_5_0",
		ps_blob.ReleaseAndGetAddressOf());
#else
	LoadCompiledShader(
		L"Data\\Shader\\Mstc\\Vertex.cso",
		vs_blob.ReleaseAndGetAddressOf());

	LoadCompiledShader(
		L"Data\\Shader\\Mstc\\Pixel.cso",
		ps_blob.ReleaseAndGetAddressOf());
#endif // _DEBUG.

	D3D12_INPUT_ELEMENT_DESC input_layout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC pipeline = {};
	pipeline.pRootSignature = m_pRootSignature.Get();
	pipeline.VS = CD3DX12_SHADER_BYTECODE(vs_blob.Get());
	pipeline.PS = CD3DX12_SHADER_BYTECODE(ps_blob.Get());

	pipeline.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	pipeline.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT); // 不透明.

	pipeline.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	pipeline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE; // 既存パイプラインに合わせカリングしない.

	pipeline.DepthStencilState.DepthEnable   = true;
	pipeline.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	pipeline.DepthStencilState.DepthFunc      = D3D12_COMPARISON_FUNC_LESS;
	pipeline.DSVFormat                        = DXGI_FORMAT_D32_FLOAT;
	pipeline.DepthStencilState.StencilEnable  = false;

	pipeline.InputLayout.pInputElementDescs = input_layout;
	pipeline.InputLayout.NumElements        = _countof(input_layout);

	pipeline.IBStripCutValue          = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
	pipeline.PrimitiveTopologyType    = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipeline.NumRenderTargets         = 1;
	pipeline.RTVFormats[0]            = DXGI_FORMAT_R8G8B8A8_UNORM;
	pipeline.SampleDesc.Count         = 1;
	pipeline.SampleDesc.Quality       = 0;

	MyAssert::IsFailed(
		_T("MstcRenderer: グラフィックパイプラインの作成"),
		&ID3D12Device::CreateGraphicsPipelineState, m_Dx12.GetDevice(),
		&pipeline,
		IID_PPV_ARGS(m_pPipelineState.ReleaseAndGetAddressOf()));
}

void MstcRenderer::CreateRootSignature()
{
	// ディスクリプタレンジの作成.
	D3D12_DESCRIPTOR_RANGE desc_ranges[3] = {};

	// 定数[b0](ビュープロジェクション用. DirectX12の共有バッファ).
	desc_ranges[0].NumDescriptors = 1;
	desc_ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	desc_ranges[0].BaseShaderRegister = 0;
	desc_ranges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// 定数[b1](ワールド変換用. VS/PS両方で使用するためALL).
	desc_ranges[1].NumDescriptors = 1;
	desc_ranges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	desc_ranges[1].BaseShaderRegister = 1;
	desc_ranges[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// 定数[b2](マテリアル用)とテクスチャ2枚(t0=ベースカラー, t1=オブジェクト空間法線マップ).
	desc_ranges[2].NumDescriptors = 1;
	desc_ranges[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	desc_ranges[2].BaseShaderRegister = 2;
	desc_ranges[2].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_DESCRIPTOR_RANGE texture_range{};
	texture_range.NumDescriptors = 2;
	texture_range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	texture_range.BaseShaderRegister = 0; // t0, t1.
	texture_range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_ROOT_PARAMETER root_params[3] = {};

	root_params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	root_params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	root_params[0].DescriptorTable.pDescriptorRanges = &desc_ranges[0];
	root_params[0].DescriptorTable.NumDescriptorRanges = 1;

	root_params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	root_params[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	root_params[1].DescriptorTable.pDescriptorRanges = &desc_ranges[1];
	root_params[1].DescriptorTable.NumDescriptorRanges = 1;

	D3D12_DESCRIPTOR_RANGE material_and_texture_ranges[] = { desc_ranges[2], texture_range };
	root_params[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	root_params[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	root_params[2].DescriptorTable.pDescriptorRanges = material_and_texture_ranges;
	root_params[2].DescriptorTable.NumDescriptorRanges = _countof(material_and_texture_ranges);

	D3D12_ROOT_SIGNATURE_DESC root_signature_desc = {};
	root_signature_desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	root_signature_desc.pParameters = root_params;
	root_signature_desc.NumParameters = _countof(root_params);

	CD3DX12_STATIC_SAMPLER_DESC sampler_desc[1] = {};
	sampler_desc[0].Init(0); // s0(ラップ).

	root_signature_desc.pStaticSamplers = sampler_desc;
	root_signature_desc.NumStaticSamplers = _countof(sampler_desc);

	MyComPtr<ID3DBlob> root_sig_blob(nullptr);
	MyComPtr<ID3DBlob> error_blob(nullptr);

	MyAssert::IsFailed(
		_T("MstcRenderer: ルートシグネチャをシリアライズする"),
		&D3D12SerializeRootSignature,
		&root_signature_desc,
		D3D_ROOT_SIGNATURE_VERSION_1,
		root_sig_blob.GetAddressOf(),
		error_blob.GetAddressOf());

	MyAssert::IsFailed(
		_T("MstcRenderer: ルートシグネチャの作成"),
		&ID3D12Device::CreateRootSignature, m_Dx12.GetDevice(),
		0,
		root_sig_blob->GetBufferPointer(),
		root_sig_blob->GetBufferSize(),
		IID_PPV_ARGS(m_pRootSignature.ReleaseAndGetAddressOf()));
}

HRESULT MstcRenderer::CompileShaderFromFile(
	const std::wstring& FilePath,
	LPCSTR EntryPoint,
	LPCSTR Target,
	ID3DBlob** ShaderBlob)
{
	ID3DBlob* error_blob = nullptr;
	HRESULT result = D3DCompileFromFile(
		FilePath.c_str(),
		nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		EntryPoint, Target,
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0, ShaderBlob, &error_blob
	);

	MyAssert::ErrorBlob(result, error_blob);

	return result;
}

HRESULT MstcRenderer::LoadCompiledShader(
	const std::wstring& FilePath,
	ID3DBlob** ShaderBlob)
{
	MyAssert::IsFailed(_T("MstcRenderer: コンパイル済みシェーダー(.cso)の読み込み"),
		D3DReadFileToBlob, FilePath.c_str(), ShaderBlob);

	return S_OK;
}
