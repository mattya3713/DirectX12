// SaveLoadManager + ISerializableの往復テスト(スタンドアロン. ゲーム本体には含まれない).
// ビルド例: cl /nologo /EHsc /std:c++20 /W4 /I..\Data\Library SerializationTest.cpp /Fe:SerializationTest.exe
//
// 確認内容:
// 1. Serialize→保存→読込→Deserializeの往復で値が保持される
// 2. 複数オブジェクトのSaveAll/LoadAllが1ファイルで完結する
// 3. 単体Saveは他キーを壊さない(既存ファイルへ追記される)
// 4. ファイル・キーが無い場合はLoadがfalseを返す
// 5. Deserialize時に足りないキーは既定値のまま残る

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>

#include "../SourceCode/99_Utility/Serialization/ISerializable.h"
#include "../SourceCode/99_Utility/Serialization/SaveLoadManager.h"

namespace {

	// 動作確認用のサンプルクラス(将来の設定セーブを想定した最小構成).
	class GameSettings final : public ISerializable
	{
	public:
		float MasterVolume = 1.0f;
		float BgmVolume = 0.8f;
		int   Difficulty = 1;
		bool  Fullscreen = false;

		nlohmann::json Serialize() const override
		{
			return {
				{"master_volume", MasterVolume},
				{"bgm_volume",    BgmVolume},
				{"difficulty",    Difficulty},
				{"fullscreen",    Fullscreen},
			};
		}

		void Deserialize(const nlohmann::json& Data) override
		{
			if (Data.contains("master_volume")) { Data.at("master_volume").get_to(MasterVolume); }
			if (Data.contains("bgm_volume"))    { Data.at("bgm_volume").get_to(BgmVolume); }
			if (Data.contains("difficulty"))    { Data.at("difficulty").get_to(Difficulty); }
			if (Data.contains("fullscreen"))    { Data.at("fullscreen").get_to(Fullscreen); }
		}
	};

	const std::filesystem::path kTestFile = "serialization_test_output.json";

	bool ValuesEqual(const GameSettings& A, const GameSettings& B)
	{
		return A.MasterVolume == B.MasterVolume &&
		       A.BgmVolume == B.BgmVolume &&
		       A.Difficulty == B.Difficulty &&
		       A.Fullscreen == B.Fullscreen;
	}

} // namespace

int main()
{
	std::error_code ec;
	std::filesystem::remove(kTestFile, ec); // 前回の残骸があれば消す.

	// ---- 1. 往復テスト ----
	{
		GameSettings original;
		original.MasterVolume = 0.25f;
		original.BgmVolume = 0.5f;
		original.Difficulty = 2;
		original.Fullscreen = true;

		assert(SaveLoadManager::Save(kTestFile, "settings", original));

		GameSettings loaded; // 既定値から出発.
		assert(SaveLoadManager::Load(kTestFile, "settings", loaded));
		assert(ValuesEqual(original, loaded));
		std::cout << "[PASS] 1. round trip" << std::endl;
	}

	// ---- 2. SaveAll/LoadAll(複数オブジェクト) ----
	{
		GameSettings a; a.Difficulty = 0; a.MasterVolume = 1.0f;
		GameSettings b; b.Difficulty = 3; b.BgmVolume = 0.1f;

		assert(SaveLoadManager::SaveAll(kTestFile, {
			{"player_settings", &a},
			{"boss_settings",   &b},
		}));

		GameSettings ra; // 全メンバ既定値.
		GameSettings rb;
		assert(SaveLoadManager::LoadAll(kTestFile, {
			{"player_settings", &ra},
			{"boss_settings",   &rb},
		}));
		assert(ra.Difficulty == 0 && ra.MasterVolume == 1.0f);
		assert(rb.Difficulty == 3 && rb.BgmVolume == 0.1f);
		std::cout << "[PASS] 2. save/load all" << std::endl;
	}

	// ---- 3. 単体Saveは他キーを壊さない ----
	{
		GameSettings patch; patch.MasterVolume = 0.75f;
		assert(SaveLoadManager::Save(kTestFile, "player_settings", patch));

		GameSettings untouched_boss;
		assert(SaveLoadManager::Load(kTestFile, "boss_settings", untouched_boss));
		assert(untouched_boss.Difficulty == 3 && untouched_boss.BgmVolume == 0.1f);

		GameSettings patched;
		assert(SaveLoadManager::Load(kTestFile, "player_settings", patched));
		assert(patched.MasterVolume == 0.75f);
		std::cout << "[PASS] 3. single save keeps other keys" << std::endl;
	}

	// ---- 4. 無いファイル・無いキーはfalse ----
	{
		GameSettings dummy;
		assert(!SaveLoadManager::Load("no_such_file.json", "settings", dummy));
		assert(!SaveLoadManager::Load(kTestFile, "no_such_key", dummy));
		std::cout << "[PASS] 4. missing file/key returns false" << std::endl;
	}

	// ---- 5. 足りないキーは既定値のまま ----
	{
		GameSettings partial; partial.Difficulty = 9;
		// difficultyだけ含むJSONを直接書く(他キー欠落).
		nlohmann::json partial_json = { {"settings", {{"difficulty", 9}}} };
		{
			std::ofstream file(kTestFile);
			file << partial_json.dump();
		}

		GameSettings target; // 既定値(Difficulty=1)から出発.
		assert(SaveLoadManager::Load(kTestFile, "settings", target));
		assert(target.Difficulty == 9);        // 有るキーは読める.
		assert(target.MasterVolume == 1.0f);   // 無いキーは既定値のまま.
		assert(target.Fullscreen == false);
		std::cout << "[PASS] 5. missing keys keep defaults" << std::endl;
	}

	std::error_code remove_ec;
	std::filesystem::remove(kTestFile, remove_ec);

	std::cout << "All tests passed." << std::endl;
	return 0;
}
