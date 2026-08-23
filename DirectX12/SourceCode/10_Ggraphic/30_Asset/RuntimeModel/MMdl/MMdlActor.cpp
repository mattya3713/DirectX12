#include "MmdlActor.h"

#include <algorithm>
#include <cmath>
#include <cfloat>

#include "10_Ggraphic/30_Asset/RuntimeModel/MMdl/MmdlRenderer.h"
#include "10_Device/DirectX/DirectX12.h"
#include "10_Ggraphic/30_Asset/RuntimeFormat/RuntimeFormatIO.h"
#include "00_Game/00_GameLoop/Time/Time.h"
#include "..\\..\\..\\Data\\Library\\DirectXTex\\Common\\d3dx12.h"

namespace {

	// 指定時刻の前後のキーを探し、区間内を線形補間する(範囲外は端のキーを使う).
	DirectX::XMVECTOR InterpolateVector3Keys(
		const std::vector<XSkeleton::TimedKey<DirectX::XMFLOAT3>>& Keys, float Time, DirectX::XMVECTOR DefaultValue)
	{
		if (Keys.empty()) { return DefaultValue; }
		if (Time <= static_cast<float>(Keys.front().Time)) { return DirectX::XMLoadFloat3(&Keys.front().Value); }
		if (Time >= static_cast<float>(Keys.back().Time))  { return DirectX::XMLoadFloat3(&Keys.back().Value); }

		for (size_t i = 0; i + 1 < Keys.size(); ++i)
		{
			const float t0 = static_cast<float>(Keys[i].Time);
			const float t1 = static_cast<float>(Keys[i + 1].Time);
			if (Time < t0 || Time > t1) { continue; }

			const float t = (t1 > t0) ? (Time - t0) / (t1 - t0) : 0.0f;
			return DirectX::XMVectorLerp(DirectX::XMLoadFloat3(&Keys[i].Value), DirectX::XMLoadFloat3(&Keys[i + 1].Value), t);
		}
		return DefaultValue;
	}

	// 指定時刻の前後のキーを探し、区間内をSlerpする(範囲外は端のキーを使う).
	DirectX::XMVECTOR InterpolateQuaternionKeys(
		const std::vector<XSkeleton::TimedKey<DirectX::XMFLOAT4>>& Keys, float Time, DirectX::XMVECTOR DefaultValue)
	{
		if (Keys.empty()) { return DefaultValue; }
		if (Time <= static_cast<float>(Keys.front().Time)) { return DirectX::XMLoadFloat4(&Keys.front().Value); }
		if (Time >= static_cast<float>(Keys.back().Time))  { return DirectX::XMLoadFloat4(&Keys.back().Value); }

		for (size_t i = 0; i + 1 < Keys.size(); ++i)
		{
			const float t0 = static_cast<float>(Keys[i].Time);
			const float t1 = static_cast<float>(Keys[i + 1].Time);
			if (Time < t0 || Time > t1) { continue; }

			const float t = (t1 > t0) ? (Time - t0) / (t1 - t0) : 0.0f;
			return DirectX::XMQuaternionSlerp(DirectX::XMLoadFloat4(&Keys[i].Value), DirectX::XMLoadFloat4(&Keys[i + 1].Value), t);
		}
		return DefaultValue;
	}

} // namespace

MmdlActor::MmdlActor(const char* FilePath, MmdlRenderer& Renderer)
	: MmdlActor(std::filesystem::path{ FilePath }, Renderer)
{
}

MmdlActor::MmdlActor(const std::filesystem::path& FilePath, MmdlRenderer& Renderer)
	: m_Renderer { Renderer }
	, m_Dx12     { Renderer.m_pDx12 }
	, m_spResource { std::make_shared<MmdlResource>(FilePath) }
{
	LoadRuntimeModel(FilePath);
	m_spResource->StoreCpuData(m_ModelData, m_Skeleton, m_LocalHeight, m_SkinSubmeshRoles,
		m_FrontCompositeOpacity, m_FrontCompositeMaxDistance);
	CreateResources();
}

MmdlActor::MmdlActor(std::shared_ptr<MmdlResource> Resource, MmdlRenderer& Renderer)
	: m_Renderer { Renderer }
	, m_Dx12 { Renderer.m_pDx12 }
	, m_spResource { std::move(Resource) }
{
	if (!m_spResource) { throw std::invalid_argument("MmdlResourceがnullです"); }
	if (m_spResource->HasCpuData())
	{
		m_ModelData = m_spResource->GetModelData();
		m_Skeleton = m_spResource->GetSkeleton();
		m_LocalHeight = m_spResource->GetLocalHeight();
		m_SkinSubmeshRoles = m_spResource->GetSubmeshRoles();
		m_FrontCompositeOpacity = m_spResource->GetFrontCompositeOpacity();
		m_FrontCompositeMaxDistance = m_spResource->GetFrontCompositeMaxDistance();
	}
	else
	{
		LoadRuntimeModel(m_spResource->GetFilePath());
		m_spResource->StoreCpuData(m_ModelData, m_Skeleton, m_LocalHeight, m_SkinSubmeshRoles,
			m_FrontCompositeOpacity, m_FrontCompositeMaxDistance);
	}
	CreateResources();
}

void MmdlActor::LoadRuntimeModel(const std::filesystem::path& FilePath)
{
	RuntimeFormat::MsknData mskn{};
	if (!RuntimeFormatIO::ReadMskn(FilePath, mskn))
	{
		throw std::runtime_error("MSKNの読み込みに失敗しました: " + FilePath.string());
	}
	if (mskn.Vertices.empty() || mskn.Indices.empty() || mskn.Bones.empty() || mskn.Submeshes.empty())
	{
		throw std::runtime_error("MSKNのメッシュ構成が空です: " + FilePath.string());
	}
	for (std::size_t bone_index = 0; bone_index < mskn.Bones.size(); ++bone_index)
	{
		const std::int32_t parent_index = mskn.Bones[bone_index].ParentIndex;
		if (parent_index != -1 && (parent_index < 0 || static_cast<std::size_t>(parent_index) >= bone_index))
		{
			throw std::runtime_error("MSKNのボーン親順序が不正です: " + FilePath.string());
		}
	}
	float min_y = mskn.Vertices.front().Position.y;
	float max_y = min_y;
	for (const RuntimeFormat::SkinVertex& vertex : mskn.Vertices)
	{
		min_y = std::min(min_y, vertex.Position.y);
		max_y = std::max(max_y, vertex.Position.y);
	}
	m_LocalHeight = max_y - min_y;
	for (const std::uint32_t index : mskn.Indices)
	{
		if (index >= mskn.Vertices.size())
		{
			throw std::runtime_error("MSKNが範囲外の頂点インデックスを含みます: " + FilePath.string());
		}
	}
	for (const RuntimeFormat::SkinVertex& vertex : mskn.Vertices)
	{
		for (std::size_t influence = 0; influence < 4; ++influence)
		{
			if (static_cast<std::size_t>(vertex.BoneIndices[influence]) >= mskn.SkinSlots.size())
			{
				throw std::runtime_error("MSKNが範囲外のSkinSlotを参照しています: " + FilePath.string());
			}
		}
	}
	m_ModelData.Vertices.resize(mskn.Vertices.size());
	for (std::size_t i = 0; i < mskn.Vertices.size(); ++i)
	{
		const RuntimeFormat::SkinVertex& source = mskn.Vertices[i];
		Model::Vertex& destination = m_ModelData.Vertices[i];
		destination.Position = source.Position;
		destination.Normal = source.Normal;
		destination.UV = source.UV;
		for (std::size_t influence = 0; influence < 4; ++influence)
		{
			destination.BoneIndices[influence] = source.BoneIndices[influence];
			destination.BoneWeights[influence] = source.BoneWeights[influence];
		}
	}
	m_ModelData.Indices.assign(mskn.Indices.begin(), mskn.Indices.end());

	for (const RuntimeFormat::SkinBone& source : mskn.Bones)
	{
		Model::Bone bone{};
		bone.Name = source.Name;
		bone.ParentBoneIndex = source.ParentIndex < 0 ? Model::Bone::NoParentIndex : static_cast<std::uint32_t>(source.ParentIndex);
		bone.Position = source.BindPosition;
		m_ModelData.Bones.push_back(std::move(bone));
		XSkeleton::Bone skeleton_bone{};
		skeleton_bone.Name = source.Name;
		skeleton_bone.ParentIndex = source.ParentIndex;
		DirectX::XMStoreFloat4x4(&skeleton_bone.LocalBindMatrix,
			DirectX::XMMatrixAffineTransformation(DirectX::XMLoadFloat3(&source.BindScale), DirectX::XMVectorZero(),
				DirectX::XMLoadFloat4(&source.BindRotation), DirectX::XMLoadFloat3(&source.BindPosition)));
		m_Skeleton.Bones.push_back(std::move(skeleton_bone));
	}
	for (const RuntimeFormat::SkinSlot& source : mskn.SkinSlots)
	{
		XSkeleton::SkinSlot slot{};
		slot.BoneIndex = source.BoneIndex;
		slot.OffsetMatrix = source.OffsetMatrix;
		m_Skeleton.SkinSlots.push_back(std::move(slot));
	}

	const std::filesystem::path material_directory = FilePath.parent_path().parent_path() / "mmat";
	m_FrontCompositeOpacity = mskn.FrontCompositeOpacity;
	m_FrontCompositeMaxDistance = mskn.FrontCompositeMaxDistance;
	for (const RuntimeFormat::SkinSubmesh& source : mskn.Submeshes)
	{
		m_SkinSubmeshRoles.push_back(source.Role);
		const std::filesystem::path material_path = material_directory / source.MaterialPath;
		RuntimeFormat::MmatData runtime_material{};
		if (!RuntimeFormatIO::ReadMmat(material_path, runtime_material))
		{
			throw std::runtime_error("MMATの読み込みに失敗しました: " + material_path.string());
		}
		Model::Material material{};
		material.Diffuse = runtime_material.Diffuse;
		material.Specular = runtime_material.Specular;
		material.SpecularPower = runtime_material.SpecularPower;
		material.Ambient = runtime_material.Ambient;
		material.NumFaceCount = source.IndexCount;
		material.Textures.BaseTexture = runtime_material.BaseColorTexturePath;
		material.Textures.ToonTexture = runtime_material.ToonTexturePath;
		material.Textures.UseToonMap = false;
		material.Textures.SphereTexture = runtime_material.SphereTexturePath;
		material.Textures.UseSphereMap = runtime_material.UseSphereMap;
		m_ModelData.Materials.push_back(std::move(material));
	}

	const std::filesystem::path clip_directory = FilePath.parent_path().parent_path() / "mstc";
	if (std::filesystem::exists(clip_directory))
	{
		const std::string model_prefix = FilePath.stem().string() + "__";
		for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(clip_directory))
		{
			const std::string file_stem = entry.path().stem().string();
			if (!entry.is_regular_file() || entry.path().extension() != ".mclp" || file_stem.rfind(model_prefix, 0) != 0) { continue; }
			RuntimeFormat::MclpData runtime_clip{};
			if (!RuntimeFormatIO::ReadMclp(entry.path(), runtime_clip))
			{
				throw std::runtime_error("MCLPの読み込みに失敗しました: " + entry.path().string());
			}
		XSkeleton::AnimationClip clip{};
		clip.Name = runtime_clip.ClipName;
		if (runtime_clip.Duration < 0.0f) { continue; }
		clip.MaxTime = static_cast<std::uint32_t>(runtime_clip.Duration * 4800.0f);
		bool is_compatible = true;
		for (const RuntimeFormat::MclpBoneTrack& source : runtime_clip.Tracks)
		{
			if (source.BoneIndex >= mskn.Bones.size())
			{
				is_compatible = false;
				break;
			}
				XSkeleton::BoneAnimation animation{};
				animation.BoneIndex = static_cast<std::int32_t>(source.BoneIndex);
				for (const RuntimeFormat::Keyframe& key : source.Keyframes)
				{
					const std::uint32_t time = static_cast<std::uint32_t>(key.Time * 4800.0f);
					animation.PositionKeys.push_back({ time, key.Position });
					animation.RotationKeys.push_back({ time, key.Rotation });
					animation.ScaleKeys.push_back({ time, key.Scale });
				}
				clip.BoneAnimations.push_back(std::move(animation));
			}
			m_Skeleton.Clips.push_back(std::move(clip));
			if (!is_compatible) { m_Skeleton.Clips.pop_back(); }
		}
	}
	// クリップが無いモデルはバインドポーズで描画する(VMDなしPMXを許可する).
	m_Skeleton.TicksPerSecond = 4800;
}

MmdlActor::~MmdlActor()
{
	if (m_pMappedTransformCB && m_pTransformConstantBuffer) {
		m_pTransformConstantBuffer->Unmap(0, nullptr);
		m_pMappedTransformCB = nullptr;
	}
	if (m_pMappedBoneTransforms && m_pBoneTransformBuffer) {
		m_pBoneTransformBuffer->Unmap(0, nullptr);
		m_pMappedBoneTransforms = nullptr;
	}
}

void MmdlActor::PlayAnimation(const std::string& ClipName)
{
	for (size_t i = 0; i < m_Skeleton.Clips.size(); ++i)
	{
		if (m_Skeleton.Clips[i].Name == ClipName)
		{
			m_CurrentClipIndex = static_cast<int>(i);
			m_CurrentTime = 0.0f;
			m_IsExternallyDriven = false;
			return;
		}
	}
}

void MmdlActor::Update()
{
	if (m_CurrentClipIndex >= 0 && static_cast<size_t>(m_CurrentClipIndex) < m_Skeleton.Clips.size())
	{
		const float max_time = static_cast<float>(m_Skeleton.Clips[m_CurrentClipIndex].MaxTime);

		// キーフレームの時刻はAnimTicksPerSecond単位(秒ではない)なので、実時間から変換する.
		if (!m_IsExternallyDriven)
		{
			m_CurrentTime += GameTime::GetDeltaTime() * static_cast<float>(m_Skeleton.TicksPerSecond) * m_PlaybackSpeed;
			if (max_time > 0.0f) { m_CurrentTime = std::fmod(m_CurrentTime, max_time); } // ループ再生.
		}
	}

	UpdateBoneMatrices();
}

void MmdlActor::UpdateBoneMatrices()
{
	if (!m_pMappedBoneTransforms || m_Skeleton.Bones.empty()) { return; }

	const XSkeleton::AnimationClip* p_clip =
		(m_CurrentClipIndex >= 0 && static_cast<size_t>(m_CurrentClipIndex) < m_Skeleton.Clips.size())
		? &m_Skeleton.Clips[m_CurrentClipIndex] : nullptr;

	// ボーンIndex→そのボーンを動かすBoneAnimationへの索引(再生中クリップがそのボーンに
	// キーを持たない場合はnullptrのまま=バインドポーズを使う).
	std::vector<const XSkeleton::BoneAnimation*> anim_by_bone(m_Skeleton.Bones.size(), nullptr);
	if (p_clip)
	{
		for (const XSkeleton::BoneAnimation& anim : p_clip->BoneAnimations)
		{
			if (anim.BoneIndex >= 0 && static_cast<size_t>(anim.BoneIndex) < anim_by_bone.size())
			{
				anim_by_bone[anim.BoneIndex] = &anim;
			}
		}
	}

	// ボーンは親が必ず自分より前のIndexになるように構築されている(Frame階層を親から子へ
	// 辿りながらpush_backしているため)ので、前から1回なめるだけでワールド変換を計算できる.
	std::vector<DirectX::XMMATRIX> world_transforms(m_Skeleton.Bones.size());
	for (size_t i = 0; i < m_Skeleton.Bones.size(); ++i)
	{
		const XSkeleton::Bone& bone = m_Skeleton.Bones[i];

		DirectX::XMMATRIX local;
		if (const XSkeleton::BoneAnimation* p_anim = anim_by_bone[i])
		{
			const DirectX::XMVECTOR rotation = InterpolateQuaternionKeys(p_anim->RotationKeys, m_CurrentTime, DirectX::XMQuaternionIdentity());
			const DirectX::XMVECTOR scale = InterpolateVector3Keys(p_anim->ScaleKeys, m_CurrentTime, DirectX::XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f));
			const DirectX::XMVECTOR translation = InterpolateVector3Keys(p_anim->PositionKeys, m_CurrentTime, DirectX::XMVectorZero());
			local = DirectX::XMMatrixAffineTransformation(scale, DirectX::XMVectorZero(), rotation, translation);
		}
		else
		{
			local = DirectX::XMLoadFloat4x4(&bone.LocalBindMatrix); // アニメーションキーが無いボーンはバインドポーズのまま.
		}

		world_transforms[i] = (bone.ParentIndex >= 0 && static_cast<size_t>(bone.ParentIndex) < i)
			? DirectX::XMMatrixMultiply(local, world_transforms[bone.ParentIndex])
			: local;
	}

	// GPUへ送るのはボーンではなくSkinSlot単位(SkinWeights.matrixOffsetは実際には
	// (メッシュ,ボーン)の組ごとに値が異なりうるため. 詳細はMMdlSkeletonData.h参照).
	for (size_t i = 0; i < m_Skeleton.SkinSlots.size(); ++i)
	{
		const XSkeleton::SkinSlot& slot = m_Skeleton.SkinSlots[i];
		const DirectX::XMMATRIX offset = DirectX::XMLoadFloat4x4(&slot.OffsetMatrix);
		const DirectX::XMMATRIX world = (slot.BoneIndex >= 0 && static_cast<size_t>(slot.BoneIndex) < world_transforms.size())
			? world_transforms[slot.BoneIndex]
			: DirectX::XMMatrixIdentity();

		// スキニング行列 = オフセット行列(メッシュ座標系→バインド時のボーン座標系) *
		// ボーンの現在のワールド変換(行ベクトル規約のためオフセットを先に掛ける).
		m_pMappedBoneTransforms[i] = DirectX::XMMatrixMultiply(offset, world);
	}
}

void MmdlActor::Draw()
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

	D3D12_GPU_DESCRIPTOR_HANDLE shadow_srv_handle = m_pCbvSrvUavHeap->GetGPUDescriptorHandleForHeapStart();
	shadow_srv_handle.ptr += descriptor_size;                                                        // Scene CBV分.
	shadow_srv_handle.ptr += descriptor_size;                                                        // Transform CBV分.
	shadow_srv_handle.ptr += static_cast<UINT64>(m_ModelData.Materials.size()) * 4 * descriptor_size; // 全マテリアルセット分.
	shadow_srv_handle.ptr += descriptor_size;                                                        // Bone SRV分.
	command_list->SetGraphicsRootDescriptorTable(RP_SHADOW_SRV, shadow_srv_handle);

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

void MmdlActor::CreateResources()
{
	D3D12_HEAP_PROPERTIES upload_heap_properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

	// ===== 頂点バッファ =====
	if (m_spResource->GetVertexBuffer())
	{
		m_pVertexBuffer = m_spResource->GetVertexBuffer();
	}
	else
	{
		D3D12_RESOURCE_DESC vertex_buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(m_ModelData.Vertices.size() * Model::GPU_VERTEX_SIZE);
		MyAssert::IsFailed(_T("MmdlActor: 頂点バッファの作成"), &ID3D12Device::CreateCommittedResource, m_Dx12.GetDevice(),
			&upload_heap_properties, D3D12_HEAP_FLAG_NONE, &vertex_buffer_desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
			IID_PPV_ARGS(m_pVertexBuffer.ReleaseAndGetAddressOf()));

		Model::Vertex* p_mapped_vertex = nullptr;
		MyAssert::IsFailed(_T("MmdlActor: 頂点バッファをマップ"), &ID3D12Resource::Map, m_pVertexBuffer.Get(),
			0, nullptr, (void**)&p_mapped_vertex);
		std::copy(m_ModelData.Vertices.begin(), m_ModelData.Vertices.end(), p_mapped_vertex);
		m_pVertexBuffer->Unmap(0, nullptr);
	}

	m_VertexBufferView.BufferLocation = m_pVertexBuffer->GetGPUVirtualAddress();
	m_VertexBufferView.SizeInBytes    = static_cast<UINT>(m_ModelData.Vertices.size()) * Model::GPU_VERTEX_SIZE;
	m_VertexBufferView.StrideInBytes  = Model::GPU_VERTEX_SIZE;

	// ===== インデックスバッファ =====
	if (m_spResource->GetIndexBuffer())
	{
		m_pIndexBuffer = m_spResource->GetIndexBuffer();
	}
	else
	{
		D3D12_RESOURCE_DESC index_buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(static_cast<UINT64>(m_ModelData.Indices.size()) * Model::GPU_INDEX_SIZE);
		MyAssert::IsFailed(_T("MmdlActor: インデックスバッファの作成"), &ID3D12Device::CreateCommittedResource, m_Dx12.GetDevice(),
			&upload_heap_properties, D3D12_HEAP_FLAG_NONE, &index_buffer_desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
			IID_PPV_ARGS(m_pIndexBuffer.ReleaseAndGetAddressOf()));

		uint32_t* p_mapped_index = nullptr;
		MyAssert::IsFailed(_T("MmdlActor: インデックスバッファをマップ"), &ID3D12Resource::Map, m_pIndexBuffer.Get(),
			0, nullptr, (void**)&p_mapped_index);
		std::copy(m_ModelData.Indices.begin(), m_ModelData.Indices.end(), p_mapped_index);
		m_pIndexBuffer->Unmap(0, nullptr);
	}

	m_IndexBufferView.BufferLocation = m_pIndexBuffer->GetGPUVirtualAddress();
	m_IndexBufferView.Format         = DXGI_FORMAT_R32_UINT;
	m_IndexBufferView.SizeInBytes    = static_cast<UINT>(m_ModelData.Indices.size()) * Model::GPU_INDEX_SIZE;

	// ===== Transform Constant Buffer (b1) =====
	const UINT transform_cbv_size_aligned = (sizeof(PMX::TransformConstantBuffer) + 255) & ~255;
	D3D12_RESOURCE_DESC transform_buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(transform_cbv_size_aligned);
	MyAssert::IsFailed(_T("MmdlActor: Transform Constant Bufferの作成"), &ID3D12Device::CreateCommittedResource, m_Dx12.GetDevice(),
		&upload_heap_properties, D3D12_HEAP_FLAG_NONE, &transform_buffer_desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(m_pTransformConstantBuffer.ReleaseAndGetAddressOf()));
	MyAssert::IsFailed(_T("MmdlActor: Transform Constant Bufferをマップ"), &ID3D12Resource::Map, m_pTransformConstantBuffer.Get(),
		0, nullptr, (void**)&m_pMappedTransformCB);

	// ボーン行列バッファはSkinSlot単位(SkinWeights.matrixOffsetは(メッシュ,ボーン)の組ごとに
	// 異なりうるため、ボーン数ではなくSkinSlot数で確保する. 詳細はMMdlSkeletonData.h参照).
	const UINT bone_count = static_cast<UINT>(std::max<size_t>(m_Skeleton.SkinSlots.size(), 1));
	m_pMappedTransformCB->World     = DirectX::XMMatrixIdentity();
	m_pMappedTransformCB->BoneCount = bone_count;

	// ===== Bone StructuredBuffer (t3、実SkinSlot数ぶん. スキニングされないファイルでも
	// ダミーの単位行列1要素を確保しておく(ルートシグネチャ上、有効なSRVが必要なため)) =====
	D3D12_RESOURCE_DESC bone_buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(static_cast<UINT64>(bone_count) * sizeof(DirectX::XMMATRIX));
	MyAssert::IsFailed(_T("MmdlActor: Bone StructuredBufferの作成"), &ID3D12Device::CreateCommittedResource, m_Dx12.GetDevice(),
		&upload_heap_properties, D3D12_HEAP_FLAG_NONE, &bone_buffer_desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(m_pBoneTransformBuffer.ReleaseAndGetAddressOf()));
	MyAssert::IsFailed(_T("MmdlActor: Bone StructuredBufferをマップ"), &ID3D12Resource::Map, m_pBoneTransformBuffer.Get(),
		0, nullptr, (void**)&m_pMappedBoneTransforms);

	if (m_Skeleton.SkinSlots.empty())
	{
		m_pMappedBoneTransforms[0] = DirectX::XMMatrixIdentity();
	}
	else
	{
		UpdateBoneMatrices(); // バインドポーズ(未再生状態)を初期値として書き込む.
	}

	// ===== CBV/SRV/UAV ディスクリプタヒープ =====
	UINT total_descriptors = 0;
	total_descriptors += 1; // RP_SCENE_CBV (b0)
	total_descriptors += 1; // RP_TRANSFORM_CBV (b1)
	total_descriptors += (1 + 3) * static_cast<UINT>(m_ModelData.Materials.size()); // マテリアルごとの (CBV + BaseTex + ToonTex + SphTex)
	total_descriptors += 1; // RP_BONE_SRV (t3)
	total_descriptors += 1; // RP_SHADOW_SRV (ピクセルシェーダーt3. MmdlRenderer共有のシャドウマップ).

	D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
	heap_desc.NumDescriptors = total_descriptors;
	heap_desc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heap_desc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heap_desc.NodeMask       = 0;
	MyAssert::IsFailed(_T("MmdlActor: CBV/SRV/UAV ディスクリプタヒープの作成"), &ID3D12Device::CreateDescriptorHeap, m_Dx12.GetDevice(),
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
	const UINT material_upload_buffer_size = static_cast<UINT>(static_cast<UINT64>(m_ModelData.Materials.size()) * material_buffer_size_aligned);

	MyComPtr<ID3D12Resource> p_material_upload_buffer = m_spResource->GetMaterialBuffer();
	D3D12_RESOURCE_DESC material_upload_buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(material_upload_buffer_size);
	if (!p_material_upload_buffer)
	{
		MyAssert::IsFailed(_T("MmdlActor: マテリアルアップロードバッファの作成"), &ID3D12Device::CreateCommittedResource, m_Dx12.GetDevice(),
			&upload_heap_properties, D3D12_HEAP_FLAG_NONE, &material_upload_buffer_desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
			IID_PPV_ARGS(p_material_upload_buffer.ReleaseAndGetAddressOf()));
	}

	char* p_mapped_material_upload = nullptr;
	if (!m_spResource->GetMaterialBuffer())
	{
		MyAssert::IsFailed(_T("MmdlActor: マテリアルアップロードバッファをマップ"), &ID3D12Resource::Map, p_material_upload_buffer.Get(),
			0, nullptr, (void**)&p_mapped_material_upload);
	}

	m_pTextureResource = m_spResource->GetBaseTextureResources();
	m_pToonResource = m_spResource->GetToonTextureResources();
	m_pSphereResource = m_spResource->GetSphereTextureResources();
	if (m_pTextureResource.empty()) { m_pTextureResource.resize(m_ModelData.Materials.size()); }
	if (m_pToonResource.empty()) { m_pToonResource.resize(m_ModelData.Materials.size()); }
	if (m_pSphereResource.empty()) { m_pSphereResource.resize(m_ModelData.Materials.size()); }

	for (size_t i = 0; i < m_ModelData.Materials.size(); ++i)
	{
		const Model::Material& material = m_ModelData.Materials[i];

		Model::MaterialForHLSL gpu_material{};
		gpu_material.Diffuse       = material.Diffuse;
		gpu_material.Specular      = material.Specular;
		gpu_material.SpecularPower = material.SpecularPower;
		gpu_material.Ambient       = material.Ambient;
		gpu_material.UseSphereMap  = material.Textures.UseSphereMap ? 1.0f : 0.0f;
		gpu_material.UseToonMap    = material.Textures.UseToonMap ? 1.0f : 0.0f;
		if (p_mapped_material_upload)
		{
			std::memcpy(p_mapped_material_upload + i * material_buffer_size_aligned, &gpu_material, Model::GPU_MATERIAL_SIZE);
		}

		if (!m_pTextureResource[i]) { m_pTextureResource[i] = LoadTexture(material.Textures.BaseTexture.string()); }
		if (!m_pToonResource[i])
		{
			m_pToonResource[i] = material.Textures.ToonTexture.empty()
				? m_Renderer.GetWhiteTex() : LoadTexture(material.Textures.ToonTexture.string());
		}
		if (!m_pSphereResource[i])
		{
			m_pSphereResource[i] = material.Textures.SphereTexture.empty()
				? m_Renderer.GetWhiteTex() : LoadTexture(material.Textures.SphereTexture.string());
		}

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

		// --- トゥーンテクスチャSRV (t1) ---
		if (ID3D12Resource* p_toon_tex = m_pToonResource[i].Get())
		{
			srv_desc.Format               = p_toon_tex->GetDesc().Format;
			srv_desc.Texture2D.MipLevels  = p_toon_tex->GetDesc().MipLevels;
			m_Dx12.GetDevice()->CreateShaderResourceView(p_toon_tex, &srv_desc, current_cpu_handle);
		}
		current_cpu_handle.ptr += m_CbvSrvUavDescriptorSize;
		current_gpu_handle.ptr += m_CbvSrvUavDescriptorSize;

		// --- スフィアテクスチャSRV (t2) ---
		if (ID3D12Resource* p_sph_tex = m_pSphereResource[i].Get())
		{
			srv_desc.Format               = p_sph_tex->GetDesc().Format;
			srv_desc.Texture2D.MipLevels  = p_sph_tex->GetDesc().MipLevels;
			m_Dx12.GetDevice()->CreateShaderResourceView(p_sph_tex, &srv_desc, current_cpu_handle);
		}
		current_cpu_handle.ptr += m_CbvSrvUavDescriptorSize;
		current_gpu_handle.ptr += m_CbvSrvUavDescriptorSize;
	}
	if (p_mapped_material_upload) { p_material_upload_buffer->Unmap(0, nullptr); }

	// --- RP_BONE_SRV (t3) ---
	D3D12_SHADER_RESOURCE_VIEW_DESC bone_srv_desc = {};
	bone_srv_desc.Shader4ComponentMapping   = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	bone_srv_desc.Format                    = DXGI_FORMAT_UNKNOWN;
	bone_srv_desc.ViewDimension              = D3D12_SRV_DIMENSION_BUFFER;
	bone_srv_desc.Buffer.FirstElement        = 0;
	bone_srv_desc.Buffer.NumElements         = bone_count;
	bone_srv_desc.Buffer.StructureByteStride = sizeof(DirectX::XMMATRIX);
	bone_srv_desc.Buffer.Flags               = D3D12_BUFFER_SRV_FLAG_NONE;
	m_Dx12.GetDevice()->CreateShaderResourceView(m_pBoneTransformBuffer.Get(), &bone_srv_desc, current_cpu_handle);
	current_cpu_handle.ptr += m_CbvSrvUavDescriptorSize;
	current_gpu_handle.ptr += m_CbvSrvUavDescriptorSize;

	// --- RP_SHADOW_SRV (ピクセルシェーダーt3. MmdlRenderer共有のシャドウ深度バッファを参照するだけ) ---
	if (ID3D12Resource* p_shadow_map = m_Renderer.GetShadowMapResource())
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC shadow_srv_desc = {};
		shadow_srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		shadow_srv_desc.Format                  = DXGI_FORMAT_R32_FLOAT; // 深度リソース(D32_FLOAT)をSRVとして読む際の形式.
		shadow_srv_desc.ViewDimension           = D3D12_SRV_DIMENSION_TEXTURE2D;
		shadow_srv_desc.Texture2D.MipLevels     = 1;
		m_Dx12.GetDevice()->CreateShaderResourceView(p_shadow_map, &shadow_srv_desc, current_cpu_handle);
	}

	// 静的な頂点・インデックス・ベーステクスチャはResourceが所有し、同じResourceを
	// 渡された次のActorでは再生成せず参照カウントだけを共有する.
	if (!m_spResource->GetVertexBuffer())
	{
		m_spResource->StoreGpuResources(std::move(m_pVertexBuffer), std::move(m_pIndexBuffer),
			std::move(p_material_upload_buffer), std::move(m_pTextureResource), std::move(m_pToonResource),
			std::move(m_pSphereResource));
		m_pVertexBuffer = m_spResource->GetVertexBuffer();
		m_pIndexBuffer = m_spResource->GetIndexBuffer();
		m_pTextureResource = m_spResource->GetBaseTextureResources();
		m_pToonResource = m_spResource->GetToonTextureResources();
		m_pSphereResource = m_spResource->GetSphereTextureResources();
	}
}

MyComPtr<ID3D12Resource> MmdlActor::LoadTexture(const std::string& Path)
{
	if (Path.empty()) { return m_Renderer.GetWhiteTex(); }

	MyComPtr<ID3D12Resource> texture = m_Dx12.GetTextureByPath(Path.c_str());
	// 読み込み失敗(ファイル欠損等)時もnullptrのまま返さず、白テクスチャへフォールバックする.
	return texture ? texture : m_Renderer.GetWhiteTex();
}
