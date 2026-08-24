#include "MmdlResource.h"
#include "10_Ggraphic/30_Asset/RuntimeFormat/RuntimeFormatIO.h"
#include <algorithm>
#include <DirectXMath.h>

MmdlResource::MmdlResource(const std::filesystem::path& FilePath)
	: m_FilePath { FilePath }
{
}

void MmdlResource::StoreCpuData(Model::ModelData ModelData, XSkeleton::SkeletalData Skeleton,
	float LocalHeight, std::vector<RuntimeFormat::SkinSubmeshRole> SubmeshRoles,
	float FrontCompositeOpacity, float FrontCompositeMaxDistance)
{
	if (HasCpuData()) { return; }
	m_ModelData = std::move(ModelData);
	m_Skeleton = std::move(Skeleton);
	m_LocalHeight = LocalHeight;
	m_SkinSubmeshRoles = std::move(SubmeshRoles);
	m_FrontCompositeOpacity = FrontCompositeOpacity;
	m_FrontCompositeMaxDistance = FrontCompositeMaxDistance;
}

void MmdlResource::StoreGpuResources(MyComPtr<ID3D12Resource> VertexBuffer, MyComPtr<ID3D12Resource> IndexBuffer,
	MyComPtr<ID3D12Resource> MaterialBuffer, std::vector<MyComPtr<ID3D12Resource>> BaseTextureResources,
	std::vector<MyComPtr<ID3D12Resource>> ToonTextureResources,
	std::vector<MyComPtr<ID3D12Resource>> SphereTextureResources)
{
	if (m_pVertexBuffer || m_pIndexBuffer || m_pMaterialBuffer) { return; }
	m_pVertexBuffer = std::move(VertexBuffer);
	m_pIndexBuffer = std::move(IndexBuffer);
	m_pMaterialBuffer = std::move(MaterialBuffer);
	m_pBaseTextureResources = std::move(BaseTextureResources);
	m_pToonTextureResources = std::move(ToonTextureResources);
	m_pSphereTextureResources = std::move(SphereTextureResources);
}

void MmdlResource::LoadFromFiles()
{
	RuntimeFormat::MsknData mskn{};
	if (!RuntimeFormatIO::ReadMskn(m_FilePath, mskn))
	{
		throw std::runtime_error("MSKNの読み込みに失敗しました: " + m_FilePath.string());
	}
	if (mskn.Vertices.empty() || mskn.Indices.empty() || mskn.Bones.empty() || mskn.Submeshes.empty())
	{
		throw std::runtime_error("MSKNのメッシュ構成が空です: " + m_FilePath.string());
	}
	for (std::size_t bone_index = 0; bone_index < mskn.Bones.size(); ++bone_index)
	{
		const std::int32_t parent_index = mskn.Bones[bone_index].ParentIndex;
		if (parent_index != -1 && (parent_index < 0 || static_cast<std::size_t>(parent_index) >= bone_index))
		{
			throw std::runtime_error("MSKNのボーン親順序が不正です: " + m_FilePath.string());
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
			throw std::runtime_error("MSKNが範囲外の頂点インデックスを含みます: " + m_FilePath.string());
		}
	}
	for (const RuntimeFormat::SkinVertex& vertex : mskn.Vertices)
	{
		for (std::size_t influence = 0; influence < 4; ++influence)
		{
			if (static_cast<std::size_t>(vertex.BoneIndices[influence]) >= mskn.SkinSlots.size())
			{
				throw std::runtime_error("MSKNが範囲外のSkinSlotを参照しています: " + m_FilePath.string());
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

	const std::filesystem::path material_directory = m_FilePath.parent_path().parent_path() / "mmat";
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

	const std::filesystem::path clip_directory = m_FilePath.parent_path().parent_path() / "mstc";
	if (std::filesystem::exists(clip_directory))
	{
		const std::string model_prefix = m_FilePath.stem().string() + "__";
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
