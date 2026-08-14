#include "DebugGrid.h"

#include <algorithm>
#include <d3dcompiler.h>
#include <vector>

#include "d3dx12.h"
#include "10_Ggraphic/DirectX/DirectX12.h"
#include "00_Game/30_Camera/00_Base/CameraBase.h"
#include "00_Game/30_Camera/99_Manager/CameraManager.h"
#include "99_Utility/ServiceLocator/ServiceLocator.h"

namespace
{
struct GridVertex
{
	DirectX::XMFLOAT3 Position;
	DirectX::XMFLOAT3 Color;
};
}

DebugGrid::DebugGrid(DirectX12& Dx12)
	: m_Dx12{ Dx12 }
{
	CreatePipeline();
	CreateGridVertices();

	const UINT constant_buffer_size = (sizeof(DirectX::XMMATRIX) + 255u) & ~255u;
	const D3D12_HEAP_PROPERTIES upload_heap_properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	const D3D12_RESOURCE_DESC constant_buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(constant_buffer_size);
	MyAssert::IsFailed(_T("DebugGridの定数バッファ作成"), &ID3D12Device::CreateCommittedResource,
		m_Dx12.GetDevice(), &upload_heap_properties, D3D12_HEAP_FLAG_NONE, &constant_buffer_desc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(m_pConstantBuffer.ReleaseAndGetAddressOf()));
	MyAssert::IsFailed(_T("DebugGridの定数バッファをマップ"), &ID3D12Resource::Map,
		m_pConstantBuffer.Get(), 0, nullptr, reinterpret_cast<void**>(&m_pMappedConstantBuffer));
	if (m_pMappedConstantBuffer != nullptr) {
		*m_pMappedConstantBuffer = DirectX::XMMatrixIdentity();
	}
}

DebugGrid::~DebugGrid()
{
	if (m_pMappedConstantBuffer != nullptr && m_pConstantBuffer != nullptr) {
		m_pConstantBuffer->Unmap(0, nullptr);
		m_pMappedConstantBuffer = nullptr;
	}
}

void DebugGrid::CreatePipeline()
{
	MyComPtr<ID3DBlob> vertex_shader_blob;
	MyComPtr<ID3DBlob> pixel_shader_blob;
	MyComPtr<ID3DBlob> error_blob;

	HRESULT result = D3DCompileFromFile(L"Data\\Shader\\Debug\\GridVertex.hlsl", nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "vs_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0,
		vertex_shader_blob.ReleaseAndGetAddressOf(), error_blob.ReleaseAndGetAddressOf());
	MyAssert::ErrorBlob(result, error_blob.Detach());

	result = D3DCompileFromFile(L"Data\\Shader\\Debug\\GridPixel.hlsl", nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "ps_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0,
		pixel_shader_blob.ReleaseAndGetAddressOf(), error_blob.ReleaseAndGetAddressOf());
	MyAssert::ErrorBlob(result, error_blob.Detach());

	CD3DX12_ROOT_PARAMETER root_parameter;
	root_parameter.InitAsConstantBufferView(0);
	const CD3DX12_ROOT_SIGNATURE_DESC root_signature_desc{
		1, &root_parameter, 0, nullptr,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
	};

	MyComPtr<ID3DBlob> root_signature_blob;
	MyComPtr<ID3DBlob> root_signature_error_blob;
	MyAssert::IsFailed(_T("DebugGridのルートシグネチャをシリアライズ"), &D3D12SerializeRootSignature,
		&root_signature_desc, D3D_ROOT_SIGNATURE_VERSION_1, root_signature_blob.ReleaseAndGetAddressOf(),
		root_signature_error_blob.ReleaseAndGetAddressOf());
	MyAssert::IsFailed(_T("DebugGridのルートシグネチャ作成"), &ID3D12Device::CreateRootSignature,
		m_Dx12.GetDevice(), 0, root_signature_blob->GetBufferPointer(), root_signature_blob->GetBufferSize(),
		IID_PPV_ARGS(m_pRootSignature.ReleaseAndGetAddressOf()));

	const D3D12_INPUT_ELEMENT_DESC input_layout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
	D3D12_GRAPHICS_PIPELINE_STATE_DESC pipeline_desc{};
	pipeline_desc.pRootSignature = m_pRootSignature.Get();
	pipeline_desc.VS = CD3DX12_SHADER_BYTECODE(vertex_shader_blob.Get());
	pipeline_desc.PS = CD3DX12_SHADER_BYTECODE(pixel_shader_blob.Get());
	pipeline_desc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	pipeline_desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	pipeline_desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	pipeline_desc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	pipeline_desc.InputLayout = { input_layout, _countof(input_layout) };
	pipeline_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
	pipeline_desc.NumRenderTargets = 1;
	pipeline_desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	pipeline_desc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	pipeline_desc.SampleDesc.Count = 1;
	MyAssert::IsFailed(_T("DebugGridのパイプライン作成"), &ID3D12Device::CreateGraphicsPipelineState,
		m_Dx12.GetDevice(), &pipeline_desc, IID_PPV_ARGS(m_pPipelineState.ReleaseAndGetAddressOf()));
}

void DebugGrid::CreateGridVertices()
{
	const DirectX::XMFLOAT3 gray{ 0.5f, 0.5f, 0.5f };
	const DirectX::XMFLOAT3 blue{ 0.0f, 0.0f, 1.0f };
	const DirectX::XMFLOAT3 red{ 1.0f, 0.0f, 0.0f };
	std::vector<GridVertex> vertices;
	vertices.reserve(84);
	for (int x = -10; x <= 10; ++x) {
		const DirectX::XMFLOAT3 color = x == 0 ? blue : gray;
		vertices.push_back({ { static_cast<float>(x), 0.0f, -10.0f }, color });
		vertices.push_back({ { static_cast<float>(x), 0.0f, 10.0f }, color });
	}
	for (int z = -10; z <= 10; ++z) {
		const DirectX::XMFLOAT3 color = z == 0 ? red : gray;
		vertices.push_back({ { -10.0f, 0.0f, static_cast<float>(z) }, color });
		vertices.push_back({ { 10.0f, 0.0f, static_cast<float>(z) }, color });
	}

	const D3D12_HEAP_PROPERTIES upload_heap_properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	const D3D12_RESOURCE_DESC vertex_buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(
		static_cast<UINT64>(vertices.size() * sizeof(GridVertex)));
	MyAssert::IsFailed(_T("DebugGridの頂点バッファ作成"), &ID3D12Device::CreateCommittedResource,
		m_Dx12.GetDevice(), &upload_heap_properties, D3D12_HEAP_FLAG_NONE, &vertex_buffer_desc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(m_pVertexBuffer.ReleaseAndGetAddressOf()));
	GridVertex* p_mapped_vertices = nullptr;
	MyAssert::IsFailed(_T("DebugGridの頂点バッファをマップ"), &ID3D12Resource::Map,
		m_pVertexBuffer.Get(), 0, nullptr, reinterpret_cast<void**>(&p_mapped_vertices));
	std::copy(vertices.begin(), vertices.end(), p_mapped_vertices);
	m_pVertexBuffer->Unmap(0, nullptr);

	m_VertexCount = static_cast<UINT>(vertices.size());
	m_VertexBufferView.BufferLocation = m_pVertexBuffer->GetGPUVirtualAddress();
	m_VertexBufferView.SizeInBytes = static_cast<UINT>(vertices.size() * sizeof(GridVertex));
	m_VertexBufferView.StrideInBytes = sizeof(GridVertex);
}

void DebugGrid::Draw()
{
	CameraManager* p_camera_manager = ServiceLocator::Get<CameraManager>();
	CameraBase* p_active_camera = p_camera_manager != nullptr ? p_camera_manager->GetActive() : nullptr;
	if (p_active_camera == nullptr || m_pMappedConstantBuffer == nullptr) {
		return;
	}
	*m_pMappedConstantBuffer = p_active_camera->GetViewProjMatrix();

	ID3D12GraphicsCommandList* p_command_list = m_Dx12.GetCommandList().Get();
	p_command_list->SetGraphicsRootSignature(m_pRootSignature.Get());
	p_command_list->SetPipelineState(m_pPipelineState.Get());
	p_command_list->IASetVertexBuffers(0, 1, &m_VertexBufferView);
	p_command_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
	p_command_list->SetGraphicsRootConstantBufferView(0, m_pConstantBuffer->GetGPUVirtualAddress());
	p_command_list->DrawInstanced(m_VertexCount, 1, 0, 0);
}
