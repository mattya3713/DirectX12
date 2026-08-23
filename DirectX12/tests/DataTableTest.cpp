// DataTable<T>単体テスト(スタンドアロン. ゲーム本体には含まれない).
// ビルド方法: cl /nologo /EHsc /std:c++20 /W4 /I Data\Library /I SourceCode DataTableTest.cpp /Fe:DataTableTest.exe
//
// 確認内容:
// 1. 配列形式JSONからの読込とキー指定取得
// 2. オブジェクト形式JSONからの読込
// 3. 存在しないキーのFind(nullptr)/Contains(false)
// 4. Set/Edit→Save→再Loadの往復で内容が復元される
// 5. 不正JSON/存在しないファイルでLoadがfalseを返す

#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include "../SourceCode/99_Utility/Data/DataTable.h"
#include "json/json.hpp"

namespace {

	int g_CheckCount = 0; // 実施した確認数.

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

	// 動作確認用のダミーデータ(敵種別ごとの仮パラメータ).
	struct EnemyParam
	{
		int Hp = 0;
		float Speed = 0.0f;
		std::string Color;
	};

	// nlohmann相互変換(ADL. ゲーム側の実装規約と同じ形).
	void from_json(const nlohmann::json& Data, EnemyParam& Param)
	{
		Data.at("Hp").get_to(Param.Hp);
		Data.at("Speed").get_to(Param.Speed);
		if (Data.contains("Color")) { Data.at("Color").get_to(Param.Color); }
	}

	void to_json(nlohmann::json& Data, const EnemyParam& Param)
	{
		Data = nlohmann::json{ {"Hp", Param.Hp}, {"Speed", Param.Speed} };
		if (!Param.Color.empty()) { Data["Color"] = Param.Color; }
	}

	void WriteText(const std::filesystem::path& Path, const char* Text)
	{
		std::ofstream file(Path);
		file << Text;
	}

}

int main()
{
	const std::filesystem::path array_json = "DataTableTest_array.json";
	const std::filesystem::path object_json = "DataTableTest_object.json";
	const std::filesystem::path roundtrip_json = "DataTableTest_roundtrip.json";

	// 準備: 配列形式・オブジェクト形式のサンプルを書き出す.
	WriteText(array_json,
		R"([
			{"Name": "goblin", "Hp": 30, "Speed": 3.5},
			{"Name": "knight", "Hp": 100, "Speed": 2.0, "Color": "silver"}
		])");
	WriteText(object_json,
		R"({
			"goblin": {"Hp": 30, "Speed": 3.5},
			"knight": {"Hp": 100, "Speed": 2.0}
		})");

	// 1. 配列形式からの読込とキー指定取得.
	{
		DataTable<EnemyParam> table;
		Check(table.Load(array_json), "Load(array)==true");
		Check(table.Size() == 2, "Size==2");
		Check(table.Contains("goblin"), "Contains(goblin)");
		Check(table.Get("goblin").Hp == 30 && table.Get("goblin").Speed == 3.5f, "Get(goblin) values");
		Check(table.Find("knight") != nullptr && table.Find("knight")->Color == "silver", "Find(knight) Color");
	}

	// 2. オブジェクト形式からの読込.
	{
		DataTable<EnemyParam> table;
		Check(table.Load(object_json), "Load(object)==true");
		Check(table.Size() == 2 && table.Get("knight").Hp == 100, "object format Get(knight)");
	}

	// 3. 存在しないキー.
	{
		DataTable<EnemyParam> table;
		table.Load(array_json);
		Check(!table.Contains("dragon"), "!Contains(dragon)");
		Check(table.Find("dragon") == nullptr, "Find(dragon)==nullptr");
	}

	// 4. 編集→Save→再Loadの往復.
	{
		DataTable<EnemyParam> table("Name");
		table.Load(array_json);

		EnemyParam boss{};
		boss.Hp = 999;
		boss.Speed = 1.25f;
		boss.Color = "gold";
		table.Set("boss", boss);

		Check(table.Save(roundtrip_json), "Save(roundtrip)==true");

		DataTable<EnemyParam> reloaded;
		Check(reloaded.Load(roundtrip_json), "Reload(roundtrip)==true");
		Check(reloaded.Size() == 3, "reloaded Size==3");
		Check(reloaded.Get("boss").Hp == 999 && reloaded.Get("boss").Speed == 1.25f, "roundtrip Get(boss)");
		Check(reloaded.Get("goblin").Hp == 30, "roundtrip preserves goblin");
		Check(reloaded.Contains("boss") && reloaded.Get("boss").Color == "gold", "roundtrip key field restored");
	}

	// 5. 異常系.
	{
		DataTable<EnemyParam> table;
		Check(!table.Load("DataTableTest_not_found.json"), "Load(missing)==false");

		const std::filesystem::path broken_json = "DataTableTest_broken.json";
		WriteText(broken_json, "{ this is not json ]");
		Check(!table.Load(broken_json), "Load(broken)==false");
		Check(table.Size() == 0, "broken load leaves table empty");
	}

	// 後片付け.
	std::filesystem::remove(array_json);
	std::filesystem::remove(object_json);
	std::filesystem::remove(roundtrip_json);
	std::filesystem::remove("DataTableTest_broken.json");
	std::filesystem::remove("DataTableTest_not_found.json"); // Load失敗時に作成されないため通常は無い.

	std::cout << "All " << g_CheckCount << " checks passed.\n";
	return 0;
}
