#pragma once

#include <cmath>
#include <filesystem>
#include <fstream>
#include <set>
#include <string>
#include <vector>

#include "json/json.hpp"

#include "00_Game/50_Enemy/Definition/EnemyDefinitionCatalog.h"

/**********************************************************************************
* @author    : 朱雀 (Suzaku / 閃斬 Production Loop Coder).
* @date      : 2026/08/24.
* @brief     : レベルJSONの静的検証(lint). 読み取りのみでロード挙動は変えず、
*            : 記述ミスをError(個体落ち・データ不備確定)/Warning(動くが怪しい)
*            : の2段階で報告する. 実行時の「ログして個体スキップ」
*            : (EnemySpawnPlanner)とは役割分担.
*            : 検証は生のJSONに対して行う(LevelData::Parseは欠損を黙って補完・
*            : スキップするため、そこでは見えないミスを拾える).
**********************************************************************************/

// 検証結果1件分.
struct LevelLintIssue
{
	bool        IsError = false; // true=Error(データ不備確定) / false=Warning(動くが怪しい).
	std::string Path;            // 発生位置(例: "EnemySpawns[2].DefinitionId").
	std::string Message;
};

// 検証レポート(問題0件ならIsClean()).
struct LevelLintReport
{
	std::vector<LevelLintIssue> Issues;

	bool IsClean() const noexcept { return Issues.empty(); }
	size_t ErrorCount() const noexcept
	{
		size_t count = 0;
		for (const LevelLintIssue& issue : Issues) { if (issue.IsError) { ++count; } }
		return count;
	}
	size_t WarningCount() const noexcept { return Issues.size() - ErrorCount(); }
};

namespace LevelLint {

	// 検証オプション(テストから差し替え可能にするためパラメータ化).
	struct Options
	{
		const EnemyDefinitionCatalog* Catalog = nullptr; // nullptrなら未知ID検証をスキップ.
		std::filesystem::path MstcDir       = "Data\\Model\\mmdl\\mstc"; // モデル存在確認の基準ディレクトリ.
		float                 FarPositionSq = 25000000.0f; // スポーン座標の警告閾値(5000の2乗).
	};

namespace {

	inline void AddIssue(LevelLintReport& Report, bool IsError, std::string Path, std::string Message)
	{
		Report.Issues.push_back({ IsError, std::move(Path), std::move(Message) });
	}

	// 数値配列3要素として成立しているか(成立しない場合は理由付きでfalse).
	inline bool GetVec3(const nlohmann::json& Entry, const char* Key,
		float& OutX, float& OutY, float& OutZ, std::string& OutReason)
	{
		if (!Entry.contains(Key)) { return true; } // 欠損はParse側の既定値に任せる(検証対象外).

		const nlohmann::json& value = Entry.at(Key);
		if (!value.is_array() || value.size() < 3) {
			OutReason = std::string(Key) + " は3要素の数値配列である必要があります";
			return false;
		}

		for (const nlohmann::json& element : value) {
			if (!element.is_number()) {
				OutReason = std::string(Key) + " に数値以外が含まれています";
				return false;
			}
		}

		OutX = value[0].get<float>();
		OutY = value[1].get<float>();
		OutZ = value[2].get<float>();

		if (!std::isfinite(OutX) || !std::isfinite(OutY) || !std::isfinite(OutZ)) {
			OutReason = std::string(Key) + " に非有限値(NaN/Inf)があります";
			return false;
		}

		return true;
	}

	// Position/RotationDeg/Scale共通のTransform検証.
	inline void CheckTransform(const nlohmann::json& Entry, const std::string& BasePath, LevelLintReport& Report)
	{
		float x = 0.0f, y = 0.0f, z = 0.0f;

		for (const char* key : { "Position", "RotationDeg", "Scale" }) {
			std::string reason;
			if (!GetVec3(Entry, key, x, y, z, reason)) {
				AddIssue(Report, true, BasePath + "." + key, reason);
				continue;
			}

			if (std::strcmp(key, "Scale") == 0 && (x < 0.0f || y < 0.0f || z < 0.0f)) {
				AddIssue(Report, false, BasePath + ".Scale", "負のスケールが指定されています(意図した見た目にならない可能性)");
			}
		}
	}

} // namespace

inline LevelLintReport Run(const nlohmann::json& Root, const Options& Options_)
{
	LevelLintReport report{};

	if (!Root.is_object()) {
		AddIssue(report, true, "(root)", "ルートはJSONオブジェクトである必要があります");
		return report;
	}

	// ----- 静的オブジェクト -----
	if (Root.contains("Objects")) {
		const nlohmann::json& objects = Root["Objects"];
		if (!objects.is_array()) {
			AddIssue(report, true, "Objects", "配列である必要があります");
		}
		else {
			for (size_t i = 0; i < objects.size(); ++i) {
				const nlohmann::json& entry = objects[i];
				const std::string base = "Objects[" + std::to_string(i) + "]";

				if (!entry.is_object()) {
					AddIssue(report, true, base, "オブジェクトである必要があります");
					continue;
				}

				const std::string mstc = entry.value("Mstc", std::string());
				if (mstc.empty()) {
					// Parse段階で無視されるため、ここでは確実にデータ不備として通知する.
					AddIssue(report, true, base + ".Mstc", "必須フィールド Mstc が空です(このエントリは読込時に破棄されます)");
				}
				else if (!Options_.MstcDir.empty()
					&& !std::filesystem::exists(std::filesystem::path(Options_.MstcDir) / mstc)) {
					AddIssue(report, false, base + ".Mstc", "モデルファイルが見つかりません: " + mstc);
				}

				CheckTransform(entry, base, report);
			}
		}
	}

	// ----- 敵スポーン -----
	if (Root.contains("EnemySpawns")) {
		const nlohmann::json& spawns = Root["EnemySpawns"];
		if (!spawns.is_array()) {
			AddIssue(report, true, "EnemySpawns", "配列である必要があります");
		}
		else {
			std::set<std::string> used_names;

			for (size_t i = 0; i < spawns.size(); ++i) {
				const nlohmann::json& entry = spawns[i];
				const std::string base = "EnemySpawns[" + std::to_string(i) + "]";

				if (!entry.is_object()) {
					AddIssue(report, true, base, "オブジェクトである必要があります");
					continue;
				}

				const std::string definition_id = entry.value("DefinitionId", std::string());
				if (definition_id.empty()) {
					AddIssue(report, true, base + ".DefinitionId", "必須フィールド DefinitionId が空です(このエントリは読込時に破棄されます)");
				}
				else if (Options_.Catalog != nullptr && !Options_.Catalog->Contains(definition_id)) {
					AddIssue(report, true, base + ".DefinitionId", "未登録の敵IDです: " + definition_id);
				}

				const std::string instance_name = entry.value("InstanceName", std::string());
				if (!instance_name.empty()) {
					if (used_names.contains(instance_name)) {
						AddIssue(report, false, base + ".InstanceName",
							"InstanceName が重複しています: " + instance_name + "(2体目以降は読込時に除外されます)");
					}
					else {
						used_names.insert(instance_name);
					}
				}

				CheckTransform(entry, base, report);

				// スポーン座標の異常値(極端に遠い)はWarning扱い.
				if (entry.contains("Position") && entry["Position"].is_array() && entry["Position"].size() >= 3
					&& entry["Position"][0].is_number() && entry["Position"][1].is_number() && entry["Position"][2].is_number()) {
					const float x = entry["Position"][0].get<float>();
					const float y = entry["Position"][1].get<float>();
					const float z = entry["Position"][2].get<float>();
					if (std::isfinite(x) && std::isfinite(y) && std::isfinite(z)
						&& (x * x + y * y + z * z) > Options_.FarPositionSq) {
						AddIssue(report, false, base + ".Position", "スポーン座標が原点から大きく離れています(到達不能な配置の可能性)");
					}
				}
			}
		}
	}

	// ----- Player/Bossスポーン -----
	for (const char* key : { "PlayerSpawn", "BossSpawn" }) {
		if (!Root.contains(key)) { continue; }

		const nlohmann::json& entry = Root[key];
		const std::string base = key;

		if (!entry.is_object()) {
			AddIssue(report, true, base, "オブジェクトである必要があります");
			continue;
		}

		CheckTransform(entry, base, report);

		if (entry.contains("YawDeg")) {
			const nlohmann::json& yaw = entry["YawDeg"];
			if (!yaw.is_number() || !std::isfinite(yaw.get<float>())) {
				AddIssue(report, true, base + ".YawDeg", "数値である必要があります");
			}
		}
	}

	return report;
}

// ファイルに対して検証する(構文エラー・読込失敗はError1件として報告).
inline LevelLintReport RunFile(const std::filesystem::path& Path, const Options& Options_ = {})
{
	LevelLintReport report{};

	std::ifstream file(Path);
	if (!file) {
		AddIssue(report, true, "(file)", "ファイルを開けません: " + Path.string());
		return report;
	}

	nlohmann::json root{};
	try {
		file >> root;
	}
	catch (const nlohmann::json::exception&) {
		AddIssue(report, true, "(syntax)", "JSON構文エラーです: " + Path.string());
		return report;
	}

	// 環境によっては例外ではなくdiscardedが返る場合があるため両方で判定する.
	if (root.is_discarded()) {
		AddIssue(report, true, "(syntax)", "JSON構文エラーです: " + Path.string());
		return report;
	}

	return Run(root, Options_);
}

} // namespace LevelLint
