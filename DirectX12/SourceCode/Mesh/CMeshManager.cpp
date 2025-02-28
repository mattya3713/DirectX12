#include "Ggraphic/PMD/CPMDRenderer.h"
#include "CMeshManager.h"
#include "Utility/String/String.h"
#include <filesystem>

static constexpr char DEFORECONVERSIONFILEPATH[]	= "In";

CMeshManager::CMeshManager()
	: m_PMDMesh			()
	, m_PMDMeshList		()
{	
}

CMeshManager::~CMeshManager()
{
}

// PMD���b�V���̎擾.
std::vector<std::string> CMeshManager::GetPMDMeshList()
{
	return GetInstance()->m_PMDMeshList;
}

// PMD���b�V���̓ǂݍ���.
bool CMeshManager::LoadPMDMesh(CDirectX12& pDx12, CPMDRenderer& Renderer)
{
	CMeshManager* pI = GetInstance();

	auto LoadMesh = [&](const std::filesystem::directory_entry& Entry)
		{
			const std::string	Extension = Entry.path().extension().string();	// �g���q.
			const std::string	FileName  = Entry.path().stem().string();		// �t�@�C����.
			const std::wstring	FilePath  = Entry.path().wstring();				// �t�@�C���p�X.

			// �g���q��".x"�łȂ��ꍇ�ǂݍ��܂Ȃ�.
			if (Extension != ".pmd" && Extension != ".PMD") { return; }

			const std::string FilePathA = MyString::WStringToString(FilePath);

			pI->m_PMDMesh[FileName] = std::make_unique<CPMDActor>(FilePathA.c_str(), Renderer);
	
			pI->m_PMDMeshList.emplace_back(FileName);
		};

	try
	{
		std::filesystem::recursive_directory_iterator dir_it(DEFORECONVERSIONFILEPATH), end_it;
		std::for_each(dir_it, end_it, LoadMesh);
	}
	catch (const std::exception& e)
	{
		// �G���[���b�Z�[�W��\��.
		std::wstring WStr = MyString::StringToWString(e.what());
		_ASSERT_EXPR(false, WStr.c_str());
		return false;
	}
	return true;
}

CPMDActor* CMeshManager::GetPMDMesh(const std::string& Name)
{
	// �w�肵�����f����Ԃ�..
	for (auto& Model : GetInstance()->m_PMDMesh)
	{
		if (Model.first == Name) { return Model.second.get(); }
	}
	_ASSERT_EXPR(false, L"Not Error Message");
	return nullptr;
}
