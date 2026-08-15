#pragma once

#include <DirectXMath.h>

#include <cstdint>
#include <string>
#include <vector>

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/15.
* @brief     : ランタイムモデルフォーマットのディスク構造と実行時データ.
**********************************************************************************/

	namespace RuntimeFormat {

	// マテリアルファイルのヘッダー.
	struct MmatHeader {
		char Magic[4];                                  // ファイル種別を示す4文字のマジック.
		std::uint32_t Version;                          // ファイルフォーマットのバージョン.
		std::uint32_t BaseColorTexturePathLength;       // ベースカラー画像パスのバイト数.
		std::uint32_t NormalMapTexturePathLength;       // 法線画像パスのバイト数.
	};
	static_assert(sizeof(MmatHeader) == 16, "MmatHeaderのレイアウトが変わった場合はファイル形式のバージョンを上げること");

	// マテリアルの実行時データ.
	struct MmatData {
		std::string BaseColorTexturePath;                // ベースカラー画像のパス.
		std::string NormalMapTexturePath;                // 法線画像のパス.
	};

	// 静的メッシュファイルのヘッダー.
	struct MstcHeader {
		char Magic[4];                                  // ファイル種別を示す4文字のマジック.
		std::uint32_t Version;                          // ファイルフォーマットのバージョン.
		std::uint32_t VertexCount;                      // 頂点数.
		std::uint32_t IndexCount;                       // インデックス数.
		std::uint32_t MaterialPathLength;               // マテリアルパスのバイト数.
	};
	static_assert(sizeof(MstcHeader) == 20, "MstcHeaderのレイアウトが変わった場合はファイル形式のバージョンを上げること");

	// 静的メッシュの頂点.
	struct StaticVertex {
		DirectX::XMFLOAT3 Position;                     // 頂点位置.
		DirectX::XMFLOAT3 Normal;                       // 法線ベクトル.
		DirectX::XMFLOAT2 UV;                           // テクスチャ座標.
	};
	static_assert(sizeof(StaticVertex) == 32, "StaticVertexのレイアウトが変わった場合はファイル形式のバージョンを上げること");

	// 静的メッシュの実行時データ.
	struct MstcData {
		std::vector<StaticVertex> Vertices;              // 頂点配列.
		std::vector<std::uint16_t> Indices;              // インデックス配列.
		std::string MaterialPath;                        // マテリアルファイルの相対パス.
	};

	// スキニングメッシュファイルのヘッダー.
	struct MsknHeader {
		char Magic[4];                                  // ファイル種別を示す4文字のマジック.
		std::uint32_t Version;                          // ファイルフォーマットのバージョン.
		std::uint32_t VertexCount;                      // 頂点数.
		std::uint32_t IndexCount;                       // インデックス数.
		std::uint32_t BoneCount;                        // ボーン数.
		std::uint32_t SubmeshCount;                     // サブメッシュ数.
	};
	static_assert(sizeof(MsknHeader) == 24, "MsknHeaderのレイアウトが変わった場合はファイル形式のバージョンを上げること");

	// スキニングメッシュの頂点.
	struct SkinVertex {
		DirectX::XMFLOAT3 Position;                     // 頂点位置.
		DirectX::XMFLOAT3 Normal;                       // 法線ベクトル.
		DirectX::XMFLOAT2 UV;                           // テクスチャ座標.
		std::uint16_t BoneIndices[4];                   // 影響するボーンのインデックス.
		float BoneWeights[4];                            // 各ボーンの影響度.
	};
	static_assert(sizeof(SkinVertex) == 56, "SkinVertexのレイアウトが変わった場合はファイル形式のバージョンを上げること");

	// スキニングメッシュのボーン.
	struct SkinBone {
		char Name[64];                                  // ボーン名の終端付き固定長文字列.
		std::int32_t ParentIndex;                       // 親ボーンのインデックス。-1はルート.
		DirectX::XMFLOAT3 BindPosition;                 // バインドポーズの位置.
		DirectX::XMFLOAT4 BindRotation;                 // バインドポーズの回転.
		DirectX::XMFLOAT3 BindScale;                    // バインドポーズのスケール.
	};
	static_assert(sizeof(SkinBone) == 108, "SkinBoneのレイアウトが変わった場合はファイル形式のバージョンを上げること");

	// スキニングメッシュのマテリアル単位のインデックス範囲.
	struct SkinSubmesh {
		char MaterialPath[128];                         // マテリアルファイルの相対パス.
		std::uint32_t IndexCount;                       // このマテリアルに対応するインデックス数.
	};
	static_assert(sizeof(SkinSubmesh) == 132, "SkinSubmeshのレイアウトが変わった場合はファイル形式のバージョンを上げること");

	// スキニングメッシュの実行時データ.
	struct MsknData {
		std::vector<SkinVertex> Vertices;                // 頂点配列.
		std::vector<std::uint16_t> Indices;              // インデックス配列.
		std::vector<SkinBone> Bones;                     // ボーン配列.
		std::vector<SkinSubmesh> Submeshes;              // マテリアル単位のサブメッシュ配列.
	};

	// アニメーションクリップファイルのヘッダー.
	struct MclpHeader {
		char Magic[4];                                  // ファイル種別を示す4文字のマジック.
		std::uint32_t Version;                          // ファイルフォーマットのバージョン.
		std::uint32_t BoneTrackCount;                   // ボーントラック数.
		std::uint32_t ClipNameLength;                   // クリップ名のバイト数.
		float Duration;                                  // クリップの再生時間.
	};
	static_assert(sizeof(MclpHeader) == 20, "MclpHeaderのレイアウトが変わった場合はファイル形式のバージョンを上げること");

	// ボーントラックのヘッダー.
	struct BoneTrackHeader {
		std::uint32_t BoneIndex;                        // 対象ボーンのインデックス.
		std::uint32_t KeyframeCount;                    // キーフレーム数.
	};
	static_assert(sizeof(BoneTrackHeader) == 8, "BoneTrackHeaderのレイアウトが変わった場合はファイル形式のバージョンを上げること");

	// ボーンの一時点における変換.
	struct Keyframe {
		float Time;                                     // クリップ開始からの経過時間.
		DirectX::XMFLOAT3 Position;                     // キーフレームの位置.
		DirectX::XMFLOAT4 Rotation;                     // キーフレームの回転.
		DirectX::XMFLOAT3 Scale;                        // キーフレームのスケール.
	};
	static_assert(sizeof(Keyframe) == 44, "Keyframeのレイアウトが変わった場合はファイル形式のバージョンを上げること");

	// ボーン1本分のアニメーショントラック.
	struct MclpBoneTrack {
		std::uint32_t BoneIndex;                        // 対象ボーンのインデックス.
		std::vector<Keyframe> Keyframes;                // 時系列のキーフレーム配列.
	};

	// アニメーションクリップの実行時データ.
	struct MclpData {
		std::string ClipName;                           // クリップ名.
		float Duration;                                  // クリップの再生時間.
		std::vector<MclpBoneTrack> Tracks;              // ボーントラック配列.
	};

} // namespace RuntimeFormat
