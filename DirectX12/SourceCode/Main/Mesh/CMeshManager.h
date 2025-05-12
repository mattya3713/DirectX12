#pragma once
#include "Ggraphic/PMD/CPMDActor.h"

/*****************************
* ���b�V���}�l�[�W���[�N���X.
*****************************/
class CMeshManager final
{
public:
	CMeshManager();
	~CMeshManager();

	// �C���X�^���X���擾.
	static CMeshManager* GetInstance() {
		static CMeshManager Instance;
		return &Instance;
	}

	// ���b�V���̓ǂݍ���.
	static bool LoadPMDMesh(CDirectX12& pDx12, CPMDRenderer& Renderer);

	// PMD���b�V���̎擾.
	static CPMDActor* GetPMDMesh(const std::string& Name);

	// �X�^�e�B�b�N���b�V���̃��X�g���擾.
	static std::vector<std::string> GetPMDMeshList();
private:

	// ������R�s�[���폜.
	CMeshManager(const CMeshManager& rhs)				= delete;
	CMeshManager& operator = (const CMeshManager& rhs)	= delete;
private:
	std::unordered_map<std::string, std::unique_ptr<CPMDActor>>	m_PMDMesh;	// PMD���b�V��.

	std::vector<std::string> m_PMDMeshList;	// �X�^�e�B�b�N���b�V�����X�g.
};