#pragma once
#include "Ggraphic/PMD/PMDActor.h"

/*****************************
* メッシュマネージャークラス.
*****************************/
class MeshManager final
{
public:
	MeshManager();
	~MeshManager();

	// インスタンスを取得.
	static MeshManager* GetInstance() {
		static MeshManager Instance;
		return &Instance;
	}

	// メッシュの読み込み.
	static bool LoadPMDMesh(DirectX12& pDx12, PMDRenderer& Renderer);

	// PMDメッシュの取得.
	static PMDActor* GetPMDMesh(const std::string& Name);

	// スタティックメッシュのリストを取得.
	static std::vector<std::string> GetPMDMeshList();
private:

	// 生成やコピーを削除.
	MeshManager(const MeshManager& rhs)				= delete;
	MeshManager& operator = (const MeshManager& rhs)	= delete;
private:
	std::unordered_map<std::string, std::unique_ptr<PMDActor>>	m_PMDMesh;	// PMDメッシュ.

	std::vector<std::string> m_PMDMeshList;	// スタティックメッシュリスト.
};