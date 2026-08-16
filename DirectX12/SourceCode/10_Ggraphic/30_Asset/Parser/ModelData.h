#pragma once

#include<d3d12.h>
#include<DirectXMath.h>
#include<vector>
#include<string>
#include<cstdint>
#include <filesystem>

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/03.
* @brief     : モデルフォーマットを問わない、ゲームが実際に使うモデルデータ.
**********************************************************************************/

namespace Model {

	// GPUへ転送する頂点データ(全フォーマット共通レイアウト).
	struct Vertex
	{
		DirectX::XMFLOAT3 Position {};
		DirectX::XMFLOAT3 Normal {};
		DirectX::XMFLOAT2 UV {};

		DirectX::XMFLOAT4 AdditionalUV[4] {};// 追加UV.

		uint32_t BoneIndices[4] {};			// 影響を受けるボーンのインデックス (最大4).
		float BoneWeights[4] { 1.0f };		// 各ボーンのウェイト (合計で1.0fになるように正規化).

		DirectX::XMFLOAT3 SDEF_C {};
		DirectX::XMFLOAT3 SDEF_R0 {};
		DirectX::XMFLOAT3 SDEF_R1 {};
		float Edge{};
	};

	static constexpr size_t GPU_VERTEX_SIZE = sizeof(Vertex);
	static constexpr size_t GPU_INDEX_SIZE  = sizeof(uint32_t);

	// マテリアルが参照するテクスチャパス(アプリからの相対パス).
	struct MaterialTextures
	{
		std::filesystem::path BaseTexture{};
		std::filesystem::path SphereTexture{};	// 乗算スフィアマップ(PMXのSphereMode=1、PMDの.sph).
		std::filesystem::path SphereAddTexture{};	// 加算スフィアマップ(PMDの.spaのみ。PMXでは未使用).
		std::filesystem::path ToonTexture{};
		bool UseSphereMap = false;
		bool UseToonMap = false;
	};

	// ゲームが描画に使うマテリアルデータ.
	struct Material
	{
		std::string        Name{};
		DirectX::XMFLOAT4  Diffuse{};
		DirectX::XMFLOAT3  Specular{};
		float              SpecularPower{};
		DirectX::XMFLOAT3  Ambient{};
		uint32_t           NumFaceCount{};	// このマテリアルが描画するインデックス数.
		MaterialTextures   Textures{};
	};

	// GPUの定数バッファへ転送するマテリアルデータ.
	struct MaterialForHLSL
	{
		DirectX::XMFLOAT4 Diffuse{};
		DirectX::XMFLOAT3 Specular{};
		float             SpecularPower{};
		DirectX::XMFLOAT3 Ambient{};
		float             UseSphereMap{};
		float             UseToonMap{};
	};

	static constexpr size_t GPU_MATERIAL_SIZE = (sizeof(MaterialForHLSL) + 255) & ~255;

	// ボーン階層データ(ゲームが実際に使う最小限の情報).
	struct Bone
	{
		// 「親なし」を表すセンチネル値。PMXの符号ありIndex(-1=指定なし)を符号拡張した値と一致させる.
		static constexpr uint32_t NoParentIndex = 0xFFFFFFFFU;

		std::string        Name{};
		DirectX::XMFLOAT3  Position{};		// ワールド空間での初期位置.
		uint32_t           ParentBoneIndex{ NoParentIndex };
	};

	// フォーマットを問わない、ゲームが使うモデルデータ一式.
	struct ModelData
	{
		std::vector<Vertex>   Vertices;
		std::vector<uint32_t> Indices;
		std::vector<Material> Materials;
		std::vector<Bone>     Bones;
	};

} // namespace Model
