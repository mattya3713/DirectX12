#include "RuntimeConverter.h"

#include "RuntimeFormatIO.h"
#include "30_Asset/Parser/PMXParser.h"
#include "30_Asset/Parser/XParser.h"
#include "90_Legacy/PMX/VMD/VMDLoader.h"
#include "json/json.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <fstream>
#include <functional>
#include <map>
#include <sstream>

namespace {

	constexpr std::uint32_t MaximumRuntimeIndex = 65535;

	struct FrontCompositeConfig {
		bool Enabled = false;
		std::vector<std::uint32_t> SourceSubmeshes;
		std::vector<std::uint32_t> RelaxedOccluders;
		float Opacity = 1.0f;
		float MaxDistance = 0.0f;
	};

	bool ReadFrontCompositeConfig(const std::filesystem::path& InputPath, FrontCompositeConfig& OutConfig, std::string& OutError)
	{
		const std::filesystem::path config_path = InputPath.parent_path() / (InputPath.stem().string() + ".mmdl.json");
		if (!std::filesystem::exists(config_path)) { return true; }
		try
		{
			std::ifstream file(config_path);
			nlohmann::json root = nlohmann::json::parse(file);
			if (!root.contains("frontComposite")) { return true; }
			const nlohmann::json& composite = root.at("frontComposite");
			if (!composite.is_object() || !composite.contains("sourceSubmeshes") || !composite.contains("relaxedOccluders") ||
				!composite.at("sourceSubmeshes").is_array() || !composite.at("relaxedOccluders").is_array() ||
				composite.at("sourceSubmeshes").empty() || composite.at("relaxedOccluders").empty())
			{
				OutError = config_path.string() + ": sourceSubmeshesとrelaxedOccludersは空でない整数配列が必要です。"; return false;
			}
			for (const nlohmann::json& value : composite.at("sourceSubmeshes"))
			{
				if (!value.is_number_integer() || value.get<std::int64_t>() < 0 || value.get<std::uint64_t>() > MaximumRuntimeIndex) { OutError = config_path.string() + ": sourceSubmeshesの番号が不正です。"; return false; }
				OutConfig.SourceSubmeshes.push_back(value.get<std::uint32_t>());
			}
			for (const nlohmann::json& value : composite.at("relaxedOccluders"))
			{
				if (!value.is_number_integer() || value.get<std::int64_t>() < 0 || value.get<std::uint64_t>() > MaximumRuntimeIndex) { OutError = config_path.string() + ": relaxedOccludersの番号が不正です。"; return false; }
				OutConfig.RelaxedOccluders.push_back(value.get<std::uint32_t>());
			}
			for (const std::uint32_t source : OutConfig.SourceSubmeshes)
				if (std::find(OutConfig.RelaxedOccluders.begin(), OutConfig.RelaxedOccluders.end(), source) != OutConfig.RelaxedOccluders.end()) { OutError = config_path.string() + ": 同じサブメッシュを両方の役割に指定できません。"; return false; }
			if (!composite.contains("opacity") || !composite.contains("maxDistance") || !composite.at("opacity").is_number() || !composite.at("maxDistance").is_number()) { OutError = config_path.string() + ": opacityとmaxDistanceが必要です。"; return false; }
			OutConfig.Opacity = composite.at("opacity").get<float>(); OutConfig.MaxDistance = composite.at("maxDistance").get<float>();
			if (!std::isfinite(OutConfig.Opacity) || OutConfig.Opacity < 0.0f || OutConfig.Opacity > 1.0f || !std::isfinite(OutConfig.MaxDistance) || OutConfig.MaxDistance < 0.0f) { OutError = config_path.string() + ": opacityまたはmaxDistanceの値が不正です。"; return false; }
			OutConfig.Enabled = true; return true;
		}
		catch (const std::exception& exception) { OutError = config_path.string() + ": " + exception.what(); return false; }
	}

	std::string NormalizePath(const std::filesystem::path& Path)
	{
		std::string value = Path.lexically_normal().generic_string();
		std::transform(value.begin(), value.end(), value.begin(), [](unsigned char Character) {
			return static_cast<char>(std::tolower(Character));
		});
		const std::string data_model = "data/model/";
		const std::size_t data_model_position = value.find(data_model);
		if (data_model_position != std::string::npos)
		{
			value = value.substr(data_model_position);
		}
		return value;
	}

	std::uint64_t HashMaterial(const RuntimeFormat::MmatData& Material)
	{
		std::uint64_t hash = 14695981039346656037ULL;
		auto add = [&hash](const void* p_Data, std::size_t Size) {
			const unsigned char* p_Bytes = static_cast<const unsigned char*>(p_Data);
			for (std::size_t i = 0; i < Size; ++i) { hash = (hash ^ p_Bytes[i]) * 1099511628211ULL; }
		};
		add(&Material.Diffuse, sizeof(Material.Diffuse));
		add(&Material.Specular, sizeof(Material.Specular));
		add(&Material.SpecularPower, sizeof(Material.SpecularPower));
		add(&Material.Ambient, sizeof(Material.Ambient));
		const std::uint8_t use_sphere = Material.UseSphereMap ? 1 : 0;
		add(&use_sphere, sizeof(use_sphere));
		const std::uint8_t use_toon = Material.UseToonMap ? 1 : 0;
		add(&use_toon, sizeof(use_toon));
		const std::string paths[] = { NormalizePath(Material.BaseColorTexturePath), NormalizePath(Material.NormalMapTexturePath),
			NormalizePath(Material.ToonTexturePath), NormalizePath(Material.SphereTexturePath) };
		for (const std::string& path : paths) { add(path.data(), path.size()); const char separator = '\0'; add(&separator, sizeof(separator)); }
		return hash;
	}

	bool SameMaterial(const RuntimeFormat::MmatData& Left, const RuntimeFormat::MmatData& Right)
	{
		return std::memcmp(&Left.Diffuse, &Right.Diffuse, sizeof(Left.Diffuse)) == 0 &&
			std::memcmp(&Left.Specular, &Right.Specular, sizeof(Left.Specular)) == 0 &&
			Left.SpecularPower == Right.SpecularPower && std::memcmp(&Left.Ambient, &Right.Ambient, sizeof(Left.Ambient)) == 0 &&
			Left.UseSphereMap == Right.UseSphereMap && NormalizePath(Left.BaseColorTexturePath) == NormalizePath(Right.BaseColorTexturePath) &&
			Left.UseToonMap == Right.UseToonMap &&
			NormalizePath(Left.NormalMapTexturePath) == NormalizePath(Right.NormalMapTexturePath) &&
			NormalizePath(Left.ToonTexturePath) == NormalizePath(Right.ToonTexturePath) &&
			NormalizePath(Left.SphereTexturePath) == NormalizePath(Right.SphereTexturePath);
	}

	bool ValidateMskn(const std::filesystem::path& Path, const RuntimeFormat::MsknData& Expected)
	{
		RuntimeFormat::MsknData actual{};
		return RuntimeFormatIO::ReadMskn(Path, actual) && actual.Vertices.size() == Expected.Vertices.size() &&
			actual.Indices.size() == Expected.Indices.size() && actual.Bones.size() == Expected.Bones.size() &&
			actual.SkinSlots.size() == Expected.SkinSlots.size() && actual.Submeshes.size() == Expected.Submeshes.size();
	}

	bool ValidateMclp(const std::filesystem::path& Path, const RuntimeFormat::MclpData& Expected)
	{
		RuntimeFormat::MclpData actual{};
		return RuntimeFormatIO::ReadMclp(Path, actual) && actual.ClipName == Expected.ClipName &&
			actual.Duration == Expected.Duration && actual.Tracks.size() == Expected.Tracks.size();
	}

	bool WriteFixedString(char* p_Destination, std::size_t Capacity, const std::string& Value)
	{
		if (Value.size() >= Capacity) { return false; }
		std::memset(p_Destination, 0, Capacity);
		std::memcpy(p_Destination, Value.data(), Value.size());
		return true;
	}

	bool WriteMaterial(const std::filesystem::path& Directory, const Model::Material& Source, std::string& OutName)
	{
		RuntimeFormat::MmatData material{};
		material.Diffuse = Source.Diffuse;
		material.Specular = Source.Specular;
		material.SpecularPower = Source.SpecularPower;
		material.Ambient = Source.Ambient;
		material.BaseColorTexturePath = NormalizePath(Source.Textures.BaseTexture);
		material.NormalMapTexturePath = "";
		material.ToonTexturePath = NormalizePath(Source.Textures.ToonTexture);
		material.SphereTexturePath = NormalizePath(Source.Textures.SphereTexture);
		material.UseSphereMap = Source.Textures.UseSphereMap && !material.SphereTexturePath.empty();
		material.UseToonMap = Source.Textures.UseToonMap && !material.ToonTexturePath.empty();

		std::ostringstream name_stream;
		name_stream << std::hex << HashMaterial(material);
		const std::string stem = name_stream.str();
		for (std::uint32_t suffix = 0; suffix < 0xFFFFFFFFU; ++suffix)
		{
			OutName = stem + (suffix == 0 ? "" : "_" + std::to_string(suffix)) + ".mmat";
			const std::filesystem::path path = Directory / OutName;
			RuntimeFormat::MmatData existing{};
			if (std::filesystem::exists(path))
			{
				if (RuntimeFormatIO::ReadMmat(path, existing) && SameMaterial(existing, material)) { return true; }
				continue;
			}
			if (!RuntimeFormatIO::WriteMmat(path, material)) { return false; }
			RuntimeFormat::MmatData written{};
			const bool valid = RuntimeFormatIO::ReadMmat(path, written) && SameMaterial(written, material);
			return valid;
		}
		return false;
	}

	bool BuildMskn(const Model::ModelData& Source, const std::vector<RuntimeFormat::SkinBone>& Bones,
		const std::vector<RuntimeFormat::SkinSlot>& Slots, const std::filesystem::path& Directory,
		RuntimeFormat::MsknData& OutData, const std::vector<DirectX::XMFLOAT4X4>* p_VertexTransforms = nullptr,
		const std::vector<XSkeleton::SkinSlot>* p_SourceSlots = nullptr, const FrontCompositeConfig* p_Composite = nullptr)
	{
		if (Source.Indices.size() > 0xFFFFFFFFULL || Source.Vertices.empty() || Source.Indices.empty() ||
			(p_VertexTransforms != nullptr && p_VertexTransforms->size() != Source.Vertices.size()) ||
			(p_SourceSlots != nullptr && p_SourceSlots->size() != Slots.size())) { return false; }
		OutData.Bones = Bones;
		OutData.SkinSlots = Slots;
		OutData.Vertices.reserve(Source.Vertices.size());
		for (const Model::Vertex& source : Source.Vertices)
		{
			RuntimeFormat::SkinVertex vertex{};
			vertex.Position = source.Position; vertex.Normal = source.Normal; vertex.UV = source.UV;
			if (p_VertexTransforms != nullptr)
			{
				const std::size_t vertex_index = OutData.Vertices.size();
				const DirectX::XMMATRIX transform = DirectX::XMLoadFloat4x4(&(*p_VertexTransforms)[vertex_index]);
				DirectX::XMStoreFloat3(&vertex.Position, DirectX::XMVector3Transform(DirectX::XMLoadFloat3(&vertex.Position), transform));
				DirectX::XMStoreFloat3(&vertex.Normal, DirectX::XMVector3Normalize(DirectX::XMVector3TransformNormal(DirectX::XMLoadFloat3(&vertex.Normal), transform)));
			}
			float total = 0.0f;
			for (std::size_t i = 0; i < 4; ++i)
			{
				if (source.BoneWeights[i] > 0.00001f && (source.BoneIndices[i] >= Slots.size() || source.BoneIndices[i] > MaximumRuntimeIndex)) { return false; }
				vertex.BoneIndices[i] = static_cast<std::uint16_t>(source.BoneIndices[i]);
				vertex.BoneWeights[i] = std::max(source.BoneWeights[i], 0.0f); total += vertex.BoneWeights[i];
			}
			if (total > 0.00001f) { for (float& weight : vertex.BoneWeights) { weight /= total; } }
			OutData.Vertices.push_back(vertex);
		}
		for (std::uint32_t index : Source.Indices) { if (index >= Source.Vertices.size()) { return false; } OutData.Indices.push_back(index); }
		std::size_t index_offset = 0;
		for (const Model::Material& source : Source.Materials)
		{
			std::string material_name;
			if (!WriteMaterial(Directory, source, material_name)) { return false; }
			RuntimeFormat::SkinSubmesh submesh{};
			if (!WriteFixedString(submesh.MaterialPath, sizeof(submesh.MaterialPath), material_name)) { return false; }
			submesh.IndexCount = source.NumFaceCount;
			if (p_Composite != nullptr && p_Composite->Enabled)
			{
				const std::size_t submesh_index = OutData.Submeshes.size();
				if (std::find(p_Composite->SourceSubmeshes.begin(), p_Composite->SourceSubmeshes.end(), submesh_index) != p_Composite->SourceSubmeshes.end()) { submesh.Role = RuntimeFormat::SkinSubmeshRole::Source; }
				else if (std::find(p_Composite->RelaxedOccluders.begin(), p_Composite->RelaxedOccluders.end(), submesh_index) != p_Composite->RelaxedOccluders.end()) { submesh.Role = RuntimeFormat::SkinSubmeshRole::RelaxedOccluder; }
			}
			if (index_offset + submesh.IndexCount > OutData.Indices.size()) { return false; }
			OutData.Submeshes.push_back(submesh); index_offset += submesh.IndexCount;
		}
		if (p_Composite != nullptr && p_Composite->Enabled)
		{
			for (const std::uint32_t index : p_Composite->SourceSubmeshes) if (index >= OutData.Submeshes.size()) return false;
			for (const std::uint32_t index : p_Composite->RelaxedOccluders) if (index >= OutData.Submeshes.size()) return false;
			OutData.FrontCompositeOpacity = p_Composite->Opacity; OutData.FrontCompositeMaxDistance = p_Composite->MaxDistance;
		}
		return index_offset == OutData.Indices.size();
	}

	std::vector<RuntimeFormat::SkinSlot> MakeXSkinSlots(const XSkeleton::SkeletalData& Source, RuntimeConverter::ConversionResult& Result)
	{
		std::vector<RuntimeFormat::SkinSlot> slots;
		slots.reserve(Source.SkinSlots.size());
		for (const XSkeleton::SkinSlot& source : Source.SkinSlots)
		{
			if (source.BoneIndex < 0 || static_cast<std::size_t>(source.BoneIndex) >= Source.Bones.size())
			{
				Result.Error = "XのSkinWeightsが存在しないボーンを参照しています。";
				return {};
			}
			const DirectX::XMMATRIX mesh_transform = DirectX::XMLoadFloat4x4(&source.MeshTransform);
			const DirectX::XMVECTOR determinant = DirectX::XMMatrixDeterminant(mesh_transform);
			if (std::abs(DirectX::XMVectorGetX(determinant)) < 0.000001f)
			{
				Result.Error = "XのMeshTransformMatrixが逆行列を持たないためSkinSlotを再構成できません。";
				return {};
			}
			RuntimeFormat::SkinSlot slot{};
			slot.BoneIndex = source.BoneIndex;
			DirectX::XMStoreFloat4x4(&slot.OffsetMatrix,
				DirectX::XMMatrixMultiply(DirectX::XMMatrixInverse(nullptr, mesh_transform), DirectX::XMLoadFloat4x4(&source.OffsetMatrix)));
			slots.push_back(slot);
		}
		return slots;
	}

	RuntimeFormat::SkinBone MakePmxBone(const Model::ModelData& Source, std::size_t Index)
	{
		RuntimeFormat::SkinBone bone{};
		const Model::Bone& source = Source.Bones[Index];
		WriteFixedString(bone.Name, sizeof(bone.Name), source.Name);
		bone.ParentIndex = source.ParentBoneIndex == Model::Bone::NoParentIndex ? -1 : static_cast<std::int32_t>(source.ParentBoneIndex);
		const DirectX::XMFLOAT3 parent = bone.ParentIndex >= 0 ? Source.Bones[bone.ParentIndex].Position : DirectX::XMFLOAT3{ 0,0,0 };
		bone.BindPosition = { source.Position.x - parent.x, source.Position.y - parent.y, source.Position.z - parent.z };
		bone.BindRotation = { 0,0,0,1 }; bone.BindScale = { 1,1,1 };
		return bone;
	}

	bool BuildPmxOrder(const Model::ModelData& Source, std::vector<std::size_t>& OutOrder, std::vector<std::size_t>& OutRemap,
		RuntimeConverter::ConversionResult& Result)
	{
		std::vector<std::uint8_t> colors(Source.Bones.size(), 0);
		std::function<bool(std::size_t)> visit = [&](std::size_t index) {
			if (colors[index] == 1) { return false; }
			if (colors[index] == 2) { return true; }
			colors[index] = 1;
			const std::uint32_t parent = Source.Bones[index].ParentBoneIndex;
			if (parent != Model::Bone::NoParentIndex)
			{
				if (parent >= Source.Bones.size() || parent == index || !visit(parent)) { return false; }
			}
			colors[index] = 2;
			OutOrder.push_back(index);
			return true;
		};
		for (std::size_t index = 0; index < Source.Bones.size(); ++index)
		{
			if (!visit(index)) { Result.Error = "PMXのボーン階層が循環または不正です。"; return false; }
		}
		OutRemap.resize(Source.Bones.size());
		for (std::size_t new_index = 0; new_index < OutOrder.size(); ++new_index) { OutRemap[OutOrder[new_index]] = new_index; }
		return true;
	}

	DirectX::XMVECTOR InterpolateXVector(const std::vector<XSkeleton::TimedKey<DirectX::XMFLOAT3>>& Keys,
		std::uint32_t Time, DirectX::XMVECTOR BindValue)
	{
		if (Keys.empty()) { return BindValue; }
		if (Time <= Keys.front().Time) { return DirectX::XMLoadFloat3(&Keys.front().Value); }
		if (Time >= Keys.back().Time) { return DirectX::XMLoadFloat3(&Keys.back().Value); }
		for (std::size_t index = 1; index < Keys.size(); ++index)
		{
			if (Time <= Keys[index].Time)
			{
				const std::uint32_t interval = Keys[index].Time - Keys[index - 1].Time;
				const float amount = interval > 0 ? static_cast<float>(Time - Keys[index - 1].Time) /
					static_cast<float>(interval) : 0.0f;
				return DirectX::XMVectorLerp(DirectX::XMLoadFloat3(&Keys[index - 1].Value),
					DirectX::XMLoadFloat3(&Keys[index].Value), amount);
			}
		}
		return DirectX::XMLoadFloat3(&Keys.back().Value);
	}

	DirectX::XMVECTOR InterpolateXQuaternion(const std::vector<XSkeleton::TimedKey<DirectX::XMFLOAT4>>& Keys,
		std::uint32_t Time, DirectX::XMVECTOR BindValue)
	{
		if (Keys.empty()) { return BindValue; }
		if (Time <= Keys.front().Time) { return DirectX::XMLoadFloat4(&Keys.front().Value); }
		if (Time >= Keys.back().Time) { return DirectX::XMLoadFloat4(&Keys.back().Value); }
		for (std::size_t index = 1; index < Keys.size(); ++index)
		{
			if (Time <= Keys[index].Time)
			{
				const std::uint32_t interval = Keys[index].Time - Keys[index - 1].Time;
				const float amount = interval > 0 ? static_cast<float>(Time - Keys[index - 1].Time) /
					static_cast<float>(interval) : 0.0f;
				return DirectX::XMQuaternionSlerp(DirectX::XMLoadFloat4(&Keys[index - 1].Value),
					DirectX::XMLoadFloat4(&Keys[index].Value), amount);
			}
		}
		return DirectX::XMLoadFloat4(&Keys.back().Value);
	}

}

RuntimeConverter::ConversionResult RuntimeConverter::ConvertPmx(const std::filesystem::path& PmxPath, const std::filesystem::path& VmdPath, const std::filesystem::path& OutputDirectory)
{
	ConversionResult result{};
	try {
		std::filesystem::create_directories(OutputDirectory);
		Model::ModelData model{}; PMXParser parser{}; parser.Load(PmxPath.string(), model);
		FrontCompositeConfig composite{};
		if (!ReadFrontCompositeConfig(PmxPath, composite, result.Error)) { return result; }
		if (parser.HasUnsupportedSdef())
		{
			result.Warnings.push_back("PMXにSDEF頂点が含まれるため、C/R0/R1を無視してBDEF2として近似変換します。変形結果は完全一致しません。");
		}
		if (model.Vertices.empty() || model.Indices.empty()) { result.Error = "PMXの頂点またはインデックスが空です。"; return result; }
		if (model.Bones.empty()) { result.Error = "PMXにボーンが存在しません。"; return result; }
		for (Model::Vertex& vertex : model.Vertices)
		{
			for (std::size_t influence = 0; influence < 4; ++influence)
			{
				if (vertex.BoneWeights[influence] > 0.00001f && static_cast<std::size_t>(vertex.BoneIndices[influence]) >= model.Bones.size())
				{
					vertex.BoneIndices[influence] = 0;
					result.Warnings.push_back("PMX頂点が範囲外のボーンを参照したため、ルートボーンへ補正しました。");
				}
			}
		}
		for (Model::Material& material : model.Materials)
		{
			material.Textures.UseToonMap = false;
		}
		std::vector<std::size_t> order;
		std::vector<std::size_t> remap;
		if (!BuildPmxOrder(model, order, remap, result)) { return result; }
		std::vector<RuntimeFormat::SkinBone> bones;
		std::vector<RuntimeFormat::SkinSlot> slots;
		bones.reserve(order.size()); slots.resize(order.size());
		for (std::size_t new_index = 0; new_index < order.size(); ++new_index)
		{
			const std::size_t old_index = order[new_index];
			RuntimeFormat::SkinBone bone = MakePmxBone(model, old_index);
			if (bone.ParentIndex >= 0) { bone.ParentIndex = static_cast<std::int32_t>(remap[bone.ParentIndex]); }
			if (std::memchr(bone.Name, '\0', sizeof(bone.Name)) == nullptr) { result.Error = "PMXのボーン名が固定長上限を超えています。"; return result; }
			bones.push_back(bone);
			slots[new_index].BoneIndex = static_cast<std::int32_t>(new_index);
			const DirectX::XMMATRIX bind_world = DirectX::XMMatrixTranslationFromVector(DirectX::XMLoadFloat3(&model.Bones[old_index].Position));
			DirectX::XMStoreFloat4x4(&slots[new_index].OffsetMatrix, DirectX::XMMatrixInverse(nullptr, bind_world));
		}
		for (Model::Vertex& vertex : model.Vertices)
		{
			for (std::size_t influence = 0; influence < 4; ++influence)
			{
				if (vertex.BoneWeights[influence] > 0.00001f) { vertex.BoneIndices[influence] = static_cast<std::uint32_t>(remap[vertex.BoneIndices[influence]]); }
			}
		}
		RuntimeFormat::MsknData data{};
		const std::filesystem::path mskn_path = OutputDirectory / (PmxPath.stem().string() + ".mskn");
		if (!BuildMskn(model, bones, slots, OutputDirectory, data, nullptr, nullptr, &composite) || !RuntimeFormatIO::WriteMskn(mskn_path, data) || !ValidateMskn(mskn_path, data)) { result.Error = "PMXのMSKN書き出しまたは検証に失敗しました。"; return result; }
		if (!VmdPath.empty()) {
			const VMD::MotionData motion = VMDLoader::Load(VmdPath.string()); RuntimeFormat::MclpData clip{}; clip.ClipName = VmdPath.stem().string();
			for (const auto& pair : motion.BoneKeyFrames)
			{
				const auto it = std::find_if(model.Bones.begin(), model.Bones.end(), [&pair](const Model::Bone& bone) { return bone.Name == pair.first; });
				if (it == model.Bones.end()) { result.Warnings.push_back("VMDの未解決ボーン: " + pair.first); continue; }
				const std::size_t old_index = static_cast<std::size_t>(std::distance(model.Bones.begin(), it));
				RuntimeFormat::MclpBoneTrack track{}; track.BoneIndex = static_cast<std::uint32_t>(remap[old_index]);
				const DirectX::XMFLOAT3 bind_position = bones[track.BoneIndex].BindPosition;
				for (const VMD::BoneFrame& source : pair.second) { RuntimeFormat::Keyframe key{}; key.Time = static_cast<float>(source.FrameNo) / 30.0f; key.Position = { bind_position.x + source.Position.x, bind_position.y + source.Position.y, bind_position.z + source.Position.z }; key.Rotation = { 0,0,0,1 }; key.Scale = { 1,1,1 }; DirectX::XMStoreFloat4(&key.Rotation, DirectX::XMQuaternionNormalize(DirectX::XMLoadFloat4(&source.Rotation))); clip.Duration = std::max(clip.Duration, key.Time); track.Keyframes.push_back(key); }
				std::sort(track.Keyframes.begin(), track.Keyframes.end(), [](const RuntimeFormat::Keyframe& left, const RuntimeFormat::Keyframe& right) { return left.Time < right.Time; }); clip.Tracks.push_back(std::move(track));
			}
			const std::filesystem::path mclp_path = OutputDirectory / (PmxPath.stem().string() + "__" + clip.ClipName + ".mclp");
			if (!RuntimeFormatIO::WriteMclp(mclp_path, clip) || !ValidateMclp(mclp_path, clip)) { result.Error = "VMDのMCLP書き出しまたは検証に失敗しました。"; return result; }
		}
		result.Success = true; return result;
	}
	catch (const std::exception& exception) { result.Error = exception.what(); return result; }
}

RuntimeConverter::ConversionResult RuntimeConverter::ConvertX(const std::filesystem::path& XPath, const std::filesystem::path& OutputDirectory)
{
	ConversionResult result{};
	try {
		std::filesystem::create_directories(OutputDirectory); Model::ModelData model{}; XSkeleton::SkeletalData skeleton{}; XParser parser{}; parser.LoadSkeletal(XPath.string(), model, skeleton);
		FrontCompositeConfig composite{};
		if (!ReadFrontCompositeConfig(XPath, composite, result.Error)) { return result; }
		std::vector<RuntimeFormat::SkinBone> bones;
		for (std::size_t bone_index = 0; bone_index < skeleton.Bones.size(); ++bone_index)
		{
			const XSkeleton::Bone& source = skeleton.Bones[bone_index];
			RuntimeFormat::SkinBone bone{};
			if (!WriteFixedString(bone.Name, sizeof(bone.Name), source.Name)) { result.Error = "Xのボーン名が固定長上限を超えています。"; return result; }
			const bool invalid_parent = source.ParentIndex < -1 ||
				(source.ParentIndex >= 0 && (static_cast<std::size_t>(source.ParentIndex) >= skeleton.Bones.size() ||
					static_cast<std::size_t>(source.ParentIndex) >= bone_index));
			bone.ParentIndex = invalid_parent ? -1 : source.ParentIndex;
			if (invalid_parent) { result.Warnings.push_back("Xの不正な親ボーンをルートとして近似変換しました: " + source.Name); }
			DirectX::XMVECTOR scale{}, rotation{}, translation{};
			if (!DirectX::XMMatrixDecompose(&scale, &rotation, &translation, DirectX::XMLoadFloat4x4(&source.LocalBindMatrix))) { result.Error = "Xのボーン行列を分解できません。"; return result; }
			DirectX::XMStoreFloat3(&bone.BindScale, scale); DirectX::XMStoreFloat4(&bone.BindRotation, DirectX::XMQuaternionNormalize(rotation)); DirectX::XMStoreFloat3(&bone.BindPosition, translation);
			bones.push_back(bone);
		}
		std::vector<RuntimeFormat::SkinSlot> slots = MakeXSkinSlots(skeleton, result); if (!result.Error.empty() && slots.empty() && !skeleton.SkinSlots.empty()) { return result; }
		RuntimeFormat::MsknData data{}; const std::filesystem::path mskn_path = OutputDirectory / (XPath.stem().string() + ".mskn"); if (!BuildMskn(model, bones, slots, OutputDirectory, data, &skeleton.VertexTransforms, &skeleton.SkinSlots, &composite) || !RuntimeFormatIO::WriteMskn(mskn_path, data) || !ValidateMskn(mskn_path, data)) { result.Error = "XのMSKN書き出しまたは検証に失敗しました。"; return result; }
		for (const XSkeleton::AnimationClip& source_clip : skeleton.Clips)
		{
			RuntimeFormat::MclpData clip{}; clip.ClipName = source_clip.Name; clip.Duration = static_cast<float>(source_clip.MaxTime) / std::max(skeleton.TicksPerSecond, 1U);
			for (const XSkeleton::BoneAnimation& source : source_clip.BoneAnimations)
			{
				if (source.BoneIndex < 0 || static_cast<std::size_t>(source.BoneIndex) >= skeleton.Bones.size()) { result.Error = "Xのアニメーションが不正なボーンを参照しています。"; return result; }
				const RuntimeFormat::SkinBone& bind = bones[source.BoneIndex];
				std::map<std::uint32_t, bool> times;
				for (const auto& key : source.PositionKeys) { times[key.Time] = true; }
				for (const auto& key : source.RotationKeys) { times[key.Time] = true; }
				for (const auto& key : source.ScaleKeys) { times[key.Time] = true; }
				RuntimeFormat::MclpBoneTrack track{}; track.BoneIndex = static_cast<std::uint32_t>(source.BoneIndex);
				for (const auto& time : times)
				{
					RuntimeFormat::Keyframe key{}; key.Time = static_cast<float>(time.first) / std::max(skeleton.TicksPerSecond, 1U);
					DirectX::XMStoreFloat3(&key.Position, InterpolateXVector(source.PositionKeys, time.first, DirectX::XMLoadFloat3(&bind.BindPosition)));
					DirectX::XMStoreFloat3(&key.Scale, InterpolateXVector(source.ScaleKeys, time.first, DirectX::XMLoadFloat3(&bind.BindScale)));
					DirectX::XMStoreFloat4(&key.Rotation, DirectX::XMQuaternionNormalize(InterpolateXQuaternion(source.RotationKeys, time.first, DirectX::XMLoadFloat4(&bind.BindRotation))));
					track.Keyframes.push_back(key);
				}
				clip.Tracks.push_back(std::move(track));
			}
			const std::filesystem::path mclp_path = OutputDirectory / (XPath.stem().string() + "__" + clip.ClipName + ".mclp");
			if (!RuntimeFormatIO::WriteMclp(mclp_path, clip) || !ValidateMclp(mclp_path, clip)) { result.Error = "XのMCLP書き出しまたは検証に失敗しました。"; return result; }
		}
		result.Success = true; return result;
	}
	catch (const std::exception& exception) { result.Error = exception.what(); return result; }
}
