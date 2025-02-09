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

// PMDメッシュの取得.
std::vector<std::string> CMeshManager::GetPMDMeshList()
{
	return GetInstance()->m_PMDMeshList;
}

// PMDメッシュの読み込み.
bool CMeshManager::LoadPMDMesh(CDirectX12& pDx12, CPMDRenderer& Renderer)
{
	CMeshManager* pI = GetInstance();

	auto LoadMesh = [&](const std::filesystem::directory_entry& Entry)
		{
			const std::string	Extension = Entry.path().extension().string();	// 拡張子.
			const std::string	FileName  = Entry.path().stem().string();		// ファイル名.
			const std::wstring	FilePath  = Entry.path().wstring();				// ファイルパス.

			// 拡張子が".x"でない場合読み込まない.
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
		// エラーメッセージを表示.
		std::wstring WStr = MyString::StringToWString(e.what());
		_ASSERT_EXPR(false, WStr.c_str());
		return false;
	}
	return true;
}

CPMDActor* CMeshManager::GetPMDMesh(const std::string& Name)
{
	// 指定したモデルを返す..
	for (auto& Model : GetInstance()->m_PMDMesh)
	{
		if (Model.first == Name) { return Model.second.get(); }
	}
	_ASSERT_EXPR(false, L"Not Error Message");
	return nullptr;
}
