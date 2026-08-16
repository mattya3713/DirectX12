#include "RuntimeFormatIO.h"

#include <cstring>
#include <fstream>
#include <limits>
#include <utility>

namespace {

	constexpr std::uint32_t RuntimeFormatVersion = 1;
	constexpr std::uint32_t MmatFormatVersion = 3;
	constexpr std::uint32_t MsknFormatVersion = 4;
	constexpr std::uint32_t MclpFormatVersion = 2;
	constexpr std::size_t MaximumVertexCount = 65536;
	constexpr std::size_t MmatTexturePathCapacity = 128;

	struct LegacyMmatValues {
		DirectX::XMFLOAT4 Diffuse;
		DirectX::XMFLOAT3 Specular;
		float SpecularPower;
		DirectX::XMFLOAT3 Ambient;
		std::uint32_t UseSphereMap;
	};

	// size_tの値がuint32_tで表現可能か確認する.
	bool CanRepresentAsUint32(std::size_t Value)
	{
		return Value <= static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max());
	}

	// 2つの値の加算結果がuint64_tの範囲内か確認する.
	bool CanAdd(std::uint64_t Left, std::uint64_t Right, std::uint64_t& OutValue)
	{
		if (Right > std::numeric_limits<std::uint64_t>::max() - Left)
		{
			return false;
		}
		OutValue = Left + Right;
		return true;
	}

	// 2つの値の乗算結果がuint64_tの範囲内か確認する.
	bool CanMultiply(std::uint64_t Left, std::uint64_t Right, std::uint64_t& OutValue)
	{
		if (Left != 0 && Right > std::numeric_limits<std::uint64_t>::max() / Left)
		{
			return false;
		}
		OutValue = Left * Right;
		return true;
	}

	// バッファー末尾に指定したバイト列を追加する.
	bool AppendBytes(std::vector<char>& Buffer, const void* p_Data, std::size_t Size)
	{
		if (Size > std::numeric_limits<std::size_t>::max() - Buffer.size())
		{
			return false;
		}
		const std::size_t offset = Buffer.size();
		Buffer.resize(offset + Size);
		if (Size != 0)
		{
			std::memcpy(Buffer.data() + offset, p_Data, Size);
		}
		return true;
	}

	// PODオブジェクトをバッファー末尾に追加する.
	template <typename T>
	bool AppendObject(std::vector<char>& Buffer, const T& Object)
	{
		return AppendBytes(Buffer, &Object, sizeof(T));
	}

	// バッファーから指定したバイト列を読み出す.
	bool ReadBytes(const std::vector<char>& Buffer, std::size_t& Offset, void* p_Destination, std::size_t Size)
	{
		if (Offset > Buffer.size() || Size > Buffer.size() - Offset)
		{
			return false;
		}
		if (Size != 0)
		{
			std::memcpy(p_Destination, Buffer.data() + Offset, Size);
		}
		Offset += Size;
		return true;
	}

	// バッファーからPODオブジェクトを読み出す.
	template <typename T>
	bool ReadObject(const std::vector<char>& Buffer, std::size_t& Offset, T& Object)
	{
		return ReadBytes(Buffer, Offset, &Object, sizeof(T));
	}

	// ヘッダーを読み込み、残りをペイロードとして返す.
	bool OpenAndReadPayload(const std::filesystem::path& FilePath, void* p_Header, std::size_t HeaderSize, std::vector<char>& OutPayload)
	{
		std::ifstream file(FilePath, std::ios::binary | std::ios::ate);
		if (!file.is_open())
		{
			return false;
		}

		const std::streampos end_position = file.tellg();
		if (end_position < static_cast<std::streamoff>(HeaderSize))
		{
			return false;
		}
		const std::uint64_t payload_size = static_cast<std::uint64_t>(end_position) - HeaderSize;
		if (payload_size > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max()))
		{
			return false;
		}
		file.seekg(0, std::ios::beg);
		file.read(static_cast<char*>(p_Header), static_cast<std::streamsize>(HeaderSize));
		if (!file || payload_size > static_cast<std::uint64_t>(std::numeric_limits<std::streamsize>::max()))
		{
			return false;
		}

		OutPayload.resize(static_cast<std::size_t>(payload_size));
		if (!OutPayload.empty())
		{
			file.read(OutPayload.data(), static_cast<std::streamsize>(OutPayload.size()));
			if (!file)
			{
				return false;
			}
		}
		return true;
	}

	// ヘッダーとペイロードを1つのバイナリファイルとして書き込む.
	template <typename T>
	bool WriteFile(const std::filesystem::path& FilePath, const T& Header, const std::vector<char>& Payload)
	{
		std::ofstream file(FilePath, std::ios::binary | std::ios::trunc);
		if (!file.is_open())
		{
			return false;
		}
		file.write(reinterpret_cast<const char*>(&Header), static_cast<std::streamsize>(sizeof(T)));
		if (!Payload.empty())
		{
			file.write(Payload.data(), static_cast<std::streamsize>(Payload.size()));
		}
		return file.good();
	}

	// ファイルのマジックとバージョンが対応する形式か確認する.
	bool HasMagicAndVersion(const char* Magic, const char* ExpectedMagic, std::uint32_t Version, std::uint32_t ExpectedVersion = RuntimeFormatVersion)
	{
		return std::memcmp(Magic, ExpectedMagic, 4) == 0 && Version == ExpectedVersion;
	}

	// 固定長パスへ格納できる文字列か確認する.
	bool CanStoreMmatPath(const std::string& Path)
	{
		return Path.size() < MmatTexturePathCapacity && Path.find('\0') == std::string::npos;
	}

	// 固定長パスの終端と格納上限を検証する.
	bool IsValidMmatPath(const char* p_Path, std::uint32_t Length)
	{
		return Length < MmatTexturePathCapacity && p_Path[Length] == '\0' &&
			std::memchr(p_Path, '\0', MmatTexturePathCapacity) == p_Path + Length;
	}

}

bool RuntimeFormatIO::WriteMmat(const std::filesystem::path& FilePath, const RuntimeFormat::MmatData& Data)
{
	std::uint64_t path_size = 0;
	if (!CanStoreMmatPath(Data.BaseColorTexturePath) ||
		!CanStoreMmatPath(Data.NormalMapTexturePath) ||
		!CanStoreMmatPath(Data.ToonTexturePath) ||
		!CanStoreMmatPath(Data.SphereTexturePath) ||
		!CanAdd(Data.BaseColorTexturePath.size(), Data.NormalMapTexturePath.size(), path_size) ||
		!CanAdd(path_size, Data.ToonTexturePath.size(), path_size) ||
		!CanAdd(path_size, Data.SphereTexturePath.size(), path_size))
	{
		return false;
	}
	RuntimeFormat::MmatHeader header{{'M', 'M', 'A', 'T'}, MmatFormatVersion,
		static_cast<std::uint32_t>(Data.BaseColorTexturePath.size()), static_cast<std::uint32_t>(Data.NormalMapTexturePath.size()),
		static_cast<std::uint32_t>(Data.ToonTexturePath.size()), static_cast<std::uint32_t>(Data.SphereTexturePath.size())};
	RuntimeFormat::MmatValues values{Data.Diffuse, Data.Specular, Data.SpecularPower, Data.Ambient,
		Data.UseSphereMap ? 1U : 0U, Data.UseToonMap ? 1U : 0U};
	RuntimeFormat::MmatTexturePaths paths{};
	std::memcpy(paths.BaseColor, Data.BaseColorTexturePath.data(), Data.BaseColorTexturePath.size());
	std::memcpy(paths.NormalMap, Data.NormalMapTexturePath.data(), Data.NormalMapTexturePath.size());
	std::memcpy(paths.Toon, Data.ToonTexturePath.data(), Data.ToonTexturePath.size());
	std::memcpy(paths.Sphere, Data.SphereTexturePath.data(), Data.SphereTexturePath.size());
	std::vector<char> payload;
	payload.reserve(sizeof(values) + sizeof(paths));
	return AppendObject(payload, values) &&
		AppendObject(payload, paths) &&
		WriteFile(FilePath, header, payload);
}

bool RuntimeFormatIO::ReadMmat(const std::filesystem::path& FilePath, RuntimeFormat::MmatData& OutData)
{
	RuntimeFormat::MmatHeader header{};
	std::vector<char> payload;
	if (!OpenAndReadPayload(FilePath, &header, sizeof(header), payload) ||
		std::memcmp(header.Magic, "MMAT", sizeof(header.Magic)) != 0 ||
		(header.Version != MmatFormatVersion && header.Version != 2))
	{
		return false;
	}
	std::uint64_t expected = 0;
	const std::size_t values_size = header.Version == 2 ? sizeof(LegacyMmatValues) : sizeof(RuntimeFormat::MmatValues);
	if (payload.size() != values_size + sizeof(RuntimeFormat::MmatTexturePaths) ||
		!IsValidMmatPath(payload.data() + values_size, header.BaseColorTexturePathLength) ||
		!IsValidMmatPath(payload.data() + values_size + 128, header.NormalMapTexturePathLength) ||
		!IsValidMmatPath(payload.data() + values_size + 256, header.ToonTexturePathLength) ||
		!IsValidMmatPath(payload.data() + values_size + 384, header.SphereTexturePathLength) ||
		!CanAdd(header.BaseColorTexturePathLength, header.NormalMapTexturePathLength, expected) ||
		!CanAdd(expected, header.ToonTexturePathLength, expected) ||
		!CanAdd(expected, header.SphereTexturePathLength, expected))
	{
		return false;
	}
	RuntimeFormat::MmatData data;
	std::size_t offset = 0;
	RuntimeFormat::MmatValues values{};
	std::uint32_t use_sphere_map = 0;
	std::uint32_t use_toon_map = 0;
	if (header.Version == 2)
	{
		LegacyMmatValues legacy_values{};
		if (!ReadObject(payload, offset, legacy_values) || legacy_values.UseSphereMap > 1U) { return false; }
		values.Diffuse = legacy_values.Diffuse; values.Specular = legacy_values.Specular;
		values.SpecularPower = legacy_values.SpecularPower; values.Ambient = legacy_values.Ambient;
		use_sphere_map = legacy_values.UseSphereMap;
	}
	else if (!ReadObject(payload, offset, values) || values.UseSphereMap > 1U || values.UseToonMap > 1U) { return false; }
	RuntimeFormat::MmatTexturePaths paths{};
	if (!ReadObject(payload, offset, paths)) { return false; }
	data.Diffuse = values.Diffuse;
	data.Specular = values.Specular;
	data.SpecularPower = values.SpecularPower;
	data.Ambient = values.Ambient;
	data.UseSphereMap = header.Version == 2 ? use_sphere_map != 0 : values.UseSphereMap != 0;
	data.UseToonMap = header.Version == 2 ? false : values.UseToonMap != 0;
	data.BaseColorTexturePath.assign(paths.BaseColor, header.BaseColorTexturePathLength);
	data.NormalMapTexturePath.assign(paths.NormalMap, header.NormalMapTexturePathLength);
	data.ToonTexturePath.assign(paths.Toon, header.ToonTexturePathLength);
	if (header.Version == 2) { data.UseToonMap = !data.ToonTexturePath.empty(); }
	data.SphereTexturePath.assign(paths.Sphere, header.SphereTexturePathLength);
	OutData = std::move(data);
	return true;
}

bool RuntimeFormatIO::WriteMstc(const std::filesystem::path& FilePath, const RuntimeFormat::MstcData& Data)
{
	// 頂点数と各可変長データがファイル形式の範囲内か確認する.
	if (Data.Vertices.size() > MaximumVertexCount || !CanRepresentAsUint32(Data.Indices.size()))
	{
		return false;
	}
	if (!CanRepresentAsUint32(Data.MaterialPath.size()))
	{
		return false;
	}

	RuntimeFormat::MstcHeader header{{'M', 'S', 'T', 'C'}, RuntimeFormatVersion,
		static_cast<std::uint32_t>(Data.Vertices.size()), static_cast<std::uint32_t>(Data.Indices.size()),
		static_cast<std::uint32_t>(Data.MaterialPath.size())};
	std::vector<char> payload;
	std::uint64_t vertex_bytes = 0;
	std::uint64_t index_bytes = 0;
	if (!CanMultiply(Data.Vertices.size(), sizeof(RuntimeFormat::StaticVertex), vertex_bytes) ||
		!CanMultiply(Data.Indices.size(), sizeof(std::uint16_t), index_bytes) ||
		!CanAdd(vertex_bytes, index_bytes, vertex_bytes) ||
		!CanAdd(vertex_bytes, Data.MaterialPath.size(), vertex_bytes) ||
		vertex_bytes > std::numeric_limits<std::size_t>::max())
	{
		return false;
	}
	payload.reserve(static_cast<std::size_t>(vertex_bytes));
	if (!Data.Vertices.empty() &&
		!AppendBytes(payload, Data.Vertices.data(), Data.Vertices.size() * sizeof(RuntimeFormat::StaticVertex)))
	{
		return false;
	}
	if (!Data.Indices.empty() &&
		!AppendBytes(payload, Data.Indices.data(), Data.Indices.size() * sizeof(std::uint16_t)))
	{
		return false;
	}
	if (!AppendBytes(payload, Data.MaterialPath.data(), Data.MaterialPath.size()))
	{
		return false;
	}
	return WriteFile(FilePath, header, payload);
}

bool RuntimeFormatIO::ReadMstc(const std::filesystem::path& FilePath, RuntimeFormat::MstcData& OutData)
{
	RuntimeFormat::MstcHeader header{};
	std::vector<char> payload;
	if (!OpenAndReadPayload(FilePath, &header, sizeof(header), payload) ||
		!HasMagicAndVersion(header.Magic, "MSTC", header.Version) ||
		header.VertexCount > MaximumVertexCount)
	{
		return false;
	}
	std::uint64_t expected = 0;
	std::uint64_t bytes = 0;
	if (!CanMultiply(header.VertexCount, sizeof(RuntimeFormat::StaticVertex), bytes) || !CanAdd(expected, bytes, expected) ||
		!CanMultiply(header.IndexCount, sizeof(std::uint16_t), bytes) || !CanAdd(expected, bytes, expected) ||
		!CanAdd(expected, header.MaterialPathLength, expected) ||
		expected != payload.size())
	{
		return false;
	}
	RuntimeFormat::MstcData data;
	data.Vertices.resize(header.VertexCount);
	data.Indices.resize(header.IndexCount);
	std::size_t offset = 0;
	if (!ReadBytes(payload, offset, data.Vertices.data(), data.Vertices.size() * sizeof(RuntimeFormat::StaticVertex)) ||
		!ReadBytes(payload, offset, data.Indices.data(), data.Indices.size() * sizeof(std::uint16_t)))
	{
		return false;
	}
	data.MaterialPath.assign(payload.data() + offset, header.MaterialPathLength);
	OutData = std::move(data);
	return true;
}

bool RuntimeFormatIO::WriteMskn(const std::filesystem::path& FilePath, const RuntimeFormat::MsknData& Data)
{
	if (!CanRepresentAsUint32(Data.Vertices.size()) ||
		!CanRepresentAsUint32(Data.Indices.size()) ||
		!CanRepresentAsUint32(Data.Bones.size()) ||
		!CanRepresentAsUint32(Data.SkinSlots.size()) ||
		!CanRepresentAsUint32(Data.Submeshes.size()))
	{
		return false;
	}
	for (const RuntimeFormat::SkinBone& bone : Data.Bones)
	{
		if (std::memchr(bone.Name, '\0', sizeof(bone.Name)) == nullptr)
		{
			return false;
		}
	}
	for (const RuntimeFormat::SkinSlot& slot : Data.SkinSlots)
	{
		if (slot.BoneIndex < 0 || static_cast<std::size_t>(slot.BoneIndex) >= Data.Bones.size())
		{
			return false;
		}
	}
	for (std::size_t bone_index = 0; bone_index < Data.Bones.size(); ++bone_index)
	{
		const std::int32_t parent_index = Data.Bones[bone_index].ParentIndex;
		if (parent_index != -1 && (parent_index < 0 || static_cast<std::size_t>(parent_index) >= bone_index))
		{
			return false;
		}
	}
	for (const RuntimeFormat::SkinVertex& vertex : Data.Vertices)
	{
		for (std::size_t i = 0; i < 4; ++i)
		{
			if (vertex.BoneWeights[i] > 0.00001f && static_cast<std::size_t>(vertex.BoneIndices[i]) >= Data.SkinSlots.size())
			{
				return false;
			}
		}
	}
	std::uint64_t submesh_index_count = 0;
	std::uint32_t source_count = 0;
	std::uint32_t relaxed_count = 0;
	for (const RuntimeFormat::SkinSubmesh& submesh : Data.Submeshes)
	{
		if (std::memchr(submesh.MaterialPath, '\0', sizeof(submesh.MaterialPath)) == nullptr ||
			(submesh.Role != RuntimeFormat::SkinSubmeshRole::Normal && submesh.Role != RuntimeFormat::SkinSubmeshRole::Source && submesh.Role != RuntimeFormat::SkinSubmeshRole::RelaxedOccluder) ||
			!CanAdd(submesh_index_count, submesh.IndexCount, submesh_index_count))
		{
			return false;
		}
		if (submesh.Role == RuntimeFormat::SkinSubmeshRole::Source) { ++source_count; }
		if (submesh.Role == RuntimeFormat::SkinSubmeshRole::RelaxedOccluder) { ++relaxed_count; }
	}
	if ((source_count == 0) != (relaxed_count == 0) || Data.FrontCompositeOpacity < 0.0f || Data.FrontCompositeOpacity > 1.0f || Data.FrontCompositeMaxDistance < 0.0f)
	{
		return false;
	}
	if (submesh_index_count != Data.Indices.size())
	{
		return false;
	}
	RuntimeFormat::MsknHeader header{{'M', 'S', 'K', 'N'}, MsknFormatVersion,
		static_cast<std::uint32_t>(Data.Vertices.size()), static_cast<std::uint32_t>(Data.Indices.size()),
		static_cast<std::uint32_t>(Data.Bones.size()), static_cast<std::uint32_t>(Data.Submeshes.size()),
		static_cast<std::uint32_t>(Data.SkinSlots.size()), source_count, relaxed_count,
		Data.FrontCompositeOpacity, Data.FrontCompositeMaxDistance};
	std::vector<char> payload;
	if (!AppendBytes(payload, Data.Vertices.data(), Data.Vertices.size() * sizeof(RuntimeFormat::SkinVertex)) ||
		!AppendBytes(payload, Data.Indices.data(), Data.Indices.size() * sizeof(std::uint32_t)) ||
		!AppendBytes(payload, Data.Bones.data(), Data.Bones.size() * sizeof(RuntimeFormat::SkinBone)) ||
		!AppendBytes(payload, Data.SkinSlots.data(), Data.SkinSlots.size() * sizeof(RuntimeFormat::SkinSlot)) ||
		!AppendBytes(payload, Data.Submeshes.data(), Data.Submeshes.size() * sizeof(RuntimeFormat::SkinSubmesh)))
	{
		return false;
	}
	return WriteFile(FilePath, header, payload);
}

bool RuntimeFormatIO::ReadMskn(const std::filesystem::path& FilePath, RuntimeFormat::MsknData& OutData)
{
	RuntimeFormat::MsknHeader header{};
	std::vector<char> payload;
	if (!OpenAndReadPayload(FilePath, &header, sizeof(header), payload) ||
		!HasMagicAndVersion(header.Magic, "MSKN", header.Version, MsknFormatVersion))
	{
		return false;
	}
	std::uint64_t expected = 0;
	std::uint64_t bytes = 0;
	if (!CanMultiply(header.VertexCount, sizeof(RuntimeFormat::SkinVertex), bytes) ||
		!CanAdd(expected, bytes, expected) ||
		!CanMultiply(header.IndexCount, sizeof(std::uint32_t), bytes) ||
		!CanAdd(expected, bytes, expected) ||
		!CanMultiply(header.BoneCount, sizeof(RuntimeFormat::SkinBone), bytes) ||
		!CanAdd(expected, bytes, expected) ||
		!CanMultiply(header.SkinSlotCount, sizeof(RuntimeFormat::SkinSlot), bytes) ||
		!CanAdd(expected, bytes, expected) ||
		!CanMultiply(header.SubmeshCount, sizeof(RuntimeFormat::SkinSubmesh), bytes) ||
		!CanAdd(expected, bytes, expected) ||
		expected != payload.size())
	{
		return false;
	}
	RuntimeFormat::MsknData data;
	data.Vertices.resize(header.VertexCount);
	data.Indices.resize(header.IndexCount);
	data.Bones.resize(header.BoneCount);
	data.SkinSlots.resize(header.SkinSlotCount);
	data.Submeshes.resize(header.SubmeshCount);
	std::size_t offset = 0;
	if (!ReadBytes(payload, offset, data.Vertices.data(), data.Vertices.size() * sizeof(RuntimeFormat::SkinVertex)) ||
		!ReadBytes(payload, offset, data.Indices.data(), data.Indices.size() * sizeof(std::uint32_t)) ||
		!ReadBytes(payload, offset, data.Bones.data(), data.Bones.size() * sizeof(RuntimeFormat::SkinBone)) ||
		!ReadBytes(payload, offset, data.SkinSlots.data(), data.SkinSlots.size() * sizeof(RuntimeFormat::SkinSlot)) ||
		!ReadBytes(payload, offset, data.Submeshes.data(), data.Submeshes.size() * sizeof(RuntimeFormat::SkinSubmesh)))
	{
		return false;
	}
	std::uint64_t submesh_index_count = 0;
	for (const RuntimeFormat::SkinSubmesh& submesh : data.Submeshes)
	{
		if (std::memchr(submesh.MaterialPath, '\0', sizeof(submesh.MaterialPath)) == nullptr ||
			!CanAdd(submesh_index_count, submesh.IndexCount, submesh_index_count))
		{
			return false;
		}
	}
	for (const RuntimeFormat::SkinSlot& slot : data.SkinSlots)
	{
		if (slot.BoneIndex < 0 || static_cast<std::size_t>(slot.BoneIndex) >= data.Bones.size())
		{
			return false;
		}
	}
	for (std::size_t bone_index = 0; bone_index < data.Bones.size(); ++bone_index)
	{
		const std::int32_t parent_index = data.Bones[bone_index].ParentIndex;
		if (parent_index != -1 && (parent_index < 0 || static_cast<std::size_t>(parent_index) >= bone_index))
		{
			return false;
		}
	}
	for (const RuntimeFormat::SkinVertex& vertex : data.Vertices)
	{
		for (std::size_t i = 0; i < 4; ++i)
		{
			if (vertex.BoneWeights[i] > 0.00001f && static_cast<std::size_t>(vertex.BoneIndices[i]) >= data.SkinSlots.size())
			{
				return false;
			}
		}
	}
	if (submesh_index_count != data.Indices.size())
	{
		return false;
	}
	if ((header.SourceSubmeshCount == 0) != (header.RelaxedOccluderCount == 0) ||
		header.FrontCompositeOpacity < 0.0f || header.FrontCompositeOpacity > 1.0f || header.FrontCompositeMaxDistance < 0.0f)
	{
		return false;
	}
	std::uint32_t source_count = 0;
	std::uint32_t relaxed_count = 0;
	for (const RuntimeFormat::SkinSubmesh& submesh : data.Submeshes)
	{
		if (submesh.Role == RuntimeFormat::SkinSubmeshRole::Source) { ++source_count; }
		else if (submesh.Role == RuntimeFormat::SkinSubmeshRole::RelaxedOccluder) { ++relaxed_count; }
		else if (submesh.Role != RuntimeFormat::SkinSubmeshRole::Normal) { return false; }
	}
	if (source_count != header.SourceSubmeshCount || relaxed_count != header.RelaxedOccluderCount)
	{
		return false;
	}
	data.FrontCompositeOpacity = header.FrontCompositeOpacity;
	data.FrontCompositeMaxDistance = header.FrontCompositeMaxDistance;
	OutData = std::move(data);
	return true;
}

bool RuntimeFormatIO::WriteMclp(const std::filesystem::path& FilePath, const RuntimeFormat::MclpData& Data)
{
	if (!CanRepresentAsUint32(Data.ClipName.size()) || !CanRepresentAsUint32(Data.Tracks.size()))
	{
		return false;
	}
	RuntimeFormat::MclpHeader header{{'M', 'C', 'L', 'P'}, MclpFormatVersion,
		static_cast<std::uint32_t>(Data.Tracks.size()), static_cast<std::uint32_t>(Data.ClipName.size()), Data.Duration};
	std::vector<char> payload;
	for (const RuntimeFormat::MclpBoneTrack& track : Data.Tracks)
	{
		if (!CanRepresentAsUint32(track.Keyframes.size()) ||
			!AppendObject(payload, RuntimeFormat::BoneTrackHeader{
				track.BoneIndex, static_cast<std::uint32_t>(track.Keyframes.size())}) ||
			!AppendBytes(payload, track.Keyframes.data(), track.Keyframes.size() * sizeof(RuntimeFormat::Keyframe)))
		{
			return false;
		}
	}
	if (!AppendBytes(payload, Data.ClipName.data(), Data.ClipName.size()))
	{
		return false;
	}
	return WriteFile(FilePath, header, payload);
}

bool RuntimeFormatIO::ReadMclp(const std::filesystem::path& FilePath, RuntimeFormat::MclpData& OutData)
{
	RuntimeFormat::MclpHeader header{};
	std::vector<char> payload;
	if (!OpenAndReadPayload(FilePath, &header, sizeof(header), payload) ||
		!HasMagicAndVersion(header.Magic, "MCLP", header.Version, MclpFormatVersion))
	{
		return false;
	}
	if (header.BoneTrackCount > payload.size() / sizeof(RuntimeFormat::BoneTrackHeader))
	{
		return false;
	}
	RuntimeFormat::MclpData data;
	data.Duration = header.Duration;
	data.Tracks.reserve(header.BoneTrackCount);
	std::size_t offset = 0;
	for (std::uint32_t track_index = 0; track_index < header.BoneTrackCount; ++track_index)
	{
		RuntimeFormat::BoneTrackHeader track_header{};
		if (!ReadObject(payload, offset, track_header) ||
			track_header.KeyframeCount > (payload.size() - offset) / sizeof(RuntimeFormat::Keyframe))
		{
			return false;
		}
		RuntimeFormat::MclpBoneTrack track{};
		track.BoneIndex = track_header.BoneIndex;
		track.Keyframes.resize(track_header.KeyframeCount);
		if (!ReadBytes(payload, offset, track.Keyframes.data(), track.Keyframes.size() * sizeof(RuntimeFormat::Keyframe)))
		{
			return false;
		}
		data.Tracks.push_back(std::move(track));
	}
	if (header.ClipNameLength != payload.size() - offset)
	{
		return false;
	}
	data.ClipName.assign(payload.data() + offset, header.ClipNameLength);
	OutData = std::move(data);
	return true;
}
