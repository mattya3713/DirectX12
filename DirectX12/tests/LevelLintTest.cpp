// LevelLint単体テスト(スタンドアロン. ゲーム本体には含まれない).
// ビルド方法: cl /nologo /EHsc /std:c++20 /utf-8 /W4 /I SourceCode /I Data\Library tests\LevelLintTest.cpp /Fe:tests\LevelLintTest.exe
//
// 確認内容:
// 1. 正常JSONは問題なし(Issues空)
// 2. 構文エラー・ファイル欠損はErrorとして報告
// 3. 必須フィールド欠損/未知敵ID/不正TransformはError
// 4. 重複InstanceName/負Scale/遠い座標はWarning

#include <filesystem>
#include <fstream>
#include <iostream>

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

	bool HasIssue(const LevelLintReport& Report, bool IsError, const std::string& PathPart)
	{
		for (const LevelLintIssue& issue : Report.Issues)
		{
			if (issue.IsError == IsError && issue.Path.find(PathPart) != std::string::npos) { return true; }
		}
		return false;
	}

} // namespace

int main()
{
	const std::filesystem::path lint_json = "LevelLintTest.json";
	const std::filesystem::path broken_json = "LevelLintTest_broken.json";
	const std::filesystem::path mstc_dir = "LevelLintTest_mstc";

	std::filesystem::create_directories(mstc_dir);
	{ std::ofstream dummy(mstc_dir / "cube.mstc"); dummy << "dummy"; }

	LevelLint::Options options{};
	options.MstcDir = mstc_dir;

	EnemyDefinitionCatalog catalog;
	WriteText("LevelLintTest_definitions.json",
		R"([
			{"Id": "goblin", "MaxHP": 40, "MoveSpeed": 3.0}
		])");
	catalog.Load("LevelLintTest_definitions.json");
	Check(catalog.Contains("goblin"), "catalog prepared");
	options.Catalog = &catalog;

	// ===== 1. 正常JSON =====
	{
		WriteText(lint_json,
			R"({
				"Objects": [
					{"Mstc": "cube.mstc", "Position": [0, 0, 3], "RotationDeg": [0, 90, 0], "Scale": [1, 1, 1]}
				],
				"EnemySpawns": [
					{"DefinitionId": "goblin", "InstanceName": "e1", "Position": [2, 0, 5], "RotationDeg": [0, 0, 0], "Scale": [1, 1, 1]}
				],
				"PlayerSpawn": {"Position": [0, 0, 0], "YawDeg": 0},
				"BossSpawn":   {"Position": [0, 0, 8], "YawDeg": 180}
			})");

		const LevelLintReport report = LevelLint::RunFile(lint_json, options);
		Check(report.IsClean(), "valid json is clean");
	}

	// ===== 2. 構文エラー・欠損ファイル =====
	{
		WriteText(broken_json, "{ this is broken ]");
		const LevelLintReport report = LevelLint::RunFile(broken_json, options);
		Check(report.ErrorCount() == 1 && HasIssue(report, true, "(syntax)"), "broken json reported as syntax error");

		const LevelLintReport missing = LevelLint::RunFile("LevelLintTest_missing.json", options);
		Check(missing.ErrorCount() == 1 && HasIssue(missing, true, "(file)"), "missing file reported as error");
	}

	// ===== 3. Error系(必須欠損/未知ID/不正Transform) =====
	{
		WriteText(lint_json,
			R"({
				"Objects": [
					{"Mstc": ""},
					{"Mstc": "no_such_model.mstc"}
				],
				"EnemySpawns": [
					{"DefinitionId": "", "Position": [1, 2]},
					{"DefinitionId": "dragon", "Position": [0, 0, 0], "Scale": [-1, 1, 1]}
				]
			})");

		const LevelLintReport report = LevelLint::RunFile(lint_json, options);

		Check(HasIssue(report, true, "Objects[0].Mstc"), "empty Mstc is error");
		Check(HasIssue(report, false, "Objects[1].Mstc"), "missing model file is warning");
		Check(HasIssue(report, true, "EnemySpawns[0].DefinitionId"), "empty DefinitionId is error");
		Check(HasIssue(report, true, "EnemySpawns[0].Position"), "malformed Position array is error");
		Check(HasIssue(report, true, "EnemySpawns[1].DefinitionId"), "unknown enemy id is error");
		Check(HasIssue(report, false, "EnemySpawns[1].Scale"), "negative scale is warning");
		Check(report.ErrorCount() == 4, "error count == 4");
	}

	// ===== 4. Warning系(重複InstanceName/遠い座標/Yaw非数値) =====
	{
		WriteText(lint_json,
			R"({
				"EnemySpawns": [
					{"DefinitionId": "goblin", "InstanceName": "dup"},
					{"DefinitionId": "goblin", "InstanceName": "dup", "Position": [90000, 0, 0]},
					{"DefinitionId": "goblin"}
				],
				"PlayerSpawn": {"Position": [0, 0, 0], "YawDeg": "east"}
			})");

		const LevelLintReport report = LevelLint::RunFile(lint_json, options);

		Check(HasIssue(report, false, "EnemySpawns[1].InstanceName"), "duplicate instance name is warning");
		Check(HasIssue(report, false, "EnemySpawns[1].Position"), "far spawn position is warning");
		Check(!HasIssue(report, true, "EnemySpawns[2]"), "empty instance name does not duplicate");
		Check(HasIssue(report, true, "PlayerSpawn.YawDeg"), "non-numeric YawDeg is error");
	}

	std::filesystem::remove(lint_json);
	std::filesystem::remove(broken_json);
	std::filesystem::remove("LevelLintTest_definitions.json");
	std::filesystem::remove_all(mstc_dir);

	std::cout << "All " << g_CheckCount << " checks passed.\n";
	return 0;
}
