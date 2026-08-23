#pragma once

#include <filesystem>
#include <vector>

#include "30_Asset/Parser/ModelData.h"
#include "MMdlSkeletonData.h"
#include "10_Ggraphic/30_Asset/RuntimeFormat/RuntimeFormat.h"
#include "99_Utility/ComPtr/ComPtr.h"

struct ID3D12Resource;

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/15.
* @brief     : mmdlモデルを識別する共有リソース記述.
**********************************************************************************/
class MmdlResource final
{
public:
	explicit MmdlResource(const std::filesystem::path& FilePath);

	// MSKN/MCLPファイルからCPUデータ(頂点/ボーン/クリップ)を読み込む(非同期ロードのワーカーからも呼ばれる).
	void LoadFromFiles();

	MmdlResource(const MmdlResource&) = delete;
	MmdlResource& operator=(const MmdlResource&) = delete;

	const std::filesystem::path& GetFilePath() const noexcept { return m_FilePath; }
	bool HasCpuData() const noexcept { return !m_ModelData.Vertices.empty(); }
	void StoreCpuData(Model::ModelData ModelData, XSkeleton::SkeletalData Skeleton,
		float LocalHeight, std::vector<RuntimeFormat::SkinSubmeshRole> SubmeshRoles,
		float FrontCompositeOpacity, float FrontCompositeMaxDistance);
	const Model::ModelData& GetModelData() const noexcept { return m_ModelData; }
	const XSkeleton::SkeletalData& GetSkeleton() const noexcept { return m_Skeleton; }
	float GetLocalHeight() const noexcept { return m_LocalHeight; }
	const std::vector<RuntimeFormat::SkinSubmeshRole>& GetSubmeshRoles() const noexcept { return m_SkinSubmeshRoles; }
	float GetFrontCompositeOpacity() const noexcept { return m_FrontCompositeOpacity; }
	float GetFrontCompositeMaxDistance() const noexcept { return m_FrontCompositeMaxDistance; }

	const MyComPtr<ID3D12Resource>& GetVertexBuffer() const noexcept { return m_pVertexBuffer; }
	const MyComPtr<ID3D12Resource>& GetIndexBuffer() const noexcept { return m_pIndexBuffer; }
	const MyComPtr<ID3D12Resource>& GetMaterialBuffer() const noexcept { return m_pMaterialBuffer; }
	const std::vector<MyComPtr<ID3D12Resource>>& GetBaseTextureResources() const noexcept { return m_pBaseTextureResources; }
	const std::vector<MyComPtr<ID3D12Resource>>& GetToonTextureResources() const noexcept { return m_pToonTextureResources; }
	const std::vector<MyComPtr<ID3D12Resource>>& GetSphereTextureResources() const noexcept { return m_pSphereTextureResources; }
	void StoreGpuResources(MyComPtr<ID3D12Resource> VertexBuffer, MyComPtr<ID3D12Resource> IndexBuffer,
		MyComPtr<ID3D12Resource> MaterialBuffer, std::vector<MyComPtr<ID3D12Resource>> BaseTextureResources,
		std::vector<MyComPtr<ID3D12Resource>> ToonTextureResources,
		std::vector<MyComPtr<ID3D12Resource>> SphereTextureResources);

private:
	std::filesystem::path m_FilePath;					// 同一モデルを識別する入力パス.
	Model::ModelData m_ModelData;						// 頂点・インデックス・マテリアルの共有CPUデータ.
	XSkeleton::SkeletalData m_Skeleton;					// ボーン階層とクリップの共有CPUデータ.
	float m_LocalHeight = 0.0f;							// モデルの共有基準高さ.
	std::vector<RuntimeFormat::SkinSubmeshRole> m_SkinSubmeshRoles; // サブメッシュの共有描画役割.
	float m_FrontCompositeOpacity = 1.0f;				// 前景合成の共有不透明度.
	float m_FrontCompositeMaxDistance = 0.0f;			// 前景合成の共有距離.
	MyComPtr<ID3D12Resource> m_pVertexBuffer;			// 全インスタンスが参照する頂点バッファ.
	MyComPtr<ID3D12Resource> m_pIndexBuffer;			// 全インスタンスが参照するインデックスバッファ.
	MyComPtr<ID3D12Resource> m_pMaterialBuffer;			// 全インスタンスが参照するマテリアル定数バッファ.
	std::vector<MyComPtr<ID3D12Resource>> m_pBaseTextureResources; // 全インスタンスが参照するベーステクスチャ.
	std::vector<MyComPtr<ID3D12Resource>> m_pToonTextureResources; // 全インスタンスが参照するトゥーンテクスチャ.
	std::vector<MyComPtr<ID3D12Resource>> m_pSphereTextureResources; // 全インスタンスが参照するスフィアテクスチャ.
};
