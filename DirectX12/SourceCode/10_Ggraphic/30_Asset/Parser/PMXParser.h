#pragma once

#include<cstdio>
#include"IModelParser.h"
#include"../../90_Legacy/PMX/PMXStructHeader.h"	// PMXファイルの生バイナリ構造体.

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/03.
* @brief     : PMXファイル(.pmx)をModel::ModelDataへ変換するパーサー.
**********************************************************************************/

class PMXParser : public IModelParser
{
public:
	bool Load(const std::string& FilePath, Model::ModelData& OutData) override;
	// 直前の読み込みでSDEF頂点を検出したか返す.
	bool HasUnsupportedSdef() const noexcept { return m_HasUnsupportedSdef; }

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
	// PMXバイナリから頂点インデックスを読み込み、uint32_tに変換して返す(符号なし).
	uint32_t ReadAndCastIndices(FILE* fp, uint8_t IndexSize);
	// PMXバイナリからボーン/マテリアル/テクスチャ等のインデックスを読み込む(符号あり、-1は「指定なし」).
	// PMX仕様ではこれらのIndexは符号ありで、-1が「未指定」を表す。符号拡張したuint32_tを返すため、
	// 未指定は元のバイト幅に関わらず常に0xFFFFFFFFになる.
	uint32_t ReadAndCastSignedIndices(FILE* fp, uint8_t IndexSize);

	// マテリアルのテクスチャインデックスから、ベース/スフィアのテクスチャパスを解決する.
	void ResolveMaterialTextures(
		uint32_t TextureIndex, uint8_t SphereMode,
		const std::vector<std::string>& TexturePaths,
		std::string& OutBasePath, std::string& OutSpherePath);

	bool m_HasUnsupportedSdef = false;
};
