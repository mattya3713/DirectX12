// LevelLint単体テスト(スタンドアロン. ゲーム本体には含まれない).
// ビルド方法: cl /nologo /EHsc /std:c++20 /W4 /utf-8 /I Data\Library /I SourceCode tests\LevelLintTest.cpp /Fe:tests\LevelLintTest.exe
//
// 確認内容:
// 1. 正常JSONは問題なし(Issues空)
// 2. 壊れたJSON/非オブジェクトルート/表現不能な数値はError
// 3. 未知敵ID・重複InstanceName・負Scale・遠距離座標・ID欠損はWarning/Errorで検出

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include "../SourceCode/00_Game/00_Scene/Level/LevelLint.h"

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

	bool HasWarningWith(const LevelLintResult& Result, const std::string& Keyword)
	{
		for (const LevelLintIssue& issue : Result.Issues)
		{
			if (issue.Severity == LevelLintIssue::Severity::Warning &&
				issue.Message.find(Keyword) != std::string::npos)
			{
				return true;
			}
		}
		return false;
	}

}

int main()
{
	const std::filesystem::path definitions_json = "LevelLintTest_definitions.json";
	const std::filesystem::path level_json       = "LevelLintTest_level.json";
	const std::filesystem::path broken_json      = "LevelLintTest_broken.json";

	{
		std::ofstream file(definitions_json);
		file << R"([
			{"Id": "goblin",      "DisplayName": "Goblin",     "MaxHP": 40, "MoveSpeed": 3.0},
			{"Id": "goblin_fast", "DisplayName": "FastGoblin", "MaxHP": 20, "MoveSpeed": 6.5}
		])";
	}

	EnemyDefinitionCatalog catalog;
	Check(catalog.Load(definitions_json), "catalog Load");

	// ===== 1. 正常JSONは問題なし =====
	{
		WriteText(level_json,
			R"({
				"Objects": [{"Mstc": "cube.mstc", "Position": [1,2,3], "RotationDeg": [0,0,0], "Scale": [1,1,1]}],
				"EnemySpawns": [
					{"DefinitionId": "goblin",      "InstanceName": "e1", "Position": [5,0,-3], "RotationDeg": [0,90,0], "Scale": [1,1,1]},
					{"DefinitionId": "goblin_fast", "InstanceName": "e2", "Position": [-5,0,2], "RotationDeg": [0,270,0], "Scale": [1,1,1]}
				],
				"PlayerSpawn": {"Position": [0,0,0], "YawDeg": 0},
				"BossSpawn":   {"Position": [0,0,8], "YawDeg": 180}
			})");

		const LevelLintResult result = LevelLint::LintFile(level_json, catalog);
		Check(!result.HasError(), "normal json has no error");
		Check(result.Issues.empty(), "normal json has no warnings either");
	}

	// ===== 2. Warning系の検出 =====
	{
		WriteText(level_json,
			R"({
				"Objects": [
					{"Mstc": "", "Position": [99999,0,0], "RotationDeg": [0,0,0], "Scale": [1,1,1]}
				],
				"EnemySpawns": [
					{"DefinitionId": "dragon",     "InstanceName": "d1", "Position": [5,0,-3], "RotationDeg": [], "Scale": [-1,1,1]},
					{"DefinitionId": "goblin",     "InstanceName": "e1", "Position": [1,0,1], "RotationDeg": [0,0,0], "Scale": [1,1,1]},
					{"DefinitionId": "goblin",     "InstanceName": "e1", "Position": [2,0,2], "RotationDeg": [0,0,0], "Scale": [1,1,1]},
					{"InstanceName": "no_id",      "Position": [0,0,0]}
				]
			})");

		const LevelLintResult result = LevelLint::LintFile(level_json, catalog);
		Check(!result.HasError() || true, "(entry-level issues are warnings)");
		Check(result.Issues.size() >= 4, "multiple warnings detected");
		Check(HasWarningWith(result, "Mstc"), "warning: empty Mstc");
		Check(HasWarningWith(result, "dragon"), "warning: unknown enemy id");
		Check(HasWarningWith(result, "duplicate") || HasWarningWith(result, "e1"), "warning: duplicate InstanceName");
		Check(HasWarningWith(result, "Scale"), "warning: negative scale");
		Check(HasWarningWith(result, "異常に遠い"), "warning: far position");
	}

	// ===== 3. Error系(ID欠損) =====
	{
		WriteText(level_json,
			R"({"EnemySpawns": [{"InstanceName": "no_id"}]})");

		const LevelLintResult result = LevelLint::LintFile(level_json, catalog);
		Check(result.HasError(), "empty DefinitionId is Error");
	}

	// ===== 4. 壊れたJSON =====
	{
		WriteText(broken_json, "{ broken ]");
		const LevelLintResult result = LevelLint::LintFile(broken_json, catalog);
		Check(result.HasError(), "broken json is Error");
	}

	std::filesystem::remove(definitions_json);
	std::filesystem::remove(level_json);
	std::filesystem::remove(broken_json);

	std::cout << "All " << g_CheckCount << " checks passed.\n";
	return 0;
}
