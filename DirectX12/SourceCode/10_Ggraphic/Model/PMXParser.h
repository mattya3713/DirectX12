#pragma once

#include<cstdio>
#include"IModelParser.h"
#include"../PMX/PMXStructHeader.h"	// PMXファイルの生バイナリ構造体.

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/03.
* @brief     : PMXファイル(.pmx)をModel::ModelDataへ変換するパーサー.
**********************************************************************************/

class PMXParser : public IModelParser
{
public:
	bool Load(const std::string& FilePath, Model::ModelData& OutData) override;

private:
	void ReadHeader(FILE* fp, PMX::Header& OutHeader);
	void ReadModelInfo(FILE* fp, const PMX::Header& Header);
	void ReadVertices(FILE* fp, const PMX::Header& Header, Model::ModelData& OutData);
	void ReadFaces(FILE* fp, const PMX::Header& Header, Model::ModelData& OutData);
	void ReadTextures(FILE* fp, const PMX::Header& Header, std::vector<std::string>& OutTexturePaths);
	void ReadMaterials(FILE* fp, const PMX::Header& Header, const std::vector<std::string>& TexturePaths, Model::ModelData& OutData);
	void ReadBones(FILE* fp, const PMX::Header& Header, Model::ModelData& OutData);

	// 文字列の読み込み(エンコーディング変換含む).
	void ReadString(FILE* fp, std::string& OutString, PMX::TextEncodingType EncodingType);
	// PMXバイナリからインデックスを読み込み、uint32_tに変換して返す.
	uint32_t ReadAndCastIndices(FILE* fp, uint8_t IndexSize);

	// マテリアルのテクスチャインデックスから、ベース/スフィアのテクスチャパスを解決する.
	void ResolveMaterialTextures(
		uint32_t TextureIndex, uint8_t SphereMode,
		const std::vector<std::string>& TexturePaths,
		std::string& OutBasePath, std::string& OutSpherePath);
};
