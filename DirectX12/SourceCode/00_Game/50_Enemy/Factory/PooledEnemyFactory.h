#pragma once

#include <memory>
#include <string>

#include "00_Game/00_Scene/Level/LevelData.h"
#include "00_Game/10_Object/10_MeshObject/00_Character/10_Enemy/Enemy.h"
#include "00_Game/50_Enemy/Definition/EnemyDefinitionCatalog.h"
#include "99_Utility/ObjectPool/ReusableSlotPool.h"

/**********************************************************************************
* @author    : 朱雀 (Suzaku / 閃斬 Production Loop Coder).
* @date      : 2026/08/23.
* @brief     : Enemyをnew/deleteせず再利用するプール付きFactory.
*            : Spawn時にHP/速度/Transform/State/Collider/ヒット履歴を完全リセットし、
*            : 別定義の個体として再初期化できる。返却はReturn()で行い、
*            : 二重返却・所有権外ポインタ・容量超過はnullptr/falseで安全に検出する.
*            : 統計(Created/Reused/Active/Returned)をDEBUG確認できる.
*            : スレッドセーフではない. MainScene接続は別タスク.
**********************************************************************************/

class PooledEnemyFactory final
{
public:
	struct Stats
	{
		size_t Created  = 0; // 累積新規生成数.
		size_t Reused   = 0; // 累積再利用数.
		size_t Active   = 0; // 現在使用中の数.
		size_t Returned = 0; // 累積返却数.
	};

	explicit PooledEnemyFactory(const EnemyDefinitionCatalog& Catalog, size_t MaxActive = 32) noexcept
		: m_Catalog(Catalog)
		, m_Pool(MaxActive)
	{
	}

	PooledEnemyFactory(const PooledEnemyFactory&)            = delete;
	PooledEnemyFactory& operator=(const PooledEnemyFactory&) = delete;

	// 定義IDから生成または再利用する(未知ID・容量超過はnullptr).
	Enemy* Spawn(const std::string& DefinitionId, const Transform& SpawnTransform)
	{
		const EnemyDefinition* p_definition = m_Catalog.Find(DefinitionId);
		if (!p_definition || !p_definition->IsValid()) { return nullptr; }
		if (!m_Pool.HasFreeCapacity()) { return nullptr; }

		Enemy* p_enemy = m_Pool.Acquire([&]() {
			return std::make_unique<Enemy>();
		});
		if (!p_enemy) { return nullptr; }

		Reinitialize(*p_enemy, *p_definition, SpawnTransform);

		return p_enemy;
	}

	// 使用済みEnemyを返却する(false=二重返却または所有権外ポインタ. 状態は変更しない).
	bool Return(Enemy*& pEnemy)
	{
		if (!pEnemy || !m_Pool.Owns(pEnemy)) { return false; }
		if (!m_Pool.Return(pEnemy)) { return false; }

		pEnemy = nullptr;
		return true;
	}

	Stats GetStats() const noexcept
	{
		return { m_Pool.CreatedCount(), m_Pool.ReusedCount(), m_Pool.ActiveCount(), m_Pool.ReturnedCount() };
	}

private:
	// 再利用時の完全リセット(HP/速度/Transform/State/Collider/ヒット履歴).
	void Reinitialize(Enemy& Target, const EnemyDefinition& Definition, const Transform& SpawnTransform) noexcept
	{
		// 定義適用(移動速度・最大HP+現在HPリセット).
		Target.ApplyTuning(Definition.MoveSpeed, Definition.MaxHP);

		// Collider状態を構築直後へ戻す(攻撃無効・被弾有効).
		Target.SetAttackColliderActive(false);
		Target.SetDamageColliderActive(true);

		// 同一スイング重複ヒット履歴の消去(前個体の攻撃判定を引き継がない).
		Target.ClearHitHistory();

		// State初期化(Idleへ戻す. Enter内でクリップ/ポーズが設定される).
		Target.ChangeState(EnemyState::eID::Idle);

		// Transform設定.
		Target.SetTransform(SpawnTransform);
	}

	const EnemyDefinitionCatalog& m_Catalog;
	ReusableSlotPool<Enemy> m_Pool;
};
