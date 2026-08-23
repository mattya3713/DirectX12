// EnemyDefinitionCatalog単体テスト(スタンドアロン. ゲーム本体には含まれない).
// ビルド方法: cl /nologo /EHsc /std:c++20 /W4 /I Data\Library /I SourceCode tests\EnemyDefinitionCatalogTest.cpp /Fe:tests\EnemyDefinitionCatalogTest.exe
//
// 確認内容:
// 1. 通常型/高速型の2定義をJSON配列から読める
// 2. ID検索・存在確認が正しく動く
// 3. 存在しないIDはFallbackを返し、クラッシュしない
// 4. 不正データ(MaxHP<=0等)は読み込み時に除外される
// 5. 壊れたJSON/存在しないファイルでもクラッシュしない

#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>

#include "../SourceCode/00_Game/50_Enemy/Definition/EnemyDefinitionCatalog.h"

namespace {

	int g_CheckCount = 0;

	void Check(bool Condition, const char* Label)
	{
		++g_CheckCount;
		if (!Condition)
		{
			std::cerr << "FAILED: " << Label << '\n';
			std::exit(1);
		}
		std::cout << "PASS: " << Label << '\n';
	}

	void WriteText(const std::filesystem::path& Path, const char* Text)
	{
		std::ofstream file(Path);
		file << Text;
	}

}

int main()
{
	const std::filesystem::path definitions_json = "EnemyDefinitionTest.json";
	const std::filesystem::path broken_json      = "EnemyDefinitionTest_broken.json";

	// 準備: 通常型+高速型+不正1件を含むカタログ.
	WriteText(definitions_json,
		R"([
			{"Id": "goblin",      "DisplayName": "ゴブリン",   "ModelId": "goblin.mskn",  "MaxHP": 40,  "MoveSpeed": 3.0, "AttackRef": "bite"},
			{"Id": "goblin_fast", "DisplayName": "速いゴブリン","ModelId": "goblin.mskn",  "MaxHP": 20,  "MoveSpeed": 6.5},
			{"Id": "broken",      "DisplayName": "壊れた敵",    "ModelId": "",             "MaxHP": -5,  "MoveSpeed": 0.0}
		])");

	// ===== 1〜2. 読込とID検索 =====
	{
		EnemyDefinitionCatalog catalog;
		Check(catalog.Load(definitions_json), "Load==true");
		Check(catalog.Size() == 2, "invalid entry excluded (Size==2)");
		Check(catalog.Contains("goblin") && catalog.Contains("goblin_fast"), "Contains both");
		Check(!catalog.Contains("broken"), "broken excluded");

		const EnemyDefinition& goblin = catalog.GetOrFallback("goblin");
		Check(goblin.MaxHP == 40 && goblin.MoveSpeed == 3.0f && goblin.DisplayName == "ゴブリン", "GetOrFallback(goblin) values");
		Check(goblin.AttackRef == "bite", "goblin AttackRef");

		const EnemyDefinition* fast = catalog.Find("goblin_fast");
		Check(fast != nullptr && fast->MoveSpeed == 6.5f && fast->RagdollDefID.empty(), "Find(goblin_fast)");
	}

	// ===== 3. 存在しないID(Fallback) =====
	{
		EnemyDefinitionCatalog catalog;
		catalog.Load(definitions_json);

		const EnemyDefinition& unknown = catalog.GetOrFallback("dragon");
		Check(unknown.Id == "fallback" && unknown.MaxHP == 100.0f, "unknown returns Fallback");
		Check(catalog.Find("dragon") == nullptr, "Find(dragon)==nullptr");
	}

	// ===== 4. 不正JSON・欠損ファイル =====
	{
		WriteText(broken_json, "{ this is broken ]");

		EnemyDefinitionCatalog catalog;
		Check(!catalog.Load(broken_json), "Load(broken)==false");
		Check(catalog.Size() == 0, "broken load leaves empty");
		Check(!catalog.Load("EnemyDefinitionTest_missing.json"), "Load(missing)==false");

		// 空のカタログでも問い合わせは安全(Fallback).
		const EnemyDefinition& fallback = catalog.GetOrFallback("anything");
		Check(fallback.Id == "fallback", "empty catalog is safe");
	}

	std::filesystem::remove(definitions_json);
	std::filesystem::remove(broken_json);

	std::cout << "All " << g_CheckCount << " checks passed.\n";
	return 0;
}
