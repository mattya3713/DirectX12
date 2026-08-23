#pragma once

#include <cmath>
#include <string>
#include <unordered_set>
#include <vector>

#include "00_Game/00_Scene/Level/LevelData.h"
#include "00_Game/50_Enemy/Definition/EnemyDefinitionCatalog.h"
#include "99_Utility/Transform/Transform.h"

/**********************************************************************************
* @author    : 朱雀 (Suzaku / 閃斬 Production Loop Coder).
* @date      : 2026/08/23.
* @brief     : レベルの敵スポーン定義(LevelEnemySpawnDesc一覧)を、生成可能な
*            : 計画(EnemySpawnPlan)へ変換する純粋な計画層.
*            : Enemy実体は生成せず所有権も持たない。MainScene等の後続は
*            : 計画の先頭から順にEnemyFactory::Createへ渡すだけ.
*            : 同一入力から常に同一の計画を返す(決定的. Level順序を保持).
**********************************************************************************/

// 計画済みスポーン1体分(カタログ定義への非所有参照+初期Transform).
// 生成は呼び出し側がEnemyFactory::CreateへDefinition/Transformを渡して行う.
struct EnemySpawnPlanEntry
{
	const EnemyDefinition* Definition = nullptr; // カタログ内の有効な定義.
	Transform              Transform{};          // スポーン時のTransform.
	std::string            InstanceName;         // デバッグ表示用(空可).
	size_t                 SourceIndex = 0;      // 元Level配列での位置(デバッグ用).
};

// 除外されたスポーンとその理由(デバッグ/テスト用).
struct EnemySpawnIssue
{
	size_t      SourceIndex  = 0;
	std::string DefinitionId;
	std::string Reason; // "unknown_definition" / "invalid_transform" / "duplicate_instance_name".
};

struct EnemySpawnPlan
{
	std::vector<EnemySpawnPlanEntry> Entries; // 生成可能なスポーン(Level順序を保持).
	std::vector<EnemySpawnIssue>     Issues;  // 除外されたスポーンと理由.

	bool Empty() const noexcept { return Entries.empty(); }
	size_t Count() const noexcept { return Entries.size(); }
};

class EnemySpawnPlanner final
{
public:
	explicit EnemySpawnPlanner(const EnemyDefinitionCatalog& Catalog) noexcept
		: m_Catalog(Catalog)
	{
	}

	EnemySpawnPlanner(const EnemySpawnPlanner&)            = delete;
	EnemySpawnPlanner& operator=(const EnemySpawnPlanner&) = delete;

	// 敵スポーン定義一覧から生成計画を作る(決定的. Level順序を保持).
	[[nodiscard]] EnemySpawnPlan Plan(const std::vector<LevelEnemySpawnDesc>& Spawns) const
	{
		EnemySpawnPlan plan{};

		std::unordered_set<std::string> used_names;

		for (size_t i = 0; i < Spawns.size(); ++i)
		{
			const LevelEnemySpawnDesc& spawn = Spawns[i];

			const auto add_issue = [&](std::string Reason) {
				plan.Issues.push_back({ i, spawn.DefinitionId, std::move(Reason) });
			};

			// 未知ID(カタログ未登録)は生成不能のため除外.
			if (!m_Catalog.Contains(spawn.DefinitionId))
			{
				add_issue("unknown_definition");
				continue;
			}

			// 不正Transform(非有限値)は除外.
			if (!IsFinite(spawn.Position) || !IsFinite(spawn.RotationDeg) || !IsFinite(spawn.Scale))
			{
				add_issue("invalid_transform");
				continue;
			}

			// 重複InstanceName(空は除外対象外)は2体目以降を除外.
			if (!spawn.InstanceName.empty())
			{
				if (used_names.contains(spawn.InstanceName))
				{
					add_issue("duplicate_instance_name");
					continue;
				}
				used_names.insert(spawn.InstanceName);
			}

			EnemySpawnPlanEntry entry{};
			entry.Definition   = m_Catalog.Find(spawn.DefinitionId);
			entry.Transform    = MakeTransform(spawn.Position, spawn.RotationDeg, spawn.Scale);
			entry.InstanceName = spawn.InstanceName;
			entry.SourceIndex  = i;
			plan.Entries.push_back(std::move(entry));
		}

		return plan;
	}

private:
	static bool IsFinite(const DirectX::XMFLOAT3& Value) noexcept
	{
		return std::isfinite(Value.x) && std::isfinite(Value.y) && std::isfinite(Value.z);
	}

	static Transform MakeTransform(
		const DirectX::XMFLOAT3& Position,
		const DirectX::XMFLOAT3& RotationDeg,
		const DirectX::XMFLOAT3& Scale) noexcept
	{
		constexpr float deg_to_rad = 3.14159265358979f / 180.0f;
		Transform transform{};
		transform.Position             = Position;
		transform.Rotation.x           = RotationDeg.x * deg_to_rad;
		transform.Rotation.y           = RotationDeg.y * deg_to_rad;
		transform.Rotation.z           = RotationDeg.z * deg_to_rad;
		transform.Scale                = Scale;
		return transform;
	}

	const EnemyDefinitionCatalog& m_Catalog;
};
