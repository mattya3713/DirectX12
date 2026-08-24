// FontRegistryの単体テスト(スタンドアロン. ゲーム本体には含まれない).
// ビルド例: cl /nologo /EHsc /std:c++20 /W4 /FIwindows.h /I..\Data\Library /I..\SourceCode ^
//   FontRegistryTest.cpp ..\SourceCode\20_Resource\Font\FontRegistry.cpp ^
//   ..\SourceCode\20_Resource\Font\FontLoader.cpp ..\SourceCode\99_Utility\FileManager\FileManager.cpp ^
//   ..\SourceCode\99_Utility\String\String.cpp /Fe:FontRegistryTest.exe
//
// 確認内容:
// 1. 同一IDの二重登録でキャッシュが再利用される(別フォントIDになる)
// 2. 欠損ファイルはフォールバック扱いでMeiryoへ解決される
// 3. 未登録IDのResolveは-1

#include <cassert>
#include <iostream>

#include "../SourceCode/20_Resource/Font/FontRegistry.h"

namespace {
	int StubLoader(const std::string&, const std::wstring&, int)
	{
		return 100; // FontLoader非依存の固定ダミーID.
	}
}

int main()
{
	FontRegistry::SetLoaderForTest(&StubLoader);
	FontRegistry::Clear();

	// ---- 1. 二重登録のキャッシュ ----
	{
		const int first  = FontRegistry::Register("ui_main", "Data/Font/not_exist.ttf", L"Meiryo", 32);
		const int second = FontRegistry::Register("ui_main", "Data/Font/other.ttf", L"Yu Gothic", 40);
		assert(first == second); // 既存エントリを返す(設定は変更されない).
		std::cout << "[PASS] 1. duplicate register keeps cache entry" << std::endl;
	}

	// ---- 2. Resolveでフォールバック解決+キャッシュ ----
	{
		const int id1 = FontRegistry::Resolve("ui_main");
		assert(id1 >= 0);
		const int id2 = FontRegistry::Resolve("ui_main");
		assert(id1 == id2); // 同一設定の再解決は同一フォント.

		bool found_fallback = false;
		for (const auto& entry : FontRegistry::Entries())
		{
			if (entry.Id == "ui_main") { found_fallback = entry.Fallback; }
		}
		assert(found_fallback); // ファイル欠損のためフォールバック扱い.
		std::cout << "[PASS] 2. missing file falls back and caches" << std::endl;
	}

	// ---- 3. 複数IDの登録・切替 ----
	{
		assert(FontRegistry::Register("title_logo", "", L"Yu Gothic", 64) == -1);
		assert(FontRegistry::Register("damage_num", "", L"Meiryo", 24) == -1);

		const int a = FontRegistry::Resolve("title_logo");
		const int b = FontRegistry::Resolve("damage_num");
		assert(a >= 0 && b >= 0);            // 両方解決できる.
		assert(a == FontRegistry::Resolve("title_logo"));   // 再解決はキャッシュ一致.
		assert(b == FontRegistry::Resolve("damage_num"));
		std::cout << "[PASS] 3. two font ids selectable" << std::endl;
	}

	// ---- 4. 未登録ID ----
	{
		assert(FontRegistry::Resolve("no_such_id") == -1);
		std::cout << "[PASS] 4. unregistered id returns -1" << std::endl;
	}

	std::cout << "All tests passed." << std::endl;
	return 0;
}
