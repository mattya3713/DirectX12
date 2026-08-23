// EnemySpawnPlanner単体テスト(スタンドアロン. ゲーム本体には含まれない).
// ビルド方法: cl /nologo /EHsc /std:c++20 /W4 /I Data\Library /I SourceCode tests\EnemySpawnPlannerTest.cpp /Fe:tests\EnemySpawnPlannerTest.exe
//
// 確認内容:
// 1. 通常型/高速型から決定的な生成計画が得られる(Level順序保持)
// 2. 未知ID/不正Transform/重複InstanceNameが除外され、理由が記録される
// 3. 同一入力から2回Planしても結果が同一(決定性)
// 4. カタログと壊れた入力でもクラッシュしない

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>

#include "../SourceCode/00_Game/00_Scene/Level/LevelData.h"
#include "../SourceCode/00_Game/50_Enemy/Definition/EnemyDefinitionCatalog.h"
#include "../SourceCode/00_Game/50_Enemy/Planner/EnemySpawnPlanner.h"

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
	const std::filesystem::path definitions_json = "SpawnPlannerTest_definitions.json";

	{
		std::ofstream file(definitions_json);
		file << R"([
			{"Id": "goblin",      "DisplayName": "ゴブリン",    "MaxHP": 40, "MoveSpeed": 3.0},
			{"Id": "goblin_fast", "DisplayName": "速いゴブリン", "MaxHP": 20, "MoveSpeed": 6.5}
		])";
	}

	EnemyDefinitionCatalog catalog;
	Check(catalog.Load(definitions_json), "catalog Load");

	EnemySpawnPlanner planner(catalog);

	// ===== 1. 決定的な計画(正常2体+除外3種を含む入力) =====
	std::vector<LevelEnemySpawnDesc> spawns = {
		{ "goblin",      "e1",   { 3, 0, 3 },  { 0, 90, 0 },  { 1, 1, 1 } },
		{ "dragon",      "",     { 9, 9, 9 },  { 0, 0, 0 },   { 1, 1, 1 } }, // 未知ID→除外.
		{ "goblin_fast", "e2",   { -3, 0, -3 },{ 0, 0, 0 },   { 1, 1, 1 } },
		{ "goblin",      "e1",   { 1, 1, 1 },  { 0, 0, 0 },   { 1, 1, 1 } }, // 重複名→除外.
		{ "goblin_fast", "",     { 0, 0, NAN }, { 0, 0, 0 }, { 1, 1, 1 } }, // 不正Transform→除外.
	};

	const EnemySpawnPlan plan = planner.Plan(spawns);
	Check(plan.Count() == 2, "plan Count==2");
	Check(plan.Issues.size() == 3, "issues Count==3");

	Check(plan.Entries[0].Definition != nullptr && plan.Entries[0].Definition->Id == "goblin", "entry0 goblin");
	Check(plan.Entries[0].Transform.Position.x == 3.0f, "entry0 position");
	Check(plan.Entries[1].Definition->Id == "goblin_fast" && plan.Entries[1].InstanceName == "e2", "entry1 goblin_fast");

	bool has_unknown = false, has_invalid = false, has_duplicate = false;
	for (const EnemySpawnIssue& issue : plan.Issues)
	{
		if (issue.Reason == "unknown_definition")        has_unknown = true;
		if (issue.Reason == "invalid_transform")         has_invalid = true;
		if (issue.Reason == "duplicate_instance_name")   has_duplicate = true;
	}
	Check(has_unknown && has_invalid && has_duplicate, "all issue reasons recorded");

	// ===== 2. 決定性(同一入力→同一計画) =====
	{
		const EnemySpawnPlan second = planner.Plan(spawns);
		Check(second.Count() == plan.Count() && second.Issues.size() == plan.Issues.size(), "deterministic counts");
		for (size_t i = 0; i < plan.Count(); ++i)
		{
			Check(second.Entries[i].Definition->Id == plan.Entries[i].Definition->Id &&
				second.Entries[i].SourceIndex == plan.Entries[i].SourceIndex,
				"deterministic entry order");
		}
	}

	// ===== 3. 空入力・NaN以外の境界 =====
	{
		const EnemySpawnPlan empty_plan = planner.Plan({});
		Check(empty_plan.Empty(), "empty input -> empty plan");

		std::vector<LevelEnemySpawnDesc> unnamed = {
			{ "goblin", "", { 1, 0, 1 }, { 0, 0, 0 }, { 1, 1, 1 } },
			{ "goblin", "", { 2, 0, 2 }, { 0, 0, 0 }, { 1, 1, 1 } },
		};
		const EnemySpawnPlan nameless = planner.Plan(unnamed);
		Check(nameless.Count() == 2, "empty InstanceName is not duplicate");
	}

	std::filesystem::remove(definitions_json);

	std::cout << "All " << g_CheckCount << " checks passed.\n";
	return 0;
}
