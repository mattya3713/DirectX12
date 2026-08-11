#include "XActor.h"

#include "PMX/PMXRenderer.h"
#include "DirectX/DirectX12.h"
#include "Model/XParser.h"
#include "..\\..\\..\\Data\\Library\\DirectXTex\\Common\\d3dx12.h"

XActor::XActor(const char* FilePath, PMXRenderer& Renderer)
	: m_Renderer { Renderer }
	, m_Dx12     { Renderer.m_pDx12 }
{
	try {
		XParser parser;
		parser.Load(FilePath, m_ModelData);

		CreateResources();
	}
	catch (const std::runtime_error& Msg) {
		std::wstring w_str = MyString::StringToWString(Msg.what());
		_ASSERT_EXPR(false, w_str.c_str());
	}
}

XActor::~XActor()
{
	if (m_pMappedTransformCB && m_pTransformConstantBuffer) {
		m_pTransformConstantBuffer->Unmap(0, nullptr);
		m_pMappedTransformCB = nullptr;
	}
}

void XActor::Draw()
{
	auto command_list = m_Dx12.GetCommandList();
	const UINT descriptor_size = m_CbvSrvUavDescriptorSize;

	command_list->IASetVertexBuffers(0, 1, &m_VertexBufferView);
	command_list->IASetIndexBuffer(&m_IndexBufferView);
	command_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	ID3D12DescriptorHeap* pp_heaps[] = { m_pCbvSrvUavHeap.Get() };
	command_list->SetDescriptorHeaps(_countof(pp_heaps), pp_heaps);

	D3D12_GPU_DESCRIPTOR_HANDLE scene_cbv_handle = m_pCbvSrvUavHeap->GetGPUDescriptorHandleForHeapStart();
	command_list->SetGraphicsRootDescriptorTable(RP_SCENE_CBV, scene_cbv_handle);

	D3D12_GPU_DESCRIPTOR_HANDLE transform_cbv_handle = m_pCbvSrvUavHeap->GetGPUDescriptorHandleForHeapStart();
	transform_cbv_handle.ptr += descriptor_size;
	command_list->SetGraphicsRootDescriptorTable(RP_TRANSFORM_CBV, transform_cbv_handle);

	D3D12_GPU_DESCRIPTOR_HANDLE bone_srv_handle = m_pCbvSrvUavHeap->GetGPUDescriptorHandleForHeapStart();
	bone_srv_handle.ptr += descriptor_size;                                                          // Scene CBV分.
	bone_srv_handle.ptr += descriptor_size;                                                          // Transform CBV分.
	bone_srv_handle.ptr += static_cast<UINT64>(m_ModelData.Materials.size()) * 4 * descriptor_size; // 全マテリアルセット分.
	command_list->SetGraphicsRootDescriptorTable(RP_BONE_SRV, bone_srv_handle);

	unsigned int index_offset = 0;
	for (size_t i = 0; i < m_ModelData.Materials.size(); ++i)
	{
		const unsigned int num_face_indices = m_ModelData.Materials[i].NumFaceCount;

		D3D12_GPU_DESCRIPTOR_HANDLE material_table_handle = m_pCbvSrvUavHeap->GetGPUDescriptorHandleForHeapStart();
		material_table_handle.ptr += descriptor_size; // Scene CBV分.
		material_table_handle.ptr += descriptor_size; // Transform CBV分.
		material_table_handle.ptr += static_cast<UINT64>(i) * 4 * descriptor_size;
		command_list->SetGraphicsRootDescriptorTable(RP_MATERIAL_TABLE_CBV_SRV, material_table_handle);

		command_list->DrawIndexedInstanced(num_face_indices, 1, index_offset, 0, 0);
		index_offset += num_face_indices;
	}
}

void XActor::CreateResources()
{
	D3D12_HEAP_PROPERTIES upload_heap_properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

	// ===== 頂点バッファ =====
	D3D12_RESOURCE_DESC vertex_buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(m_ModelData.Vertices.size() * Model::GPU_VERTEX_SIZE);
	MyAssert::IsFailed(_T("XActor: 頂点バッファの作成"), &ID3D12Device::CreateCommittedResource, m_Dx12.GetDevice(),
		&upload_heap_properties, D3D12_HEAP_FLAG_NONE, &vertex_buffer_desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(m_pVertexBuffer.ReleaseAndGetAddressOf()));

	Model::Vertex* p_mapped_vertex = nullptr;
	MyAssert::IsFailed(_T("XActor: 頂点バッファをマップ"), &ID3D12Resource::Map, m_pVertexBuffer.Get(),
		0, nullptr, (void**)&p_mapped_vertex);
	std::copy(m_ModelData.Vertices.begin(), m_ModelData.Vertices.end(), p_mapped_vertex);
	m_pVertexBuffer->Unmap(0, nullptr);

	m_VertexBufferView.BufferLocation = m_pVertexBuffer->GetGPUVirtualAddress();
	m_VertexBufferView.SizeInBytes    = static_cast<UINT>(m_ModelData.Vertices.size()) * Model::GPU_VERTEX_SIZE;
	m_VertexBufferView.StrideInBytes  = Model::GPU_VERTEX_SIZE;

	// ===== インデックスバッファ =====
	D3D12_RESOURCE_DESC index_buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(static_cast<UINT64>(m_ModelData.Indices.size()) * Model::GPU_INDEX_SIZE);
	MyAssert::IsFailed(_T("XActor: インデックスバッファの作成"), &ID3D12Device::CreateCommittedResource, m_Dx12.GetDevice(),
		&upload_heap_properties, D3D12_HEAP_FLAG_NONE, &index_buffer_desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(m_pIndexBuffer.ReleaseAndGetAddressOf()));

	uint32_t* p_mapped_index = nullptr;
	MyAssert::IsFailed(_T("XActor: インデックスバッファをマップ"), &ID3D12Resource::Map, m_pIndexBuffer.Get(),
		0, nullptr, (void**)&p_mapped_index);
	std::copy(m_ModelData.Indices.begin(), m_ModelData.Indices.end(), p_mapped_index);
	m_pIndexBuffer->Unmap(0, nullptr);

	m_IndexBufferView.BufferLocation = m_pIndexBuffer->GetGPUVirtualAddress();
	m_IndexBufferView.Format         = DXGI_FORMAT_R32_UINT;
	m_IndexBufferView.SizeInBytes    = static_cast<UINT>(m_ModelData.Indices.size()) * Model::GPU_INDEX_SIZE;

	// ===== Transform Constant Buffer (b1) =====
	const UINT transform_cbv_size_aligned = (sizeof(PMX::TransformConstantBuffer) + 255) & ~255;
	D3D12_RESOURCE_DESC transform_buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(transform_cbv_size_aligned);
	MyAssert::IsFailed(_T("XActor: Transform Constant Bufferの作成"), &ID3D12Device::CreateCommittedResource, m_Dx12.GetDevice(),
		&upload_heap_properties, D3D12_HEAP_FLAG_NONE, &transform_buffer_desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(m_pTransformConstantBuffer.ReleaseAndGetAddressOf()));
	MyAssert::IsFailed(_T("XActor: Transform Constant Bufferをマップ"), &ID3D12Resource::Map, m_pTransformConstantBuffer.Get(),
		0, nullptr, (void**)&m_pMappedTransformCB);

	// ボーン無しのため、常にダミーの1要素(単位行列)を指すBoneCount=1にしておく
	// (全頂点のBoneWeightsは0なのでシェーダー側では実際には参照されない).
	m_pMappedTransformCB->World     = DirectX::XMMatrixIdentity();
	m_pMappedTransformCB->BoneCount = 1;

	// ===== ダミーのBone StructuredBuffer (t3、単位行列1要素) =====
	D3D12_RESOURCE_DESC bone_buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(DirectX::XMMATRIX));
	MyAssert::IsFailed(_T("XActor: ダミーBone StructuredBufferの作成"), &ID3D12Device::CreateCommittedResource, m_Dx12.GetDevice(),
		&upload_heap_properties, D3D12_HEAP_FLAG_NONE, &bone_buffer_desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(m_pDummyBoneBuffer.ReleaseAndGetAddressOf()));

	DirectX::XMMATRIX* p_mapped_bone = nullptr;
	MyAssert::IsFailed(_T("XActor: ダミーBone StructuredBufferをマップ"), &ID3D12Resource::Map, m_pDummyBoneBuffer.Get(),
		0, nullptr, (void**)&p_mapped_bone);
	*p_mapped_bone = DirectX::XMMatrixIdentity();
	m_pDummyBoneBuffer->Unmap(0, nullptr);

	// ===== CBV/SRV/UAV ディスクリプタヒープ =====
	UINT total_descriptors = 0;
	total_descriptors += 1; // RP_SCENE_CBV (b0)
	total_descriptors += 1; // RP_TRANSFORM_CBV (b1)
	total_descriptors += (1 + 3) * static_cast<UINT>(m_ModelData.Materials.size()); // マテリアルごとの (CBV + BaseTex + ToonTex + SphTex)
	total_descriptors += 1; // RP_BONE_SRV (t3)

	D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
	heap_desc.NumDescriptors = total_descriptors;
	heap_desc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heap_desc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heap_desc.NodeMask       = 0;
	MyAssert::IsFailed(_T("XActor: CBV/SRV/UAV ディスクリプタヒープの作成"), &ID3D12Device::CreateDescriptorHeap, m_Dx12.GetDevice(),
		&heap_desc, IID_PPV_ARGS(m_pCbvSrvUavHeap.ReleaseAndGetAddressOf()));

	m_CbvSrvUavDescriptorSize = m_Dx12.GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_CPU_DESCRIPTOR_HANDLE current_cpu_handle = m_pCbvSrvUavHeap->GetCPUDescriptorHandleForHeapStart();
	D3D12_GPU_DESCRIPTOR_HANDLE current_gpu_handle = m_pCbvSrvUavHeap->GetGPUDescriptorHandleForHeapStart();

	// --- RP_SCENE_CBV (b0、DirectX12が持つ共有バッファを参照するだけ) ---
	if (ID3D12Resource* p_scene_cb = m_Dx12.GetSceneConstantBuffer())
	{
		D3D12_CONSTANT_BUFFER_VIEW_DESC scene_cbv_desc = {};
		scene_cbv_desc.BufferLocation = p_scene_cb->GetGPUVirtualAddress();
		scene_cbv_desc.SizeInBytes    = static_cast<UINT>(p_scene_cb->GetDesc().Width);
		m_Dx12.GetDevice()->CreateConstantBufferView(&scene_cbv_desc, current_cpu_handle);
	}
	current_cpu_handle.ptr += m_CbvSrvUavDescriptorSize;
	current_gpu_handle.ptr += m_CbvSrvUavDescriptorSize;

	// --- RP_TRANSFORM_CBV (b1) ---
	D3D12_CONSTANT_BUFFER_VIEW_DESC transform_cbv_desc = {};
	transform_cbv_desc.BufferLocation = m_pTransformConstantBuffer->GetGPUVirtualAddress();
	transform_cbv_desc.SizeInBytes    = transform_cbv_size_aligned;
	m_Dx12.GetDevice()->CreateConstantBufferView(&transform_cbv_desc, current_cpu_handle);
	current_cpu_handle.ptr += m_CbvSrvUavDescriptorSize;
	current_gpu_handle.ptr += m_CbvSrvUavDescriptorSize;

	// --- マテリアルごとの (CBV + BaseTex SRV + ToonTex SRV + SphTex SRV) ---
	const int material_buffer_size_aligned = (Model::GPU_MATERIAL_SIZE + 255) & ~255;
	const UINT material_upload_buffer_size = static_cast<UINT64>(m_ModelData.Materials.size()) * material_buffer_size_aligned;

	MyComPtr<ID3D12Resource> p_material_upload_buffer;
	D3D12_RESOURCE_DESC material_upload_buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(material_upload_buffer_size);
	MyAssert::IsFailed(_T("XActor: マテリアルアップロードバッファの作成"), &ID3D12Device::CreateCommittedResource, m_Dx12.GetDevice(),
		&upload_heap_properties, D3D12_HEAP_FLAG_NONE, &material_upload_buffer_desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(p_material_upload_buffer.ReleaseAndGetAddressOf()));

	char* p_mapped_material_upload = nullptr;
	MyAssert::IsFailed(_T("XActor: マテリアルアップロードバッファをマップ"), &ID3D12Resource::Map, p_material_upload_buffer.Get(),
		0, nullptr, (void**)&p_mapped_material_upload);

	m_pTextureResource.resize(m_ModelData.Materials.size());

	for (size_t i = 0; i < m_ModelData.Materials.size(); ++i)
	{
		const Model::Material& material = m_ModelData.Materials[i];

		Model::MaterialForHLSL gpu_material{};
		gpu_material.Diffuse       = material.Diffuse;
		gpu_material.Specular      = material.Specular;
		gpu_material.SpecularPower = material.SpecularPower;
		gpu_material.Ambient       = material.Ambient;
		gpu_material.UseSphereMap  = 0.0f; // .xファイルにスフィアマップの概念は無い.
		std::memcpy(p_mapped_material_upload + i * material_buffer_size_aligned, &gpu_material, Model::GPU_MATERIAL_SIZE);

		m_pTextureResource[i] = LoadTexture(material.Textures.BaseTexture.string());

		// --- マテリアルCBV (b2) ---
		D3D12_CONSTANT_BUFFER_VIEW_DESC material_cbv_desc = {};
		material_cbv_desc.BufferLocation = p_material_upload_buffer->GetGPUVirtualAddress() + static_cast<UINT64>(i) * material_buffer_size_aligned;
		material_cbv_desc.SizeInBytes    = material_buffer_size_aligned;
		m_Dx12.GetDevice()->CreateConstantBufferView(&material_cbv_desc, current_cpu_handle);
		current_cpu_handle.ptr += m_CbvSrvUavDescriptorSize;
		current_gpu_handle.ptr += m_CbvSrvUavDescriptorSize;

		D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
		srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srv_desc.ViewDimension           = D3D12_SRV_DIMENSION_TEXTURE2D;
		srv_desc.Texture2D.MipLevels     = 1;

		// --- ベーステクスチャSRV (t0) ---
		if (ID3D12Resource* p_base_tex = m_pTextureResource[i].Get())
		{
			srv_desc.Format               = p_base_tex->GetDesc().Format;
			srv_desc.Texture2D.MipLevels  = p_base_tex->GetDesc().MipLevels;
			m_Dx12.GetDevice()->CreateShaderResourceView(p_base_tex, &srv_desc, current_cpu_handle);
		}
		current_cpu_handle.ptr += m_CbvSrvUavDescriptorSize;
		current_gpu_handle.ptr += m_CbvSrvUavDescriptorSize;

		// --- トゥーンテクスチャSRV (t1、.xには無いのでレンダラーの黒テクスチャで代用) ---
		if (ID3D12Resource* p_toon_tex = m_Renderer.GetBlackTex().Get())
		{
			srv_desc.Format               = p_toon_tex->GetDesc().Format;
			srv_desc.Texture2D.MipLevels  = p_toon_tex->GetDesc().MipLevels;
			m_Dx12.GetDevice()->CreateShaderResourceView(p_toon_tex, &srv_desc, current_cpu_handle);
		}
		current_cpu_handle.ptr += m_CbvSrvUavDescriptorSize;
		current_gpu_handle.ptr += m_CbvSrvUavDescriptorSize;

		// --- スフィアテクスチャSRV (t2、.xには無いので白テクスチャ(乗算の無効値)で代用) ---
		if (ID3D12Resource* p_sph_tex = m_Renderer.GetWhiteTex().Get())
		{
			srv_desc.Format               = p_sph_tex->GetDesc().Format;
			srv_desc.Texture2D.MipLevels  = p_sph_tex->GetDesc().MipLevels;
			m_Dx12.GetDevice()->CreateShaderResourceView(p_sph_tex, &srv_desc, current_cpu_handle);
		}
		current_cpu_handle.ptr += m_CbvSrvUavDescriptorSize;
		current_gpu_handle.ptr += m_CbvSrvUavDescriptorSize;
	}
	p_material_upload_buffer->Unmap(0, nullptr);

	// --- RP_BONE_SRV (t3、ダミーの1要素StructuredBuffer) ---
	D3D12_SHADER_RESOURCE_VIEW_DESC bone_srv_desc = {};
	bone_srv_desc.Shader4ComponentMapping  = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	bone_srv_desc.Format                   = DXGI_FORMAT_UNKNOWN;
	bone_srv_desc.ViewDimension             = D3D12_SRV_DIMENSION_BUFFER;
	bone_srv_desc.Buffer.FirstElement       = 0;
	bone_srv_desc.Buffer.NumElements        = 1;
	bone_srv_desc.Buffer.StructureByteStride = sizeof(DirectX::XMMATRIX);
	bone_srv_desc.Buffer.Flags              = D3D12_BUFFER_SRV_FLAG_NONE;
	m_Dx12.GetDevice()->CreateShaderResourceView(m_pDummyBoneBuffer.Get(), &bone_srv_desc, current_cpu_handle);
}

MyComPtr<ID3D12Resource> XActor::LoadTexture(const std::string& Path)
{
	if (Path.empty()) { return m_Renderer.GetWhiteTex(); }

	MyComPtr<ID3D12Resource> texture = m_Dx12.GetTextureByPath(Path.c_str());
	// 読み込み失敗(ファイル欠損等)時もnullptrのまま返さず、白テクスチャへフォールバックする.
	return texture ? texture : m_Renderer.GetWhiteTex();
}
