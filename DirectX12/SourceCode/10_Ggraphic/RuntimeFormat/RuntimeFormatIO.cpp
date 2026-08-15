#include "RuntimeFormatIO.h"

#include <cstring>
#include <fstream>
#include <limits>
#include <utility>

namespace {

	constexpr std::uint32_t RuntimeFormatVersion = 1;
	constexpr std::size_t MaximumVertexCount = 65536;

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
	bool HasMagicAndVersion(const char* Magic, const char* ExpectedMagic, std::uint32_t Version)
	{
		return std::memcmp(Magic, ExpectedMagic, 4) == 0 && Version == RuntimeFormatVersion;
	}

	// 2つのテクスチャパスの長さがヘッダーに格納可能か確認する.
	bool ValidateTextureLengths(const std::string& BaseColor, const std::string& Normal, std::uint64_t& OutSize)
	{
		if (!CanRepresentAsUint32(BaseColor.size()) || !CanRepresentAsUint32(Normal.size()))
		{
			return false;
		}
		if (!CanAdd(BaseColor.size(), Normal.size(), OutSize))
		{
			return false;
		}
		return true;
	}

}

bool RuntimeFormatIO::WriteMstc(const std::filesystem::path& FilePath, const RuntimeFormat::MstcData& Data)
{
	// 頂点数と各可変長データがファイル形式の範囲内か確認する.
	if (Data.Vertices.size() > MaximumVertexCount || !CanRepresentAsUint32(Data.Indices.size()))
	{
		return false;
	}
	std::uint64_t path_size = 0;
	if (!ValidateTextureLengths(Data.BaseColorTexturePath, Data.NormalMapTexturePath, path_size))
	{
		return false;
	}

	RuntimeFormat::MstcHeader header{{'M', 'S', 'T', 'C'}, RuntimeFormatVersion,
		static_cast<std::uint32_t>(Data.Vertices.size()), static_cast<std::uint32_t>(Data.Indices.size()),
		static_cast<std::uint32_t>(Data.BaseColorTexturePath.size()), static_cast<std::uint32_t>(Data.NormalMapTexturePath.size())};
	std::vector<char> payload;
	std::uint64_t vertex_bytes = 0;
	std::uint64_t index_bytes = 0;
	if (!CanMultiply(Data.Vertices.size(), sizeof(RuntimeFormat::StaticVertex), vertex_bytes) ||
		!CanMultiply(Data.Indices.size(), sizeof(std::uint16_t), index_bytes) ||
		!CanAdd(vertex_bytes, index_bytes, vertex_bytes) ||
		!CanAdd(vertex_bytes, path_size, vertex_bytes) ||
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
	if (!AppendBytes(payload, Data.BaseColorTexturePath.data(), Data.BaseColorTexturePath.size()) ||
		!AppendBytes(payload, Data.NormalMapTexturePath.data(), Data.NormalMapTexturePath.size()))
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
		!CanAdd(expected, header.BaseColorTexturePathLength, expected) || !CanAdd(expected, header.NormalMapTexturePathLength, expected) ||
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
	data.BaseColorTexturePath.assign(payload.data() + offset, header.BaseColorTexturePathLength);
	offset += header.BaseColorTexturePathLength;
	data.NormalMapTexturePath.assign(payload.data() + offset, header.NormalMapTexturePathLength);
	OutData = std::move(data);
	return true;
}

bool RuntimeFormatIO::WriteMskn(const std::filesystem::path& FilePath, const RuntimeFormat::MsknData& Data)
{
	if (Data.Vertices.size() > MaximumVertexCount ||
		!CanRepresentAsUint32(Data.Indices.size()) ||
		!CanRepresentAsUint32(Data.Bones.size()) ||
		!CanRepresentAsUint32(Data.BaseColorTexturePath.size()))
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
	RuntimeFormat::MsknHeader header{{'M', 'S', 'K', 'N'}, RuntimeFormatVersion,
		static_cast<std::uint32_t>(Data.Vertices.size()), static_cast<std::uint32_t>(Data.Indices.size()),
		static_cast<std::uint32_t>(Data.Bones.size()), static_cast<std::uint32_t>(Data.BaseColorTexturePath.size())};
	std::vector<char> payload;
	if (!AppendBytes(payload, Data.Vertices.data(), Data.Vertices.size() * sizeof(RuntimeFormat::SkinVertex)) ||
		!AppendBytes(payload, Data.Indices.data(), Data.Indices.size() * sizeof(std::uint16_t)) ||
		!AppendBytes(payload, Data.Bones.data(), Data.Bones.size() * sizeof(RuntimeFormat::SkinBone)) ||
		!AppendBytes(payload, Data.BaseColorTexturePath.data(), Data.BaseColorTexturePath.size()))
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
		!HasMagicAndVersion(header.Magic, "MSKN", header.Version) ||
		header.VertexCount > MaximumVertexCount)
	{
		return false;
	}
	std::uint64_t expected = 0;
	std::uint64_t bytes = 0;
	if (!CanMultiply(header.VertexCount, sizeof(RuntimeFormat::SkinVertex), bytes) ||
		!CanAdd(expected, bytes, expected) ||
		!CanMultiply(header.IndexCount, sizeof(std::uint16_t), bytes) ||
		!CanAdd(expected, bytes, expected) ||
		!CanMultiply(header.BoneCount, sizeof(RuntimeFormat::SkinBone), bytes) ||
		!CanAdd(expected, bytes, expected) ||
		!CanAdd(expected, header.BaseColorTexturePathLength, expected) ||
		expected != payload.size())
	{
		return false;
	}
	RuntimeFormat::MsknData data;
	data.Vertices.resize(header.VertexCount);
	data.Indices.resize(header.IndexCount);
	data.Bones.resize(header.BoneCount);
	std::size_t offset = 0;
	if (!ReadBytes(payload, offset, data.Vertices.data(), data.Vertices.size() * sizeof(RuntimeFormat::SkinVertex)) ||
		!ReadBytes(payload, offset, data.Indices.data(), data.Indices.size() * sizeof(std::uint16_t)) ||
		!ReadBytes(payload, offset, data.Bones.data(), data.Bones.size() * sizeof(RuntimeFormat::SkinBone)))
	{
		return false;
	}
	data.BaseColorTexturePath.assign(payload.data() + offset, header.BaseColorTexturePathLength);
	OutData = std::move(data);
	return true;
}

bool RuntimeFormatIO::WriteMclp(const std::filesystem::path& FilePath, const RuntimeFormat::MclpData& Data)
{
	if (!CanRepresentAsUint32(Data.ClipName.size()) || !CanRepresentAsUint32(Data.Tracks.size()))
	{
		return false;
	}
	RuntimeFormat::MclpHeader header{{'M', 'C', 'L', 'P'}, RuntimeFormatVersion,
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
		!HasMagicAndVersion(header.Magic, "MCLP", header.Version))
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
