#pragma once

#include"IModelParser.h"

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/11.
* @brief     : DirectXの.xファイル(テキスト形式)をModel::ModelDataへ変換するパーサー.
*            : Mesh/MeshNormals/MeshTextureCoords/MeshMaterialListのみ対応(ボーン・
*            : アニメーションを持つFrame階層/SkinWeightsは非対応. 見つかったMeshは
*            : Frame階層内も含めて全て読み込み、1つのModelDataへ連結する).
**********************************************************************************/

class XParser : public IModelParser
{
public:
	bool Load(const std::string& FilePath, Model::ModelData& OutData) override;
};
