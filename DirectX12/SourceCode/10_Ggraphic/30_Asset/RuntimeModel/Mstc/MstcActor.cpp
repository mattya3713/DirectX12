#include "MstcActor.h"

#include <cstring>
#include <stdexcept>

#include "MstcRenderer.h"
#include "10_Ggraphic/10_Device/DirectX/DirectX12.h"
#include "10_Ggraphic/30_Asset/RuntimeFormat/RuntimeFormat.h"
#include "10_Ggraphic/30_Asset/RuntimeFormat/RuntimeFormatIO.h"
#include "..\\..\\..\\Data\\Library\\DirectXTex\\Common\\d3dx12.h"

namespace {

	// マテリアル定数(b2. HLSL側のMaterialとレイアウトを合わせる).
	struct MstcMaterialConstantBuffer
	{
		DirectX::XMFLOAT4 Diffuse;             // rgb=拡散色, a=アルファ.
		DirectX::XMFLOAT4 SpecularAmount;      // rgb=鏡面反射色, a=鏡面反射強度.
		DirectX::XMFLOAT4 AmbientUseNormalMap; // rgb=環境光色, a=法線マップ使用フラグ.
	};

	constexpr UINT TransformCBAlignedSize = 256; // ワールド行列1つ分(CBVは256アライン).

} // namespace

MstcActor::MstcActor(const char* FilePath, MstcRenderer& Renderer)
	: MstcActor(std::filesystem::path{ FilePath }, Renderer)
{
}

MstcActor::MstcActor(const std::filesystem::path& FilePath, MstcRenderer& Renderer)
	: m_Dx12    { Renderer.m_Dx12 }
	, m_Renderer{ Renderer }
	, m_FilePath{ FilePath }
{
	CreateResources();
}

MstcActor::~MstcActor()
{
	if (m_pMappedTransformCB && m_pTransformConstantBuffer) {
		m_pTransformConstantBuffer->Unmap(0, nullptr);
		m_pMappedTransformCB = nullptr;
	}
}

MyComPtr<ID3D12Resource> MstcActor::LoadTexture(const std::string& Path)
{
	if (Path.empty()) { return m_Renderer.GetWhiteTex(); }

	MyComPtr<ID3D12Resource> texture = m_Dx12.GetTextureByPath(Path.c_str());
	// 読み込み失敗時(ファイル欠損等)も白テクスチャへフォールバックする.
	return texture ? texture : m_Renderer.GetWhiteTex();
}

void MstcActor::CreateResources()
{
	RuntimeFormat::MstcData mstc{};
	if (!RuntimeFormatIO::ReadMstc(m_FilePath, mstc))
	{
		throw std::runtime_error("MSTCの読み込みに失敗しました: " + m_FilePath.string());
	}
	if (mstc.Vertices.empty() || mstc.Indices.empty())
	{
		throw std::runtime_error("MSTCのメッシュ構成が空です: " + m_FilePath.string());
	}

	ID3D12Device* const p_device = m_Dx12.GetDevice().Get();

	const UINT vertex_buffer_size = static_cast<UINT>(mstc.Vertices.size() * sizeof(RuntimeFormat::StaticVertex));
	const UINT index_buffer_size  = static_cast<UINT>(mstc.Indices.size() * sizeof(std::uint16_t));

	const auto upload_heap_prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD); // Transform/Material CB(永続マップ用)が使う.
	const auto default_heap_prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	// ===== 頂点バッファ(既定ヒープを作成し、UploadBufferSync()で同期アップロード) =====
	// UploadBufferSync()はGPUの完了をフェンス待機してから戻るため、アップロード用の中間バッファを
	// GPUがまだ読んでいる最中に破棄してしまう競合が起きない(過去にここで実際に起きていたバグの修正).
	// StateAfterへの遷移バリアも内部で行うため、VB/IBがCOPY_DESTのまま使われることも無い.
	{
		const auto buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(vertex_buffer_size);
		MyAssert::IsFailed(_T("MstcActor: 頂点バッファの作成"),
			&ID3D12Device::CreateCommittedResource, p_device,
			&default_heap_prop, D3D12_HEAP_FLAG_NONE, &buffer_desc,
			D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
			IID_PPV_ARGS(m_pVertexBuffer.ReleaseAndGetAddressOf()));

		m_Dx12.UploadBufferSync(m_pVertexBuffer.Get(), mstc.Vertices.data(), vertex_buffer_size,
			D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	}

	// ===== インデックスバッファ(既定ヒープを作成し、UploadBufferSync()で同期アップロード) =====
	{
		const auto buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(index_buffer_size);
		MyAssert::IsFailed(_T("MstcActor: インデックスバッファの作成"),
			&ID3D12Device::CreateCommittedResource, p_device,
			&default_heap_prop, D3D12_HEAP_FLAG_NONE, &buffer_desc,
			D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
			IID_PPV_ARGS(m_pIndexBuffer.ReleaseAndGetAddressOf()));

		m_Dx12.UploadBufferSync(m_pIndexBuffer.Get(), mstc.Indices.data(), index_buffer_size,
			D3D12_RESOURCE_STATE_INDEX_BUFFER);
	}

	m_IndexCount = static_cast<std::uint32_t>(mstc.Indices.size());

	m_VertexBufferView.BufferLocation = m_pVertexBuffer->GetGPUVirtualAddress();
	m_VertexBufferView.SizeInBytes    = vertex_buffer_size;
	m_VertexBufferView.StrideInBytes  = sizeof(RuntimeFormat::StaticVertex);

	m_IndexBufferView.BufferLocation = m_pIndexBuffer->GetGPUVirtualAddress();
	m_IndexBufferView.SizeInBytes    = index_buffer_size;
	m_IndexBufferView.Format         = DXGI_FORMAT_R16_UINT;

	// ===== マテリアル読み込み(無し・読込失敗時はデフォルト値で続行) =====
	RuntimeFormat::MmatData material{};
	bool use_normal_map = false;
	if (!mstc.MaterialPath.empty())
	{
		// マテリアルはmsknと同じ規約(MSTCの親ディレクトリの兄弟"mmat"ディレクトリ)で
		// 解決し、見つからない場合はMSTCと同じディレクトリへフォールバックする.
		const std::filesystem::path shared_material_path =
			m_FilePath.parent_path().parent_path() / "mmat" / mstc.MaterialPath;
		const std::filesystem::path local_material_path =
			m_FilePath.parent_path() / mstc.MaterialPath;
		if (!RuntimeFormatIO::ReadMmat(shared_material_path, material))
		{
			RuntimeFormatIO::ReadMmat(local_material_path, material);
		}
		use_normal_map = !material.NormalMapTexturePath.empty();
	}

	// ===== Transform Constant Buffer (b1. 永続マップで即座に書き換え可能にする) =====
	{
		const auto heap_prop = upload_heap_prop;
		const auto desc = CD3DX12_RESOURCE_DESC::Buffer(TransformCBAlignedSize);

		MyAssert::IsFailed(_T("MstcActor: Transform Constant Bufferの作成"),
			&ID3D12Device::CreateCommittedResource, p_device,
			&heap_prop, D3D12_HEAP_FLAG_NONE, &desc,
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
			IID_PPV_ARGS(m_pTransformConstantBuffer.ReleaseAndGetAddressOf()));
		MyAssert::IsFailed(_T("MstcActor: Transform Constant Bufferをマップ"),
			&ID3D12Resource::Map, m_pTransformConstantBuffer.Get(),
			0, nullptr, reinterpret_cast<void**>(&m_pMappedTransformCB));

		m_pMappedTransformCB->World = DirectX::XMMatrixIdentity();
	}

	// ===== Material Constant Buffer (b2) =====
	{
		const auto heap_prop = upload_heap_prop;
		const auto desc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(MstcMaterialConstantBuffer));

		MyAssert::IsFailed(_T("MstcActor: Material Constant Bufferの作成"),
			&ID3D12Device::CreateCommittedResource, p_device,
			&heap_prop, D3D12_HEAP_FLAG_NONE, &desc,
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
			IID_PPV_ARGS(m_pMaterialConstantBuffer.ReleaseAndGetAddressOf()));

		MstcMaterialConstantBuffer* p_mapped_material = nullptr;
		MyAssert::IsFailed(_T("MstcActor: Material Constant Bufferをマップ"),
			&ID3D12Resource::Map, m_pMaterialConstantBuffer.Get(),
			0, nullptr, reinterpret_cast<void**>(&p_mapped_material));
		p_mapped_material->Diffuse             = material.Diffuse;
		p_mapped_material->SpecularAmount      = DirectX::XMFLOAT4{ material.Specular.x, material.Specular.y, material.Specular.z, material.SpecularPower };
		p_mapped_material->AmbientUseNormalMap = DirectX::XMFLOAT4{ material.Ambient.x, material.Ambient.y, material.Ambient.z, use_normal_map ? 1.0f : 0.0f };
		m_pMaterialConstantBuffer->Unmap(0, nullptr);

		m_MaterialCBVDesc.BufferLocation = m_pMaterialConstantBuffer->GetGPUVirtualAddress();
		m_MaterialCBVDesc.SizeInBytes    = static_cast<UINT>((sizeof(MstcMaterialConstantBuffer) + 255) & ~255);
	}

	// ===== CBV/SRV/UAVディスクリプタヒープ(b0/b1/b2/t0/t1の5個) =====
	{
		D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
		heap_desc.NumDescriptors = 5;
		heap_desc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		heap_desc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		heap_desc.NodeMask       = 0;
		MyAssert::IsFailed(_T("MstcActor: CBV/SRV/UAVディスクリプタヒープの作成"),
			&ID3D12Device::CreateDescriptorHeap, p_device,
			&heap_desc, IID_PPV_ARGS(m_pCbvSrvUavHeap.ReleaseAndGetAddressOf()));

		m_CbvSrvUavDescriptorSize = p_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		CD3DX12_CPU_DESCRIPTOR_HANDLE current_cpu_handle(m_pCbvSrvUavHeap->GetCPUDescriptorHandleForHeapStart());

		// --- b0(Scene CBV. DirectX12が持つ共有バッファを参照するだけ) ---
		if (ID3D12Resource* p_scene_cb = m_Dx12.GetSceneConstantBuffer())
		{
			D3D12_CONSTANT_BUFFER_VIEW_DESC scene_cbv_desc = {};
			scene_cbv_desc.BufferLocation = p_scene_cb->GetGPUVirtualAddress();
			scene_cbv_desc.SizeInBytes    = static_cast<UINT>(p_scene_cb->GetDesc().Width);
			p_device->CreateConstantBufferView(&scene_cbv_desc, current_cpu_handle);
		}
		current_cpu_handle.Offset(m_CbvSrvUavDescriptorSize);

		// --- b1(Transform CBV) ---
		D3D12_CONSTANT_BUFFER_VIEW_DESC transform_cbv_desc = {};
		transform_cbv_desc.BufferLocation = m_pTransformConstantBuffer->GetGPUVirtualAddress();
		transform_cbv_desc.SizeInBytes    = TransformCBAlignedSize;
		p_device->CreateConstantBufferView(&transform_cbv_desc, current_cpu_handle);
		current_cpu_handle.Offset(m_CbvSrvUavDescriptorSize);

		// --- b2(Material CBV) ---
		p_device->CreateConstantBufferView(&m_MaterialCBVDesc, current_cpu_handle);
		current_cpu_handle.Offset(m_CbvSrvUavDescriptorSize);

		// --- t0(ベースカラー) / t1(オブジェクト空間法線マップ. 無ければ白=法線Z+方向) ---
		D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
		srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srv_desc.ViewDimension           = D3D12_SRV_DIMENSION_TEXTURE2D;
		srv_desc.Texture2D.MipLevels     = 1;

		m_pBaseTexture = LoadTexture(material.BaseColorTexturePath);
		srv_desc.Format              = m_pBaseTexture->GetDesc().Format;
		srv_desc.Texture2D.MipLevels = m_pBaseTexture->GetDesc().MipLevels;
		p_device->CreateShaderResourceView(m_pBaseTexture.Get(), &srv_desc, current_cpu_handle);
		current_cpu_handle.Offset(m_CbvSrvUavDescriptorSize);

		m_pNormalMapTexture = LoadTexture(use_normal_map ? material.NormalMapTexturePath : std::string());
		srv_desc.Format              = m_pNormalMapTexture->GetDesc().Format;
		srv_desc.Texture2D.MipLevels = m_pNormalMapTexture->GetDesc().MipLevels;
		p_device->CreateShaderResourceView(m_pNormalMapTexture.Get(), &srv_desc, current_cpu_handle);
	}
}

void MstcActor::Draw()
{
	auto command_list = m_Dx12.GetCommandList();

	command_list->IASetVertexBuffers(0, 1, &m_VertexBufferView);
	command_list->IASetIndexBuffer(&m_IndexBufferView);
	command_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	ID3D12DescriptorHeap* pp_heaps[] = { m_pCbvSrvUavHeap.Get() };
	command_list->SetDescriptorHeaps(_countof(pp_heaps), pp_heaps);

	const D3D12_GPU_DESCRIPTOR_HANDLE heap_start = m_pCbvSrvUavHeap->GetGPUDescriptorHandleForHeapStart();

	command_list->SetGraphicsRootDescriptorTable(0, heap_start); // b0(Scene).

	D3D12_GPU_DESCRIPTOR_HANDLE transform_handle = heap_start;
	transform_handle.ptr += m_CbvSrvUavDescriptorSize;
	command_list->SetGraphicsRootDescriptorTable(1, transform_handle); // b1(Transform).

	D3D12_GPU_DESCRIPTOR_HANDLE material_handle = transform_handle;
	material_handle.ptr += m_CbvSrvUavDescriptorSize;
	command_list->SetGraphicsRootDescriptorTable(2, material_handle); // b2+t0/t1(Material).

	command_list->DrawIndexedInstanced(m_IndexCount, 1, 0, 0, 0);
}
