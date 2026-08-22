#include "DebugColliderRenderer.h"

#include <array>
#include <cmath>
#include <cstring>
#include <d3dcompiler.h>

#include "d3dx12.h"
#include "10_Ggraphic/10_Device/DirectX/DirectX12.h"
#include "00_Game/30_Camera/00_Base/CameraBase.h"
#include "00_Game/30_Camera/99_Manager/CameraManager.h"
#include "99_Utility/ServiceLocator/ServiceLocator.h"

namespace
{
	struct LineVertex
	{
		DirectX::XMFLOAT3 Position;
		DirectX::XMFLOAT3 Color;
	};

	constexpr int RING_SEGMENT_COUNT    = 12; // 赤道リング(始点・終点それぞれ)の分割数.
	constexpr int ARC_SEGMENT_COUNT     = 8;  // 半球キャップ1本分の円弧の分割数.
	constexpr int SILHOUETTE_LINE_COUNT = 4;  // 円柱側面の縦線本数.

	// カプセル1本分の総頂点数(線分リストのため、線分の数だけ2頂点ずつ必要).
	constexpr int CAPSULE_VERTEX_COUNT =
		(RING_SEGMENT_COUNT * 2) * 2 +   // 上下2本のリング.
		(SILHOUETTE_LINE_COUNT * 2) +    // 側面の縦線.
		(ARC_SEGMENT_COUNT * 2) * 8;     // 上下2半球 x 0/90/180/270度4方向の円弧(2本だけだと半球が半分欠ける).

	// 中心Center・半径Radiusの円周(XZ平面)上、角度Angle(rad)の点を求める.
	DirectX::XMFLOAT3 RingPoint(const DirectX::XMFLOAT3& Center, float Radius, float Angle) noexcept
	{
		return DirectX::XMFLOAT3{
			Center.x + Radius * std::cosf(Angle),
			Center.y,
			Center.z + Radius * std::sinf(Angle) };
	}

	// 円周をRING_SEGMENT_COUNT本の線分として書き込む.
	int AppendRing(LineVertex* pOut, int Index, const DirectX::XMFLOAT3& Center, float Radius, const DirectX::XMFLOAT3& Color) noexcept
	{
		for (int i = 0; i < RING_SEGMENT_COUNT; ++i)
		{
			const float angle_a = DirectX::XM_2PI * static_cast<float>(i) / static_cast<float>(RING_SEGMENT_COUNT);
			const float angle_b = DirectX::XM_2PI * static_cast<float>(i + 1) / static_cast<float>(RING_SEGMENT_COUNT);
			pOut[Index++] = { RingPoint(Center, Radius, angle_a), Color };
			pOut[Index++] = { RingPoint(Center, Radius, angle_b), Color };
		}
		return Index;
	}

	// 半球キャップの円弧を1本書き込む(CapCenterからHorizontalAngle方向・Upward方向へ90度分).
	int AppendCapArc(LineVertex* pOut, int Index, const DirectX::XMFLOAT3& CapCenter, float Radius,
		float HorizontalAngle, bool Upward, const DirectX::XMFLOAT3& Color) noexcept
	{
		const float dir_y = Upward ? 1.0f : -1.0f;
		const float cos_h = std::cosf(HorizontalAngle);
		const float sin_h = std::sinf(HorizontalAngle);

		for (int i = 0; i < ARC_SEGMENT_COUNT; ++i)
		{
			const float t_a = DirectX::XM_PIDIV2 * static_cast<float>(i) / static_cast<float>(ARC_SEGMENT_COUNT);
			const float t_b = DirectX::XM_PIDIV2 * static_cast<float>(i + 1) / static_cast<float>(ARC_SEGMENT_COUNT);

			const DirectX::XMFLOAT3 point_a{
				CapCenter.x + Radius * std::cosf(t_a) * cos_h,
				CapCenter.y + Radius * std::sinf(t_a) * dir_y,
				CapCenter.z + Radius * std::cosf(t_a) * sin_h };
			const DirectX::XMFLOAT3 point_b{
				CapCenter.x + Radius * std::cosf(t_b) * cos_h,
				CapCenter.y + Radius * std::sinf(t_b) * dir_y,
				CapCenter.z + Radius * std::cosf(t_b) * sin_h };

			pOut[Index++] = { point_a, Color };
			pOut[Index++] = { point_b, Color };
		}
		return Index;
	}

	// カプセル1本分のワイヤーフレーム頂点(線分リスト)を組み立てる.
	void BuildCapsuleVertices(std::array<LineVertex, CAPSULE_VERTEX_COUNT>& OutVertices,
		const DirectX::XMFLOAT3& SegStart, const DirectX::XMFLOAT3& SegEnd, float Radius, const DirectX::XMFLOAT3& Color) noexcept
	{
		int index = 0;
		index = AppendRing(OutVertices.data(), index, SegStart, Radius, Color);
		index = AppendRing(OutVertices.data(), index, SegEnd, Radius, Color);

		for (int i = 0; i < SILHOUETTE_LINE_COUNT; ++i)
		{
			const float angle = DirectX::XM_2PI * static_cast<float>(i) / static_cast<float>(SILHOUETTE_LINE_COUNT);
			OutVertices[index++] = { RingPoint(SegStart, Radius, angle), Color };
			OutVertices[index++] = { RingPoint(SegEnd, Radius, angle), Color };
		}

		index = AppendCapArc(OutVertices.data(), index, SegStart, Radius, 0.0f, false, Color);
		index = AppendCapArc(OutVertices.data(), index, SegStart, Radius, DirectX::XM_PIDIV2, false, Color);
		index = AppendCapArc(OutVertices.data(), index, SegStart, Radius, DirectX::XM_PI, false, Color);
		index = AppendCapArc(OutVertices.data(), index, SegStart, Radius, DirectX::XM_PI * 1.5f, false, Color);
		index = AppendCapArc(OutVertices.data(), index, SegEnd, Radius, 0.0f, true, Color);
		index = AppendCapArc(OutVertices.data(), index, SegEnd, Radius, DirectX::XM_PIDIV2, true, Color);
		index = AppendCapArc(OutVertices.data(), index, SegEnd, Radius, DirectX::XM_PI, true, Color);
		index = AppendCapArc(OutVertices.data(), index, SegEnd, Radius, DirectX::XM_PI * 1.5f, true, Color);
	}
}

DebugColliderRenderer::DebugColliderRenderer(DirectX12& Dx12)
	: m_Dx12{ Dx12 }
{
	CreatePipeline();

	const D3D12_HEAP_PROPERTIES upload_heap_properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

	const UINT vertex_buffer_size = static_cast<UINT>(CAPSULE_VERTEX_COUNT * sizeof(LineVertex));
	const D3D12_RESOURCE_DESC vertex_buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(vertex_buffer_size);
	MyAssert::IsFailed(_T("DebugColliderRendererの頂点バッファ作成"), &ID3D12Device::CreateCommittedResource,
		m_Dx12.GetDevice(), &upload_heap_properties, D3D12_HEAP_FLAG_NONE, &vertex_buffer_desc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(m_pVertexBuffer.ReleaseAndGetAddressOf()));
	MyAssert::IsFailed(_T("DebugColliderRendererの頂点バッファをマップ"), &ID3D12Resource::Map,
		m_pVertexBuffer.Get(), 0, nullptr, &m_pMappedVertexBuffer);

	m_VertexBufferView.BufferLocation = m_pVertexBuffer->GetGPUVirtualAddress();
	m_VertexBufferView.SizeInBytes    = vertex_buffer_size;
	m_VertexBufferView.StrideInBytes  = sizeof(LineVertex);

	const UINT constant_buffer_size = (sizeof(DirectX::XMMATRIX) + 255u) & ~255u;
	const D3D12_RESOURCE_DESC constant_buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(constant_buffer_size);
	MyAssert::IsFailed(_T("DebugColliderRendererの定数バッファ作成"), &ID3D12Device::CreateCommittedResource,
		m_Dx12.GetDevice(), &upload_heap_properties, D3D12_HEAP_FLAG_NONE, &constant_buffer_desc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(m_pConstantBuffer.ReleaseAndGetAddressOf()));
	MyAssert::IsFailed(_T("DebugColliderRendererの定数バッファをマップ"), &ID3D12Resource::Map,
		m_pConstantBuffer.Get(), 0, nullptr, reinterpret_cast<void**>(&m_pMappedConstantBuffer));
	if (m_pMappedConstantBuffer != nullptr) {
		*m_pMappedConstantBuffer = DirectX::XMMatrixIdentity();
	}
}

DebugColliderRenderer::~DebugColliderRenderer()
{
	if (m_pMappedConstantBuffer != nullptr && m_pConstantBuffer != nullptr) {
		m_pConstantBuffer->Unmap(0, nullptr);
		m_pMappedConstantBuffer = nullptr;
	}
	if (m_pMappedVertexBuffer != nullptr && m_pVertexBuffer != nullptr) {
		m_pVertexBuffer->Unmap(0, nullptr);
		m_pMappedVertexBuffer = nullptr;
	}
}

void DebugColliderRenderer::CreatePipeline()
{
	// 頂点フォーマット(POSITION+COLOR)がDebugGridと同一のため、シェーダーはGridVertex/GridPixelを共用する.
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
	MyAssert::IsFailed(_T("DebugColliderRendererのルートシグネチャをシリアライズ"), &D3D12SerializeRootSignature,
		&root_signature_desc, D3D_ROOT_SIGNATURE_VERSION_1, root_signature_blob.ReleaseAndGetAddressOf(),
		root_signature_error_blob.ReleaseAndGetAddressOf());
	MyAssert::IsFailed(_T("DebugColliderRendererのルートシグネチャ作成"), &ID3D12Device::CreateRootSignature,
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
	// デプステストを無効化(コライダーはモデル内部にあるため、有効だとメッシュに隠れて大部分が見えなくなる).
	pipeline_desc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	pipeline_desc.DepthStencilState.DepthEnable = FALSE;
	pipeline_desc.InputLayout = { input_layout, _countof(input_layout) };
	pipeline_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
	pipeline_desc.NumRenderTargets = 1;
	pipeline_desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM; // MmdlRenderer(実際に使われる描画パイプライン)の設定に合わせる(SRGB指定は不一致でクラッシュの原因になった).
	pipeline_desc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	pipeline_desc.SampleDesc.Count = 1;
	MyAssert::IsFailed(_T("DebugColliderRendererのパイプライン作成"), &ID3D12Device::CreateGraphicsPipelineState,
		m_Dx12.GetDevice(), &pipeline_desc, IID_PPV_ARGS(m_pPipelineState.ReleaseAndGetAddressOf()));
}

void DebugColliderRenderer::DrawCapsule(const DirectX::XMFLOAT3& SegStart, const DirectX::XMFLOAT3& SegEnd, float Radius, const DirectX::XMFLOAT3& Color)
{
	CameraManager* p_camera_manager = ServiceLocator::Get<CameraManager>();
	CameraBase* p_active_camera = p_camera_manager != nullptr ? p_camera_manager->GetActive() : nullptr;
	if (p_active_camera == nullptr || m_pMappedConstantBuffer == nullptr || m_pMappedVertexBuffer == nullptr) {
		return;
	}

	std::array<LineVertex, CAPSULE_VERTEX_COUNT> vertices{};
	BuildCapsuleVertices(vertices, SegStart, SegEnd, Radius, Color);
	std::memcpy(m_pMappedVertexBuffer, vertices.data(), vertices.size() * sizeof(LineVertex));

	*m_pMappedConstantBuffer = p_active_camera->GetViewProjMatrix();

	ID3D12GraphicsCommandList* p_command_list = m_Dx12.GetCommandList().Get();
	p_command_list->SetGraphicsRootSignature(m_pRootSignature.Get());
	p_command_list->SetPipelineState(m_pPipelineState.Get());
	p_command_list->IASetVertexBuffers(0, 1, &m_VertexBufferView);
	p_command_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
	p_command_list->SetGraphicsRootConstantBufferView(0, m_pConstantBuffer->GetGPUVirtualAddress());
	p_command_list->DrawInstanced(CAPSULE_VERTEX_COUNT, 1, 0, 0);
}
