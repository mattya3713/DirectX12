#pragma once
#include "10_Ggraphic\\90_Legacy\\PMD\\PMDActor.h"

/*****************************
* メッシュマネージャークラス.
* ServiceLocatorへ登録して使う想定(Mainが所有・登録する).
*****************************/
class MeshManager final
{
public:
	MeshManager();
	~MeshManager();

	MeshManager(const MeshManager&)            = delete;
	MeshManager& operator=(const MeshManager&) = delete;

	// メッシュの読み込み.
	static bool LoadPMDMesh(DirectX12& pDx12, PMDRenderer& Renderer);

	// PMDメッシュの取得.
	static PMDActor* GetPMDMesh(const std::string& Name);

	// スタティックメッシュのリストを取得.
	static std::vector<std::string> GetPMDMeshList();
private:
	std::unordered_map<std::string, std::unique_ptr<PMDActor>>	m_PMDMesh;	// PMDメッシュ.

	std::vector<std::string> m_PMDMeshList;	// スタティックメッシュリスト.
};