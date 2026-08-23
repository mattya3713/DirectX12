#pragma once

#include <cmath>
#include <filesystem>
#include <fstream>
#include <set>
#include <string>
#include <vector>

#include "00_Game/00_Scene/Level/LevelData.h"
#include "00_Game/50_Enemy/Definition/EnemyDefinitionCatalog.h"
#include "json/json.hpp"

/**********************************************************************************
* @author    : 朱雀 (Suzaku / 閃斬 Production Loop Coder).
* @date      : 2026/08/24.
* @brief     : レベルJSONの静的検証(lint). 記述ミスをロード前に一括検出し
*            : Error(ロード失敗につながる)/Warning(動くが怪しい)の2段階で報告する.
*            : 検証は読み取りのみで既存ロード挙動を変えない. ImGui非依存のため
*            : 単体テストからも直接呼べる.
**********************************************************************************/

struct LevelLintIssue
{
	enum class Severity : int
	{
		Error   = 0, // ロード失敗につながる問題.
		Warning = 1, // 動くが怪しい値.
	};

	Severity    Severity = Severity::Error;
	std::string Message;
};

struct LevelLintResult
{
	std::vector<LevelLintIssue> Issues;

	bool HasError() const noexcept
	{
		for (const LevelLintIssue& issue : Issues)
		{
			if (issue.Severity == LevelLintIssue::Severity::Error) { return true; }
		}
		return false;
	}
};

namespace LevelLint {

	// パース済みJSONを検証する(LintFileの前方宣言. 実体は下記).
	inline LevelLintResult Lint(const nlohmann::json& Root, const EnemyDefinitionCatalog& Catalog);

	// レベルJSONファイル全体を検証する(構文→各エントリ).
	inline LevelLintResult LintFile(const std::filesystem::path& Path, const EnemyDefinitionCatalog& Catalog)
	{
		LevelLintResult result{};

		std::ifstream file(Path);
		if (!file)
		{
			result.Issues.push_back({ LevelLintIssue::Severity::Error, "ファイルが開けません: " + Path.string() });
			return result;
		}

		nlohmann::json root{};
		try {
			file >> root;
		}
		catch (const nlohmann::json::exception& e) {
			result.Issues.push_back({ LevelLintIssue::Severity::Error,
				std::string("JSON構文エラー: ") + e.what() });
			return result;
		}

		// 表現できない数値リテラル(1e999等)はnlohmannがdiscarded値にするため全体をError扱い.
		if (root.is_discarded())
		{
			result.Issues.push_back({ LevelLintIssue::Severity::Error,
				"表現できない数値リテラルが含まれています" });
			return result;
		}

		return Lint(root, Catalog);
	}

	// パース済みJSONを検証する.
	inline LevelLintResult Lint(const nlohmann::json& Root, const EnemyDefinitionCatalog& Catalog)
	{
		LevelLintResult result{};

		const auto add_warning = [&](std::string Message) {
			result.Issues.push_back({ LevelLintIssue::Severity::Warning, std::move(Message) });
		};
		const auto add_error = [&](std::string Message) {
			result.Issues.push_back({ LevelLintIssue::Severity::Error, std::move(Message) });
		};

		if (!Root.is_object())
		{
			add_error("ルートがオブジェクトではありません");
			return result;
		}

		constexpr float kFarPositionLimit = 10000.0f; // スポーン座標の警告しきい値(仮値).

		// ----- 静的オブジェクト -----
		if (Root.contains("Objects"))
		{
			const nlohmann::json& objects = Root["Objects"];
			if (!objects.is_array()) { add_error("Objectsは配列である必要があります"); }
			else
			{
				int index = 0;
				for (const nlohmann::json& entry : objects)
				{
					if (!entry.is_object())
					{
						add_warning("Objects[" + std::to_string(index) + "] がオブジェクトではありません");
						++index;
						continue;
					}

					const std::string mstc = entry.value("Mstc", std::string());
					if (mstc.empty())
					{
						add_warning("Objects[" + std::to_string(index) + "] のMstcが空です(ロード時にスキップされます)");
					}

					// 座標の異常値(極端に遠いスポーンは警告扱い).
					const auto position = entry.value("Position", std::vector<float>{});
					for (const float value : position)
					{
						if (std::fabs(value) > kFarPositionLimit)
						{
							add_warning("Objects[" + std::to_string(index) + "] の座標が異常に遠い位置です");
							break;
						}
					}

					++index;
				}
			}
		}

		// ----- 敵スポーン -----
		std::set<std::string> used_instance_names;

		if (Root.contains("EnemySpawns"))
		{
			const nlohmann::json& spawns = Root["EnemySpawns"];
			if (!spawns.is_array()) { add_error("EnemySpawnsは配列である必要があります"); }
			else
			{
				int index = 0;
				for (const nlohmann::json& entry : spawns)
				{
					if (!entry.is_object())
					{
						add_warning("EnemySpawns[" + std::to_string(index) + "] がオブジェクトではありません");
						++index;
						continue;
					}

					const std::string id = entry.value("DefinitionId", std::string());
					if (id.empty())
					{
						add_error("EnemySpawns[" + std::to_string(index) + "] のDefinitionIdが空です");
						++index;
						continue;
					}

					if (!Catalog.Contains(id))
					{
						add_warning("EnemySpawns[" + std::to_string(index) + "] の敵ID「" + id +
							"」はカタログ未登録です(ロード時にスキップされます)");
					}

					const auto max_hp = entry.contains("MaxHP") ? entry["MaxHP"] : nlohmann::json(nullptr);
					(void)max_hp; // 敵定義側の数値はカタログ管理のためここでは見ない.

					const auto position = entry.value("Position", std::vector<float>{});
					for (const float value : position)
					{
						if (!std::isfinite(value))
						{
							add_error("EnemySpawns[" + std::to_string(index) + "] のPositionに非有限値(NaN/inf)があります");
							break;
						}
						if (std::fabs(value) > kFarPositionLimit)
						{
							add_warning("EnemySpawns[" + std::to_string(index) + "] の座標が異常に遠い位置です");
							break;
						}
					}

					const auto scale = entry.value("Scale", std::vector<float>{});
					for (const float value : scale)
					{
						if (value < 0.0f)
						{
							add_warning("EnemySpawns[" + std::to_string(index) + "] のScaleが負です");
							break;
						}
					}

					const std::string instance_name = entry.value("InstanceName", std::string());
					if (!instance_name.empty())
					{
						if (used_instance_names.contains(instance_name))
						{
							add_warning("EnemySpawns[" + std::to_string(index) + "] のInstanceName「" +
								instance_name + "」が重複しています");
						}
						else
						{
							used_instance_names.insert(instance_name);
						}
					}

					++index;
				}
			}
		}

		return result;
	}

} // namespace LevelLint
