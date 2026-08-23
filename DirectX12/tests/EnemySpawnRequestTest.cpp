// EnemySpawnRequest構築の単体テスト(スタンドアロン. ゲーム本体には含まれない).
// Enemy生成(Enemy::Create)はGPU/エンジン依存のため本テストでは扱わず、
// 要求構築(定義解決・未知ID拒否)と不正データ除外のみを検証する.
//
// ビルド方法: cl /nologo /EHsc /std:c++20 /W4 /I Data\Library /I SourceCode tests\EnemySpawnRequestTest.cpp /Fe:tests\EnemySpawnRequestTest.exe

#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>

#include <d3d12.h>
#include <DirectXMath.h>

#include "../SourceCode/00_Game/50_Enemy/Definition/EnemyDefinitionCatalog.h"
#include "../SourceCode/00_Game/50_Enemy/Factory/EnemyFactory.h"

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

}

int main()
{
	const std::filesystem::path definitions_json = "EnemySpawnRequestTest.json";
	{
		std::ofstream file(definitions_json);
		file << R"([
			{"Id": "goblin",      "DisplayName": "ゴブリン",    "MaxHP": 40,  "MoveSpeed": 3.0},
			{"Id": "goblin_fast", "DisplayName": "速いゴブリン", "MaxHP": 20,  "MoveSpeed": 6.5}
		])";
	}

	EnemyDefinitionCatalog catalog;
	Check(catalog.Load(definitions_json) && catalog.Size() == 2, "catalog Load");

	EnemyFactory factory(catalog);

	// ===== 既知ID: 定義を解決して要求を組み立てる =====
	{
		EnemySpawnRequest request{};
		request.InitialTransform.Position = { 1.0f, 0.0f, 2.0f };
		Check(factory.TryBuildRequest("goblin", request), "TryBuildRequest(goblin)");
		Check(request.Definition == catalog.Find("goblin"), "request points to catalog entry");
		Check(request.IsValid(), "request IsValid");
	}

	// ===== 未知ID: falseかつ要求を壊さない =====
	{
		EnemySpawnRequest request{};
		factory.TryBuildRequest("goblin", request); // 先に有効な内容を入れておく.
		Check(!factory.TryBuildRequest("dragon", request), "TryBuildRequest(dragon)==false");
		Check(!request.IsValid() || request.Definition->Id == "goblin", "failed build keeps previous state sane");
	}

	// ===== 不正データはカタログ側で除外済み =====
	{
		const std::filesystem::path broken_json = "EnemySpawnRequestTest_broken.json";
		{
			std::ofstream file(broken_json);
			file << R"([{"Id": "bad", "MaxHP": -1}])";
		}
		EnemyDefinitionCatalog broken_catalog;
		broken_catalog.Load(broken_json);
		Check(broken_catalog.Size() == 0, "broken excluded from catalog");

		EnemyFactory broken_factory(broken_catalog);
		EnemySpawnRequest request{};
		Check(!broken_factory.TryBuildRequest("bad", request), "broken id cannot build request");
		std::filesystem::remove(broken_json);
	}

	std::filesystem::remove(definitions_json);

	std::cout << "All " << g_CheckCount << " checks passed.\n";
	return 0;
}
