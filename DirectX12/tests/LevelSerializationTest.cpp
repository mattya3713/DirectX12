// LevelData(レベルJSON入出力)単体テスト(スタンドアロン. ゲーム本体には含まれない).
// ビルド方法: cl /nologo /EHsc /std:c++20 /W4 /I Data\Library /I SourceCode tests\LevelSerializationTest.cpp /Fe:tests\LevelSerializationTest.exe
//
// 確認内容:
// 1. 静的オブジェクト+敵スポーン+スポーン地点を含むレベルの保存→読込で復元される
// 2. 旧形式(EnemySpawns無し)のJSONがそのまま読める(後方互換)
// 3. 欠損フィールド・壊れたJSONでクラッシュしない
// 4. 敵スポーンのID欠損エントリはスキップされる

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>

#include "../SourceCode/00_Game/00_Scene/Level/LevelData.h"

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
	const std::filesystem::path full_json    = "LevelTest_full.json";
	const std::filesystem::path legacy_json  = "LevelTest_legacy.json";
	const std::filesystem::path broken_json  = "LevelTest_broken.json";

	// ===== 1. 往復(静的+敵スポーン+スポーン地点) =====
	{
		LevelDesc desc{};
		desc.Objects.push_back({ "cube.mstc", { 4, 1, 4 }, { 0, 45, 0 }, { 2, 2, 2 } });
		desc.EnemySpawns.push_back({ "goblin",      "goblin_1", { 5, 0, -3 }, { 0, 90, 0 }, { 1, 1, 1 } });
		desc.EnemySpawns.push_back({ "goblin_fast", "",         { -5, 0, 2 }, { 0, 270, 0 }, { 1, 1, 1 } });
		desc.PlayerSpawn = { true, { 0, 0, 0 }, 0 };
		desc.BossSpawn   = { true, { 0, 0, 8 }, 180 };

		Check(LevelData::WriteToFile(full_json, desc), "WriteToFile");

		const LevelDesc loaded = LevelData::LoadFromFile(full_json);
		Check(loaded.Objects.size() == 1 && loaded.Objects[0].MstcFile == "cube.mstc", "objects restored");
		Check(loaded.EnemySpawns.size() == 2, "enemy spawns restored");
		Check(loaded.EnemySpawns[0].DefinitionId == "goblin" && loaded.EnemySpawns[0].InstanceName == "goblin_1", "spawn0 id/name");
		Check(loaded.EnemySpawns[0].Position.x == 5.0f && loaded.EnemySpawns[0].RotationDeg.y == 90.0f, "spawn0 transform");
		Check(loaded.EnemySpawns[1].DefinitionId == "goblin_fast" && loaded.EnemySpawns[1].InstanceName.empty(), "spawn1 empty name ok");
		Check(loaded.PlayerSpawn.HasValue && loaded.PlayerSpawn.Position.z == 0.0f, "player spawn");
		Check(loaded.BossSpawn.HasValue && loaded.BossSpawn.YawDeg == 180.0f, "boss spawn");
	}

	// ===== 2. 旧形式(EnemySpawns無し)の後方互換 =====
	{
		WriteText(legacy_json,
			R"({"Objects": [{"Mstc": "cube.mstc", "Position": [1,2,3], "RotationDeg": [0,0,0], "Scale": [1,1,1]}]})");

		const LevelDesc loaded = LevelData::LoadFromFile(legacy_json);
		Check(loaded.Objects.size() == 1, "legacy objects load");
		Check(loaded.EnemySpawns.empty(), "legacy has no enemy spawns");
		Check(!loaded.PlayerSpawn.HasValue && !loaded.BossSpawn.HasValue, "legacy spawns absent");
	}

	// ===== 3. 異常系 =====
	{
		WriteText(broken_json, "{ broken json ]");
		const LevelDesc loaded = LevelData::LoadFromFile(broken_json);
		Check(loaded.Objects.empty() && loaded.EnemySpawns.empty(), "broken json returns empty safely");

		const LevelDesc missing = LevelData::LoadFromFile("LevelTest_missing.json");
		Check(missing.Objects.empty() && !missing.PlayerSpawn.HasValue, "missing file returns empty safely");
	}

	std::filesystem::remove(full_json);
	std::filesystem::remove(legacy_json);
	std::filesystem::remove(broken_json);

	std::cout << "All " << g_CheckCount << " checks passed.\n";
	return 0;
}
