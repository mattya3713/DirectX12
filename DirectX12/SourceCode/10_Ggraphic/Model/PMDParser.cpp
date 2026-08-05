#include "PMDParser.h"
#include "99_Utility/String/FilePath/FilePath.h"

// PMDファイルを読み込み、Model::ModelDataへ変換する.
bool PMDParser::Load(const std::string& FilePath, Model::ModelData& OutData)
{
	// ヘッダー読み込み用のシグネチャ.
	char signature[3];
	PMD::Header header = {};

	FILE* fp = nullptr;
	auto err = fopen_s(&fp, FilePath.c_str(), "rb");
	if (err != 0 || !fp) {
		throw std::runtime_error(FilePath + "ファイルを開くことができませんでした。");
	}

	// ヘッダー情報を読み込む.
	fread(signature, sizeof(signature), 1, fp);
	fread(&header, sizeof(header), 1, fp);

	// 頂点データ読み込み.
	ReadVertices(fp, OutData);

	// インデックスデータ読み込み.
	ReadFaces(fp, OutData);

	// マテリアルデータ読み込み.
	ReadMaterials(fp, FilePath, OutData);

	// ボーンデータ読み込み.
	ReadBones(fp, OutData);

	fclose(fp);
	return true;
}

void PMDParser::ReadVertices(FILE* fp, Model::ModelData& OutData)
{
	uint32_t vert_num = 0;
	fread(&vert_num, sizeof(vert_num), 1, fp);

	std::vector<PMD::Vertex> pmd_vertices(vert_num);
	fread(pmd_vertices.data(), pmd_vertices.size() * sizeof(PMD::Vertex), 1, fp);

	OutData.Vertices.resize(vert_num);
	for (uint32_t i = 0; i < vert_num; ++i) {
		const PMD::Vertex& src = pmd_vertices[i];
		Model::Vertex& dst = OutData.Vertices[i];

		dst.Position = src.Pos;
		dst.Normal = src.Normal;
		dst.UV = src.UV;

		// PMDは2ボーンまでのウェイトのみ(共通レイアウトの残り2枠は既定値のまま).
		dst.BoneIndices[0] = src.BoneNo[0];
		dst.BoneIndices[1] = src.BoneNo[1];
		dst.BoneWeights[0] = src.BoneWeight / 100.0f; // BoneWeightは0～100の百分率.
		dst.BoneWeights[1] = 1.0f - dst.BoneWeights[0];

		dst.Edge = src.EdgeFlg ? 1.0f : 0.0f;
	}
}

void PMDParser::ReadFaces(FILE* fp, Model::ModelData& OutData)
{
	uint32_t indices_num = 0;
	fread(&indices_num, sizeof(indices_num), 1, fp);

	std::vector<uint16_t> indices(indices_num);
	fread(indices.data(), indices.size() * sizeof(uint16_t), 1, fp);

	OutData.Indices.resize(indices_num);
	for (uint32_t i = 0; i < indices_num; ++i) {
		OutData.Indices[i] = indices[i];
	}
}

void PMDParser::ReadMaterials(FILE* fp, const std::string& FilePath, Model::ModelData& OutData)
{
	int material_num = 0;
	fread(&material_num, sizeof(material_num), 1, fp);

	std::vector<PMD::Material> pmd_materials(material_num);
	fread(pmd_materials.data(), pmd_materials.size() * sizeof(PMD::Material), 1, fp);

	OutData.Materials.resize(material_num);
	for (int i = 0; i < material_num; ++i) {
		const PMD::Material& src = pmd_materials[i];
		Model::Material& material = OutData.Materials[i];

		material.Diffuse = DirectX::XMFLOAT4(src.Diffuse.x, src.Diffuse.y, src.Diffuse.z, src.Alpha);
		material.Specular = src.Specular;
		material.SpecularPower = src.Specularity;
		material.Ambient = src.Ambient;
		material.NumFaceCount = src.IndicesNum;

		// Idxが255ならトゥーンなし。この場合、元実装に合わせテクスチャ解決自体を行わない.
		if (src.ToonIdx == 255) { continue; }

		char toon_file_path[32];
		sprintf_s(toon_file_path, "Data/Image/toon/toon%02d.bmp", src.ToonIdx + 1);
		material.Textures.ToonTexture = toon_file_path;

		if (strlen(src.TexFilePath) == 0) { continue; }

		// ベーステクスチャパスの分解(*区切りで sph/spa と併記されている場合がある).
		std::string tex_file_name = src.TexFilePath;
		std::string sph_file_name = "";
		std::string spa_file_name = "";

		if (std::count(tex_file_name.begin(), tex_file_name.end(), '*') > 0) {
			auto name_pair = MyFilePath::SplitFileName(tex_file_name);
			if (MyFilePath::GetExtension(name_pair.first) == "sph") {
				tex_file_name = name_pair.second;
				sph_file_name = name_pair.first;
			}
			else if (MyFilePath::GetExtension(name_pair.first) == "spa") {
				tex_file_name = name_pair.second;
				spa_file_name = name_pair.first;
			}
			else {
				tex_file_name = name_pair.first;
				if (MyFilePath::GetExtension(name_pair.second) == "sph") {
					sph_file_name = name_pair.second;
				}
				else if (MyFilePath::GetExtension(name_pair.second) == "spa") {
					spa_file_name = name_pair.second;
				}
			}
		}
		else {
			if (MyFilePath::GetExtension(tex_file_name) == "sph") {
				sph_file_name = tex_file_name;
				tex_file_name = "";
			}
			else if (MyFilePath::GetExtension(tex_file_name) == "spa") {
				spa_file_name = tex_file_name;
				tex_file_name = "";
			}
		}

		if (!tex_file_name.empty()) {
			material.Textures.BaseTexture = MyFilePath::GetTexPath(FilePath, tex_file_name.c_str());
		}
		if (!sph_file_name.empty()) {
			material.Textures.SphereTexture = MyFilePath::GetTexPath(FilePath, sph_file_name.c_str());
		}
		if (!spa_file_name.empty()) {
			material.Textures.SphereAddTexture = MyFilePath::GetTexPath(FilePath, spa_file_name.c_str());
		}
	}
}

void PMDParser::ReadBones(FILE* fp, Model::ModelData& OutData)
{
	unsigned short bone_num = 0;
	fread(&bone_num, sizeof(bone_num), 1, fp);

	std::vector<PMD::Bone> pmd_bones(bone_num);
	fread(pmd_bones.data(), sizeof(PMD::Bone), bone_num, fp);

	OutData.Bones.resize(bone_num);
	for (size_t i = 0; i < pmd_bones.size(); ++i) {
		const PMD::Bone& src = pmd_bones[i];
		Model::Bone& bone = OutData.Bones[i];

		bone.Name = std::string(reinterpret_cast<const char*>(src.BoneName));
		bone.Position = src.Pos;
		bone.ParentBoneIndex = src.ParentNo;
	}
}
