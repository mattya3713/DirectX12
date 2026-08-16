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
		std::uint32_t ToonTexturePathLength;            // トゥーン画像パスのバイト数.
		std::uint32_t SphereTexturePathLength;          // スフィア画像パスのバイト数.
	};
	static_assert(sizeof(MmatHeader) == 24, "MmatHeaderのレイアウトが変わった場合はファイル形式のバージョンを上げること");

	// マテリアルの固定長パラメーター.
	struct MmatValues {
		DirectX::XMFLOAT4 Diffuse;
		DirectX::XMFLOAT3 Specular;
		float SpecularPower;
		DirectX::XMFLOAT3 Ambient;
		std::uint32_t UseSphereMap;
		std::uint32_t UseToonMap;
	};
	static_assert(sizeof(MmatValues) == 52, "MmatValuesのレイアウトが変わった場合はファイル形式のバージョンを上げること");

	// マテリアルの固定長テクスチャパス.
	struct MmatTexturePaths {
		char BaseColor[128];                              // ベースカラー画像パスの終端付き文字列.
		char NormalMap[128];                              // 法線画像パスの終端付き文字列.
		char Toon[128];                                   // トゥーン画像パスの終端付き文字列.
		char Sphere[128];                                 // スフィア画像パスの終端付き文字列.
	};
	static_assert(sizeof(MmatTexturePaths) == 512, "MmatTexturePathsのレイアウトが変わった場合はファイル形式のバージョンを上げること");

	// マテリアルの実行時データ.
	struct MmatData {
		DirectX::XMFLOAT4 Diffuse { 1,1,1,1 };          // 拡散色.
		DirectX::XMFLOAT3 Specular { 0,0,0 };            // 鏡面反射色.
		float SpecularPower = 0.0f;                     // 鏡面反射強度.
		DirectX::XMFLOAT3 Ambient { 0,0,0 };             // 環境光色.
		bool UseSphereMap = false;                       // スフィアマップ使用フラグ.
		bool UseToonMap = false;                         // トゥーン計算使用フラグ.
		std::string BaseColorTexturePath;                // ベースカラー画像のパス.
		std::string NormalMapTexturePath;                // 法線画像のパス.
		std::string ToonTexturePath;                     // トゥーン画像のパス.
		std::string SphereTexturePath;                   // スフィア画像のパス.
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
		std::uint32_t SkinSlotCount;                    // スキンスロット数.
		std::uint32_t SourceSubmeshCount;               // 合成ソースのサブメッシュ数.
		std::uint32_t RelaxedOccluderCount;             // 深度緩和対象のサブメッシュ数.
		float FrontCompositeOpacity;                    // オフスクリーン結果の不透明度.
		float FrontCompositeMaxDistance;                // 髪を奥へ緩和する距離.
	};
	static_assert(sizeof(MsknHeader) == 44, "MsknHeaderのレイアウトが変わった場合はファイル形式のバージョンを上げること");

	// サブメッシュがFrontCompositeで担う描画役割.
	enum class SkinSubmeshRole : std::uint32_t {
		Normal = 0,
		Source = 1,
		RelaxedOccluder = 2,
	};

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

	// メッシュ・ボーン組ごとのスキニングスロット.
	struct SkinSlot {
		std::int32_t BoneIndex;
		DirectX::XMFLOAT4X4 OffsetMatrix;
	};
	static_assert(sizeof(SkinSlot) == 68, "SkinSlotのレイアウトが変わった場合はファイル形式のバージョンを上げること");

	// スキニングメッシュのマテリアル単位のインデックス範囲.
	struct SkinSubmesh {
		char MaterialPath[128];                         // マテリアルファイルの相対パス.
		std::uint32_t IndexCount;                       // このマテリアルに対応するインデックス数.
		SkinSubmeshRole Role;                            // モデル固有の合成役割.
	};
	static_assert(sizeof(SkinSubmesh) == 136, "SkinSubmeshのレイアウトが変わった場合はファイル形式のバージョンを上げること");

	// スキニングメッシュの実行時データ.
	struct MsknData {
		std::vector<SkinVertex> Vertices;                // 頂点配列.
		std::vector<std::uint32_t> Indices;              // インデックス配列.
		std::vector<SkinBone> Bones;                     // ボーン配列.
		std::vector<SkinSlot> SkinSlots;                 // 頂点が参照するスキンスロット配列.
		std::vector<SkinSubmesh> Submeshes;              // マテリアル単位のサブメッシュ配列.
		float FrontCompositeOpacity = 1.0f;             // オフスクリーン結果の不透明度.
		float FrontCompositeMaxDistance = 0.0f;         // 髪を奥へ緩和する距離.
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

	// ボーンの一時点における完全な親相対ローカル姿勢.
	struct Keyframe {
		float Time;                                     // クリップ開始からの経過時間.
		DirectX::XMFLOAT3 Position;                     // 親からの完全なローカル位置.
		DirectX::XMFLOAT4 Rotation;                     // 完全なローカル回転.
		DirectX::XMFLOAT3 Scale;                        // 完全なローカルスケール.
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
