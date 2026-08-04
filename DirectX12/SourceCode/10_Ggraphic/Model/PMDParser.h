#pragma once

#include<cstdio>
#include<cstdint>
#include<DirectXMath.h>
#include"IModelParser.h"

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/03.
* @brief     : PMXファイルの生バイナリ構造体.
**********************************************************************************/
namespace PMD {

	// PMDヘッダー構造体.
	struct Header
	{
		float Version;            // バージョン.
		char ModelName[20];       // モデルの名前.
		char ModelComment[256];   // モデルのコメント.

		Header()
			: Version		(0.0f)
			, ModelName		{}
			, ModelComment	{}
		{}
	};

#pragma pack(push, 1)
	// PMDマテリアル構造体(ファイルの生レイアウトそのまま、パディングなし70Byte).
	struct Material {
		DirectX::XMFLOAT3 Diffuse;  // ディフューズ色			: 12Byte.
		float	 Alpha;				// α値					:  4Byte.
		float    Specularity;		// スペキュラの強さ		:  4Byte.
		DirectX::XMFLOAT3 Specular; // スペキュラ色			: 12Byte.
		DirectX::XMFLOAT3 Ambient;  // アンビエント色			: 12Byte.
		uint8_t  ToonIdx;			// トゥーン番号			:  1Byte.
		uint8_t  EdgeFlg;			// Material毎の輪郭線ﾌﾗｸﾞ	:  1Byte.
		uint32_t IndicesNum;		// 割り当たるインデックス数	:  4Byte.
		char     TexFilePath[20];	// テクスチャファイル名	: 20Byte.

		Material()
			: Diffuse		(0.0f, 0.0f, 0.0f)
			, Alpha			(1.0f)
			, Specularity	(0.0f)
			, Specular		(0.0f, 0.0f, 0.0f)
			, Ambient		(0.0f, 0.0f, 0.0f)
			, ToonIdx		(0)
			, EdgeFlg		(0)
			, IndicesNum	(0)
			, TexFilePath	{}
		{}
	};

	// PMDボーン構造体(ファイルの生レイアウトそのまま、パディングなし39Byte).
	struct Bone
	{
		unsigned char	BoneName[20];	// ボーン名.
		unsigned short	ParentNo;		// 親ボーン名.
		unsigned short	NextNo;			// 先端のボーン番号.
		unsigned char	TypeNo;			// ボーンの種類.
		unsigned short	IKBoneNo;		// IKボーン番号.
		DirectX::XMFLOAT3 Pos;			// ボーンの基準座標.

		Bone()
			: BoneName		{}
			, ParentNo		(0)
			, NextNo		(0)
			, TypeNo		(0)
			, IKBoneNo		(0)
			, Pos			(0.0f, 0.0f, 0.0f)
		{}
	};

	// PMD頂点構造体(ファイルの生レイアウトそのまま、パディングなし38Byte).
	struct Vertex
	{
		DirectX::XMFLOAT3 Pos;		// 頂点座標		: 12Byte.
		DirectX::XMFLOAT3 Normal;	// 法線ベクトル	: 12Byte.
		DirectX::XMFLOAT2 UV;		// uv座標		:  8Byte.
		uint16_t BoneNo[2];			// ボーン番号	:  4Byte.
		uint8_t  BoneWeight;		// ボーン影響度	:  1Byte.
		uint8_t  EdgeFlg;			// 輪郭線フラグ	:  1Byte.

		Vertex()
			: Pos			(0.0f, 0.0f, 0.0f)
			, Normal		(0.0f, 0.0f, 0.0f)
			, UV			(0.0f, 0.0f)
			, BoneNo		{}
			, BoneWeight	(0)
			, EdgeFlg		(0)
		{}
	};
#pragma pack(pop)

} // namespace PMD

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/03.
* @brief     : PMDファイル(.pmd)をModel::ModelDataへ変換するパーサー.
**********************************************************************************/

class PMDParser : public IModelParser
{
public:
	bool Load(const std::string& FilePath, Model::ModelData& OutData) override;

private:
	void ReadVertices(FILE* fp, Model::ModelData& OutData);
	void ReadFaces(FILE* fp, Model::ModelData& OutData);
	void ReadMaterials(FILE* fp, const std::string& FilePath, Model::ModelData& OutData);
	void ReadBones(FILE* fp, Model::ModelData& OutData);
};
