#pragma once

#include <DirectXMath.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "json/json.hpp"

/**********************************************************************************
* @author    : 朱雀 (Suzaku / 閃斬 Production Loop Coder).
* @date      : 2026/08/23.
* @brief     : レベルJSONのデータ構造と入出力(ImGui非依存).
*            : 静的オブジェクト・Player/Bossスポーン・敵スポーン定義を保持し、
*            : JSONへの保存/読込を行う. 欠損フィールドは既定値で補完するため、
*            : 壊れた値・旧形式(EnemySpawns無し)でもクラッシュしない.
*            : 敵スポーンはデータ契約のみ(実際の生成はEnemyFactory側タスク).
**********************************************************************************/

struct LevelObjectDesc
{
	std::string          MstcFile;    // Data\Model\mmdl\mstc配下のファイル名.
	DirectX::XMFLOAT3    Position     = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3    RotationDeg  = { 0.0f, 0.0f, 0.0f }; // オイラー角(度. X=Pitch, Y=Yaw, Z=Roll).
	DirectX::XMFLOAT3    Scale        = { 1.0f, 1.0f, 1.0f };
};

// 敵スポーン1体分(EnemyFactoryへ渡すデータ契約. 実体生成は別タスク).
struct LevelEnemySpawnDesc
{
	std::string          DefinitionId; // EnemyDefinitionカタログのID.
	std::string          InstanceName; // デバッグ表示用の任意名(空可).
	DirectX::XMFLOAT3    Position      = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3    RotationDeg   = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3    Scale         = { 1.0f, 1.0f, 1.0f };
};

struct LevelSpawnDesc
{
	bool                 HasValue = false; // JSONにキーが存在したか(無い場合は既存のハードコード値を使う).
	DirectX::XMFLOAT3    Position = { 0.0f, 0.0f, 0.0f };
	float                YawDeg   = 0.0f;
};

struct LevelDesc
{
	std::vector<LevelObjectDesc>      Objects;
	std::vector<LevelEnemySpawnDesc>  EnemySpawns;
	LevelSpawnDesc PlayerSpawn;
	LevelSpawnDesc BossSpawn;
};

namespace LevelData {

	// JSONオブジェクトからLevelDescを復元する(欠損キーは既定値で補完).
	inline LevelDesc Parse(const nlohmann::json& Root)
	{
		LevelDesc desc{};

		if (!Root.is_object()) { return desc; }

		if (Root.contains("Objects"))
		{
			for (const nlohmann::json& entry : Root["Objects"])
			{
				LevelObjectDesc object{};
				object.MstcFile    = entry.value("Mstc", std::string());
				if (object.MstcFile.empty()) { continue; }
				const auto position   = entry.value("Position",    std::vector<float>{ 0.0f, 0.0f, 0.0f });
				const auto rotation   = entry.value("RotationDeg", std::vector<float>{ 0.0f, 0.0f, 0.0f });
				const auto scale      = entry.value("Scale",       std::vector<float>{ 1.0f, 1.0f, 1.0f });
				if (position.size() >= 3) { object.Position    = { position[0], position[1], position[2] }; }
				if (rotation.size() >= 3) { object.RotationDeg = { rotation[0], rotation[1], rotation[2] }; }
				if (scale.size() >= 3)    { object.Scale       = { scale[0], scale[1], scale[2] }; }
				desc.Objects.push_back(std::move(object));
			}
		}

		if (Root.contains("EnemySpawns"))
		{
			for (const nlohmann::json& entry : Root["EnemySpawns"])
			{
				LevelEnemySpawnDesc spawn{};
				spawn.DefinitionId = entry.value("DefinitionId", std::string());
				if (spawn.DefinitionId.empty()) { continue; } // ID欠損はスキップ(安全に無効化).
				spawn.InstanceName = entry.value("InstanceName", std::string());
				const auto position = entry.value("Position",    std::vector<float>{ 0.0f, 0.0f, 0.0f });
				const auto rotation = entry.value("RotationDeg", std::vector<float>{ 0.0f, 0.0f, 0.0f });
				const auto scale    = entry.value("Scale",       std::vector<float>{ 1.0f, 1.0f, 1.0f });
				if (position.size() >= 3) { spawn.Position    = { position[0], position[1], position[2] }; }
				if (rotation.size() >= 3) { spawn.RotationDeg = { rotation[0], rotation[1], rotation[2] }; }
				if (scale.size() >= 3)    { spawn.Scale       = { scale[0], scale[1], scale[2] }; }
				desc.EnemySpawns.push_back(std::move(spawn));
			}
		}

		if (Root.contains("PlayerSpawn"))
		{
			const nlohmann::json& spawn = Root["PlayerSpawn"];
			desc.PlayerSpawn.HasValue = true;
			const auto position = spawn.value("Position", std::vector<float>{ 0.0f, 0.0f, 0.0f });
			if (position.size() >= 3) { desc.PlayerSpawn.Position = { position[0], position[1], position[2] }; }
			desc.PlayerSpawn.YawDeg   = spawn.value("YawDeg", 0.0f);
		}

		if (Root.contains("BossSpawn"))
		{
			const nlohmann::json& spawn = Root["BossSpawn"];
			desc.BossSpawn.HasValue = true;
			const auto position = spawn.value("Position", std::vector<float>{ 0.0f, 0.0f, 8.0f });
			if (position.size() >= 3) { desc.BossSpawn.Position = { position[0], position[1], position[2] }; }
			desc.BossSpawn.YawDeg   = spawn.value("YawDeg", 180.0f);
		}

		return desc;
	}

	// LevelDescをJSONオブジェクトへ出力する(全項目を明示的に書き出す).
	inline nlohmann::json Serialize(const LevelDesc& Desc)
	{
		nlohmann::json objects = nlohmann::json::array();
		for (const LevelObjectDesc& object : Desc.Objects)
		{
			objects.push_back({
				{ "Mstc",        object.MstcFile },
				{ "Position",    { object.Position.x, object.Position.y, object.Position.z } },
				{ "RotationDeg", { object.RotationDeg.x, object.RotationDeg.y, object.RotationDeg.z } },
				{ "Scale",       { object.Scale.x, object.Scale.y, object.Scale.z } },
			});
		}

		nlohmann::json enemy_spawns = nlohmann::json::array();
		for (const LevelEnemySpawnDesc& spawn : Desc.EnemySpawns)
		{
			enemy_spawns.push_back({
				{ "DefinitionId", spawn.DefinitionId },
				{ "InstanceName", spawn.InstanceName },
				{ "Position",     { spawn.Position.x, spawn.Position.y, spawn.Position.z } },
				{ "RotationDeg",  { spawn.RotationDeg.x, spawn.RotationDeg.y, spawn.RotationDeg.z } },
				{ "Scale",        { spawn.Scale.x, spawn.Scale.y, spawn.Scale.z } },
			});
		}

		nlohmann::json out;
		out["Objects"]     = objects;
		out["EnemySpawns"] = enemy_spawns;

		if (Desc.PlayerSpawn.HasValue)
		{
			out["PlayerSpawn"] = {
				{ "Position", { Desc.PlayerSpawn.Position.x, Desc.PlayerSpawn.Position.y, Desc.PlayerSpawn.Position.z } },
				{ "YawDeg",   Desc.PlayerSpawn.YawDeg }
			};
		}

		if (Desc.BossSpawn.HasValue)
		{
			out["BossSpawn"] = {
				{ "Position", { Desc.BossSpawn.Position.x, Desc.BossSpawn.Position.y, Desc.BossSpawn.Position.z } },
				{ "YawDeg",   Desc.BossSpawn.YawDeg }
			};
		}

		return out;
	}

	// レベルJSONを読み込む(ファイルが無い・壊れている場合は空のLevelDesc).
	inline LevelDesc LoadFromFile(const std::filesystem::path& Path)
	{
		std::ifstream file(Path);
		if (!file) { return LevelDesc{}; }

		try {
			nlohmann::json root{};
			file >> root;
			return Parse(root);
		}
		catch (const nlohmann::json::exception&) {
			return LevelDesc{}; // 壊れたJSONは空として扱い、クラッシュしない.
		}
	}

	// レベルJSONへ書き出す.
	inline bool WriteToFile(const std::filesystem::path& Path, const LevelDesc& Desc)
	{
		std::ofstream file(Path);
		if (!file) { return false; }
		file << Serialize(Desc).dump(2);
		return true;
	}

} // namespace LevelData
