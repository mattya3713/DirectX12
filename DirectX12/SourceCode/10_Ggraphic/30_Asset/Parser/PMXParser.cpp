#include "PMXParser.h"
#include "99_Utility/String/FilePath/FilePath.h"

// PMXファイルを読み込み、Model::ModelDataへ変換する.
bool PMXParser::Load(const std::string& FilePath, Model::ModelData& OutData)
{
	m_HasUnsupportedSdef = false;
	FILE* fp = nullptr;
	auto err = fopen_s(&fp, FilePath.c_str(), "rb");
	if (err != 0 || !fp) {
		throw std::runtime_error(FilePath + "ファイルを開くことができませんでした。");
	}

	PMX::Header header;
	ReadHeader(fp, header);

	// モデル情報を読み込む(内容はスキップ).
	ReadModelInfo(fp, header);

	// 頂点データ読み込み.
	ReadVertices(fp, header, OutData);

	// インデックスデータ読み込み.
	ReadFaces(fp, header, OutData);

	// テクスチャパス読み込み.
	std::vector<std::string> texture_paths;
	ReadTextures(fp, header, texture_paths);

	// 読み込んだテクスチャパスを、モデルファイルからの相対パスに解決.
	std::string model_dir_path = "";
	size_t last_slash = FilePath.find_last_of("/\\");
	if (last_slash != std::string::npos) {
		model_dir_path = FilePath.substr(0, last_slash + 1);
	}
	for (auto& tex_path : texture_paths) {
		tex_path = MyFilePath::GetTexPath(model_dir_path, tex_path.c_str());
		MyFilePath::ReplaceSlashWithBackslash(&tex_path);
	}

	// マテリアルデータ読み込み.
	ReadMaterials(fp, header, texture_paths, OutData);

	// ボーンデータ読み込み.
	ReadBones(fp, header, OutData);

	fclose(fp);
	return true;
}

// PMXヘッダー読み込み.
void PMXParser::ReadHeader(FILE* fp, PMX::Header& OutHeader)
{
	fread(&OutHeader, PMX::HEADER_SIZE, 1, fp);

	// PMXファイルかどうかの判定.
	if (OutHeader.Signature != PMX::SIGNATURE) {
		throw std::runtime_error("This File is not PMX.");
	}
}

// モデル情報を読み飛ばす.
void PMXParser::ReadModelInfo(FILE* fp, const PMX::Header& Header)
{
	// モデル名(日本語).
	uint32_t name_length = {};
	fread(&name_length, sizeof(name_length), 1, fp);
	fseek(fp, name_length, SEEK_CUR);

	// モデル名(英語).
	uint32_t name_english_length = {};
	fread(&name_english_length, sizeof(name_english_length), 1, fp);
	fseek(fp, name_english_length, SEEK_CUR);

	// モデルコメント(日本語).
	uint32_t comment_length = {};
	fread(&comment_length, sizeof(comment_length), 1, fp);
	fseek(fp, comment_length, SEEK_CUR);

	// モデルコメント(英語).
	uint32_t comment_english_length = {};
	fread(&comment_english_length, sizeof(comment_english_length), 1, fp);
	fseek(fp, comment_english_length, SEEK_CUR);
}

void PMXParser::ReadVertices(FILE* fp, const PMX::Header& Header, Model::ModelData& OutData)
{
	uint32_t vertices_num;
	fread(&vertices_num, sizeof(vertices_num), 1, fp);
	OutData.Vertices.resize(vertices_num);

	for (uint32_t i = 0; i < vertices_num; ++i) {
		fread(&OutData.Vertices[i].Position, sizeof(DirectX::XMFLOAT3), 1, fp);
		fread(&OutData.Vertices[i].Normal, sizeof(DirectX::XMFLOAT3), 1, fp);
		fread(&OutData.Vertices[i].UV, sizeof(DirectX::XMFLOAT2), 1, fp);

		if (Header.AdditionalUV > 0) {
			fread(&OutData.Vertices[i].AdditionalUV[0], sizeof(DirectX::XMFLOAT4) * static_cast<size_t>(Header.AdditionalUV), 1, fp);
		}

		uint8_t weight_type;
		fread(&weight_type, sizeof(uint8_t), 1, fp);

		for (int j = 0; j < 4; ++j) { // 念のため初期化しておく.
			OutData.Vertices[i].BoneIndices[j] = 0;
			OutData.Vertices[i].BoneWeights[j] = 0.0f;
		}

		switch (weight_type) {
			case 0: // BDEF1
				OutData.Vertices[i].BoneIndices[0] = ReadAndCastSignedIndices(fp, Header.BoneIndexSize);
				OutData.Vertices[i].BoneWeights[0] = 1.0f;
				break;
			case 1: // BDEF2
				OutData.Vertices[i].BoneIndices[0] = ReadAndCastSignedIndices(fp, Header.BoneIndexSize);
				OutData.Vertices[i].BoneIndices[1] = ReadAndCastSignedIndices(fp, Header.BoneIndexSize);
				fread(&OutData.Vertices[i].BoneWeights[0], sizeof(float), 1, fp);
				OutData.Vertices[i].BoneWeights[1] = 1.0f - OutData.Vertices[i].BoneWeights[0];
				break;
			case 2: // BDEF4
				OutData.Vertices[i].BoneIndices[0] = ReadAndCastSignedIndices(fp, Header.BoneIndexSize);
				OutData.Vertices[i].BoneIndices[1] = ReadAndCastSignedIndices(fp, Header.BoneIndexSize);
				OutData.Vertices[i].BoneIndices[2] = ReadAndCastSignedIndices(fp, Header.BoneIndexSize);
				OutData.Vertices[i].BoneIndices[3] = ReadAndCastSignedIndices(fp, Header.BoneIndexSize);
				fread(&OutData.Vertices[i].BoneWeights[0], sizeof(float), 1, fp);
				fread(&OutData.Vertices[i].BoneWeights[1], sizeof(float), 1, fp);
				fread(&OutData.Vertices[i].BoneWeights[2], sizeof(float), 1, fp);
				fread(&OutData.Vertices[i].BoneWeights[3], sizeof(float), 1, fp);
				break;
			case 3: // SDEF
				m_HasUnsupportedSdef = true;
				OutData.Vertices[i].BoneIndices[0] = ReadAndCastSignedIndices(fp, Header.BoneIndexSize);
				OutData.Vertices[i].BoneIndices[1] = ReadAndCastSignedIndices(fp, Header.BoneIndexSize);
				fread(&OutData.Vertices[i].BoneWeights[0], sizeof(float), 1, fp);
				fread(&OutData.Vertices[i].SDEF_C, sizeof(DirectX::XMFLOAT3), 1, fp);
				fread(&OutData.Vertices[i].SDEF_R0, sizeof(DirectX::XMFLOAT3), 1, fp);
				fread(&OutData.Vertices[i].SDEF_R1, sizeof(DirectX::XMFLOAT3), 1, fp);
				OutData.Vertices[i].BoneWeights[1] = 1.0f - OutData.Vertices[i].BoneWeights[0];
				break;
			default:
				throw std::runtime_error("Unknown WeightType.");
		}
		fread(&OutData.Vertices[i].Edge, sizeof(float), 1, fp);
	}
}

void PMXParser::ReadFaces(FILE* fp, const PMX::Header& Header, Model::ModelData& OutData)
{
	uint32_t indices_num;
	fread(&indices_num, sizeof(indices_num), 1, fp);
	OutData.Indices.resize(indices_num);

	for (uint32_t i = 0; i < indices_num; ++i) {
		OutData.Indices[i] = ReadAndCastIndices(fp, Header.VertexIndexSize);
	}
}

void PMXParser::ReadTextures(FILE* fp, const PMX::Header& Header, std::vector<std::string>& OutTexturePaths)
{
	uint32_t textures_num;
	fread(&textures_num, sizeof(uint32_t), 1, fp);
	OutTexturePaths.resize(textures_num);

	for (uint32_t i = 0; i < textures_num; ++i) {
		ReadString(fp, OutTexturePaths[i], Header.Encoding);
	}
}

void PMXParser::ResolveMaterialTextures(
	uint32_t TextureIndex, uint8_t SphereMode,
	const std::vector<std::string>& TexturePaths,
	std::string& OutBasePath, std::string& OutSpherePath)
{
	OutBasePath = "";
	OutSpherePath = "";

	if (TextureIndex < TexturePaths.size()) {
		OutBasePath = TexturePaths[TextureIndex];
	}

	if (SphereMode > 0) {
		// スフィアマップは通常、ベーステクスチャのファイル名と*区切りで併記されている場合がある(例: base.bmp*base.sph).
		auto name_pair = MyFilePath::SplitFileName(OutBasePath);
		std::string ext1 = MyFilePath::GetExtension(name_pair.first);
		std::string ext2 = MyFilePath::GetExtension(name_pair.second);

		if (ext1 == "sph" || ext1 == "spa") {
			OutSpherePath = name_pair.first;
			OutBasePath = name_pair.second;
		}
		else if (ext2 == "sph" || ext2 == "spa") {
			OutSpherePath = name_pair.second;
			OutBasePath = name_pair.first;
		}
	}
	else {
		// スフィアマップが独立している場合や、ベーステクスチャに.sph拡張子が付いている場合.
		std::string ext = MyFilePath::GetExtension(OutBasePath);
		if (ext == "sph" || ext == "spa") {
			OutSpherePath = OutBasePath;
			OutBasePath = ""; // ベーステクスチャはなし.
		}
	}
}

void PMXParser::ReadMaterials(FILE* fp, const PMX::Header& Header, const std::vector<std::string>& TexturePaths, Model::ModelData& OutData)
{
	uint32_t material_num;
	fread(&material_num, sizeof(material_num), 1, fp);
	OutData.Materials.resize(material_num);

	for (uint32_t i = 0; i < material_num; ++i) {
		Model::Material& material = OutData.Materials[i];

		std::string name, english_name;
		ReadString(fp, name, Header.Encoding);
		ReadString(fp, english_name, Header.Encoding);
		material.Name = name;

		fread(&material.Diffuse, sizeof(DirectX::XMFLOAT4), 1, fp);
		fread(&material.Specular, sizeof(DirectX::XMFLOAT3), 1, fp);
		fread(&material.SpecularPower, sizeof(float), 1, fp);
		fread(&material.Ambient, sizeof(DirectX::XMFLOAT3), 1, fp);

		uint8_t draw_mode = 0;
		fread(&draw_mode, sizeof(uint8_t), 1, fp);

		DirectX::XMFLOAT4 edge_color = {};
		float edge_size = 0.0f;
		fread(&edge_color, sizeof(DirectX::XMFLOAT4), 1, fp);
		fread(&edge_size, sizeof(float), 1, fp);

		uint32_t texture_index = ReadAndCastSignedIndices(fp, Header.TextureIndexSize);
		uint32_t sphere_texture_index = ReadAndCastSignedIndices(fp, Header.TextureIndexSize);

		uint8_t sphere_mode = 0;
		uint8_t toon_flag = 0;
		fread(&sphere_mode, sizeof(uint8_t), 1, fp);
		fread(&toon_flag, sizeof(uint8_t), 1, fp);

		uint32_t toon_texture_index = 0;
		if (toon_flag == 0) { // モデル固有のトゥーン.
			toon_texture_index = ReadAndCastSignedIndices(fp, Header.TextureIndexSize);
		}
		else { // 共通トゥーン(PMXファイル内のindexは1?10).
			uint8_t common_toon_index = 0;
			fread(&common_toon_index, sizeof(uint8_t), 1, fp);
			toon_texture_index = common_toon_index;
		}

		std::string memo;
		ReadString(fp, memo, Header.Encoding);
		fread(&material.NumFaceCount, sizeof(uint32_t), 1, fp);

		// ベーステクスチャ/スフィアテクスチャのパス解決.
		std::string base_texture_path, sphere_texture_path;
		ResolveMaterialTextures(texture_index, sphere_mode, TexturePaths,
			base_texture_path, sphere_texture_path);
		material.Textures.BaseTexture = base_texture_path;
		material.Textures.SphereTexture = sphere_texture_path;
		material.Textures.UseSphereMap = (sphere_mode == 0);

		// トゥーンテクスチャのパス解決.
		if (toon_flag) { // 共通トゥーン.
			char buffer[32];
			sprintf_s(buffer, sizeof(buffer), PMX::COMMON_TOON_PATH, static_cast<int>(toon_texture_index) + 1);
			material.Textures.ToonTexture = buffer;
		}
		else if (toon_texture_index < TexturePaths.size()) { // モデル固有トゥーン.
			material.Textures.ToonTexture = TexturePaths[toon_texture_index];
		}
		material.Textures.UseToonMap = !material.Textures.ToonTexture.empty();
		// それ以外(モデル固有トゥーンのインデックスが無効)はToonTextureを空のままにし、
		// Actor側でレンダラーのデフォルト(黒テクスチャ)を使う.
	}
}

void PMXParser::ReadBones(FILE* fp, const PMX::Header& Header, Model::ModelData& OutData)
{
	uint32_t bone_num;
	fread(&bone_num, sizeof(bone_num), 1, fp);
	OutData.Bones.resize(bone_num);

	for (uint32_t i = 0; i < bone_num; ++i) {
		Model::Bone& bone = OutData.Bones[i];

		std::string english_name;
		ReadString(fp, bone.Name, Header.Encoding);
		ReadString(fp, english_name, Header.Encoding);

		fread(&bone.Position, sizeof(DirectX::XMFLOAT3), 1, fp);
		bone.ParentBoneIndex = ReadAndCastSignedIndices(fp, Header.BoneIndexSize);

		uint32_t deform_depth = 0;
		fread(&deform_depth, sizeof(uint32_t), 1, fp);

		uint16_t bone_flag = 0;
		fread(&bone_flag, sizeof(uint16_t), 1, fp);

		// 以下、ゲームでは未使用のためファイルカーソルを進めるためだけに読み込む.
		if (!(bone_flag & PMX::BoneFlags::TargetShowMode)) {
			DirectX::XMFLOAT3 position_offset = {};
			fread(&position_offset, sizeof(DirectX::XMFLOAT3), 1, fp);
		}
		else {
			ReadAndCastSignedIndices(fp, Header.BoneIndexSize); // LinkBoneIndex.
		}

		if ((bone_flag & PMX::BoneFlags::AppendRotate) || (bone_flag & PMX::BoneFlags::AppendTranslate)) {
			ReadAndCastSignedIndices(fp, Header.BoneIndexSize); // AppendBoneIndex.
			float append_weight = 0.0f;
			fread(&append_weight, sizeof(float), 1, fp);
		}

		if (bone_flag & PMX::BoneFlags::FixedAxis) {
			DirectX::XMFLOAT3 fixed_axis = {};
			fread(&fixed_axis, sizeof(DirectX::XMFLOAT3), 1, fp);
		}

		if (bone_flag & PMX::BoneFlags::LocalAxis) {
			DirectX::XMFLOAT3 local_x_axis = {}, local_z_axis = {};
			fread(&local_x_axis, sizeof(DirectX::XMFLOAT3), 1, fp);
			fread(&local_z_axis, sizeof(DirectX::XMFLOAT3), 1, fp);
		}

		if (bone_flag & PMX::BoneFlags::DeformOuterParent) {
			uint32_t key_value = 0;
			fread(&key_value, sizeof(uint32_t), 1, fp);
		}

		if (bone_flag & PMX::BoneFlags::IK) {
			ReadAndCastSignedIndices(fp, Header.BoneIndexSize); // IKTargetBoneIndex.
			uint32_t ik_iteration_count = 0;
			float ik_limit = 0.0f;
			fread(&ik_iteration_count, sizeof(uint32_t), 1, fp);
			fread(&ik_limit, sizeof(float), 1, fp);

			uint32_t link_count = 0;
			fread(&link_count, sizeof(uint32_t), 1, fp);
			for (uint32_t link = 0; link < link_count; ++link) {
				ReadAndCastSignedIndices(fp, Header.BoneIndexSize); // IKBoneIndex.
				uint8_t enable_limit = 0;
				fread(&enable_limit, sizeof(uint8_t), 1, fp);
				if (enable_limit != 0) {
					DirectX::XMFLOAT3 limit_min = {}, limit_max = {};
					fread(&limit_min, sizeof(DirectX::XMFLOAT3), 1, fp);
					fread(&limit_max, sizeof(DirectX::XMFLOAT3), 1, fp);
				}
			}
		}
	}
}

// PMXバイナリからインデックスを読み込み、uint32_tに変換して返す.
uint32_t PMXParser::ReadAndCastIndices(FILE* fp, uint8_t IndexSize)
{
	uint32_t value = 0;
	if (IndexSize == 1) {
		uint8_t val; fread(&val, sizeof(uint8_t), 1, fp); value = val;
	}
	else if (IndexSize == 2) {
		uint16_t val; fread(&val, sizeof(uint16_t), 1, fp); value = val;
	}
	else if (IndexSize == 4) {
		fread(&value, sizeof(uint32_t), 1, fp);
	}
	else {
		throw std::runtime_error("Unknown index size.");
	}
	return value;
}

uint32_t PMXParser::ReadAndCastSignedIndices(FILE* fp, uint8_t IndexSize)
{
	int32_t value = 0;
	if (IndexSize == 1) {
		int8_t val; fread(&val, sizeof(int8_t), 1, fp); value = val;
	}
	else if (IndexSize == 2) {
		int16_t val; fread(&val, sizeof(int16_t), 1, fp); value = val;
	}
	else if (IndexSize == 4) {
		fread(&value, sizeof(int32_t), 1, fp);
	}
	else {
		throw std::runtime_error("Unknown index size.");
	}
	return static_cast<uint32_t>(value);
}

void PMXParser::ReadString(FILE* fp, std::string& OutString, PMX::TextEncodingType EncodingType)
{
	uint32_t len;
	fread(&len, sizeof(uint32_t), 1, fp);
	std::vector<uint8_t> buffer(len);
	if (len > 0) {
		fread(buffer.data(), sizeof(uint8_t), len, fp);
	}

	if (buffer.empty()) {
		OutString = "";
		return;
	}

	// UTF16.
	if (EncodingType == PMX::TextEncodingType::UTF16) {
		std::u16string utf16_source(reinterpret_cast<const char16_t*>(buffer.data()), len / sizeof(char16_t));
		OutString = MyString::UTF16ToUTF8(utf16_source);
	}
	// UTF8.
	else {
		OutString.assign(reinterpret_cast<char*>(buffer.data()), len);
	}
}
