#pragma once
#include "Ggraphic/PMD/CPMDActor.h"

/*****************************
* メッシュマネージャークラス.
*****************************/
class CMeshManager final
{
public:
	CMeshManager();
	~CMeshManager();

	// インスタンスを取得.
	static CMeshManager* GetInstance() {
		static CMeshManager Instance;
		return &Instance;
	}

	// メッシュの読み込み.
	static bool LoadPMDMesh(CDirectX12& pDx12, CPMDRenderer& Renderer);

	// PMDメッシュの取得.
	static CPMDActor* GetPMDMesh(const std::string& Name);

	// スタティックメッシュのリストを取得.
	static std::vector<std::string> GetPMDMeshList();
private:

	// 生成やコピーを削除.
	CMeshManager(const CMeshManager& rhs)				= delete;
	CMeshManager& operator = (const CMeshManager& rhs)	= delete;
private:
	std::unordered_map<std::string, std::unique_ptr<CPMDActor>>	m_PMDMesh;	// PMDメッシュ.

	std::vector<std::string> m_PMDMeshList;	// スタティックメッシュリスト.
};