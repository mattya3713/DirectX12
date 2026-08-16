#pragma once

#include"IModelParser.h"
#include"../RuntimeModel/MMdl/MMdlSkeletonData.h"

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/11.
* @brief     : DirectXの.xファイル(テキスト形式)をModel::ModelDataへ変換するパーサー.
*            : Mesh/MeshNormals/MeshTextureCoords/MeshMaterialList/SkinWeights/
*            : Frame階層(FrameTransformMatrix)/AnimationSetに対応する.
*            : ボーンを持たない(SkinWeights無し)メッシュはFrame階層の変換行列を
*            : 頂点へ直接焼き込み、スキニングされたメッシュは頂点にBoneIndices/
*            : BoneWeightsを持たせてXActor側でGPUスキニングする.
**********************************************************************************/

class XParser : public IModelParser
{
public:
	// IModelParser実装(骨組みが不要な単純な.xファイル向け. 内部ではLoadSkeletalを呼び、
	// スケルトンは捨てる).
	bool Load(const std::string& FilePath, Model::ModelData& OutData) override;

	// ボーン階層・アニメーションクリップも合わせて取得する版.
	bool LoadSkeletal(const std::string& FilePath, Model::ModelData& OutData, XSkeleton::SkeletalData& OutSkeleton);
};
