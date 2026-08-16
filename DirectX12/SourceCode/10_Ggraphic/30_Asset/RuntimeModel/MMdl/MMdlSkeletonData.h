#pragma once

#include<DirectXMath.h>
#include<cstdint>
#include<string>
#include<vector>

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/12.
* @brief     : .xファイルのFrame階層(ボーン)・スキニング・キーフレームアニメーションを
*            : 表す中間データ. PMXのModel::Bone(名前+初期位置のみの単純な形)では
*            : 表現できない(親からのローカル変換行列・スキニング用オフセット行列・
*            : X独自のキーフレーム形式が必要)ため、X専用の型として別に定義する.
**********************************************************************************/

namespace XSkeleton {

	static constexpr int32_t NoParentIndex = -1;

	// ボーン(Frame)1つ分のデータ. 階層・アニメーション再生専用(GPUスキニング用の行列は
	// 持たない — SkinSlotを参照).
	struct Bone
	{
		std::string Name;
		int32_t ParentIndex = NoParentIndex;

		// 親ボーンからの相対変換(そのFrameのFrameTransformMatrix. バインドポーズ).
		DirectX::XMFLOAT4X4 LocalBindMatrix { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
	};

	// GPUへ送るスキニング行列1枠ぶん. SkinWeights.matrixOffsetは実際には(メッシュ,ボーン)の
	// 組ごとに値が異なりうる(同じボーン名でも、それを参照するメッシュのローカル原点からの
	// 相対位置が違うため並進成分が変わる)ため、ボーンではなく「そのメッシュがそのボーンを
	// 参照した1件」ごとにスロットを割り当てる. Model::Vertex::BoneIndicesはBones配列では
	// なくこのSkinSlots配列のIndexを指す.
	struct SkinSlot
	{
		int32_t BoneIndex = -1; // Bones配列内のIndex(このスロットが従う階層上のボーン).
		DirectX::XMFLOAT4X4 OffsetMatrix { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
		DirectX::XMFLOAT4X4 MeshTransform { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 }; // このスロットを持つMeshの共通空間変換.
	};

	// 時刻付きキー値(回転はクォータニオン(x,y,z,w)、拡縮・移動は(x,y,z)を使う).
	template<typename T>
	struct TimedKey
	{
		uint32_t Time = 0;
		T        Value {};
	};

	// 1つのボーンに対する回転/拡縮/移動のキーフレーム列(出現しない種類は空のまま).
	struct BoneAnimation
	{
		int32_t BoneIndex = -1;
		std::vector<TimedKey<DirectX::XMFLOAT4>> RotationKeys; // クォータニオン(x,y,z,w).
		std::vector<TimedKey<DirectX::XMFLOAT3>> ScaleKeys;
		std::vector<TimedKey<DirectX::XMFLOAT3>> PositionKeys;
	};

	// 1つの名前付きアニメーション(SenzanのX出力では攻撃・回避等の各モーションに対応).
	struct AnimationClip
	{
		std::string Name;
		std::vector<BoneAnimation> BoneAnimations;
		uint32_t MaxTime = 0; // 全キーの中の最大time(クリップの長さの目安. 単位はファイル依存の時間軸).
	};

	// モデル1体ぶんのボーン階層+GPUスキニング用スロット+アニメーションクリップ一式.
	struct SkeletalData
	{
		std::vector<Bone> Bones;
		std::vector<SkinSlot> SkinSlots; // GPUのボーン行列バッファはこの数ぶん.
		std::vector<AnimationClip> Clips;
		std::vector<DirectX::XMFLOAT4X4> VertexTransforms; // ModelData.Verticesと同じ順序の共通空間変換.
		uint32_t TicksPerSecond = 4800; // AnimTicksPerSecond(ファイルに無ければ既定値4800を使う).

		int32_t FindBoneIndex(const std::string& Name) const noexcept
		{
			for (size_t i = 0; i < Bones.size(); ++i)
			{
				if (Bones[i].Name == Name) { return static_cast<int32_t>(i); }
			}
			return -1;
		}
	};

} // namespace XSkeleton
