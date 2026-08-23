#pragma once

#include <algorithm>

#include "99_Utility/HealthSystem/IHealthSystem.h"

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/10.
* @brief     : IHealthSystemの具象実装. HPデータ・ダメージ処理・コールバックをすべて
*            : このクラス内で完結させる. 使う側(Character等)はこれをメンバとして持つ
*            : (継承ではなくコンポジション. GameObjectがTransformを持つのと同じ形).
**********************************************************************************/

class HealthSystem final : public IHealthSystem
{
public:
	explicit HealthSystem(float MaxHP = 100.0f) noexcept;
	~HealthSystem() override = default;

	// 最大HPの取得.
	float GetMaxHP() const noexcept override { return m_MaxHP; }
	// 現在HPの取得.
	float GetHP() const noexcept override { return m_HP; }
	// 生存しているか.
	bool IsAlive() const noexcept override { return m_HP > 0.0f; }
	// ダメージを与える(生存→死亡に変化した瞬間のみOnDeathを呼ぶ).
	void ApplyDamage(float DamageAmount) override;

	// ダメージを受けた時のコールバックを設定する.
	void SetOnDamage(DamageCallback Callback) override { m_OnDamage = std::move(Callback); }
	// 死亡した瞬間のコールバックを設定する.
	void SetOnDeath(DeathCallback Callback) override { m_OnDeath = std::move(Callback); }

	// 最大HPと現在HPを設定する(EnemyFactory等の生成時初期化用).
	void SetMaxHP(float MaxHP) noexcept { m_MaxHP = MaxHP; m_HP = (std::min)(m_HP, MaxHP); }
	void SetHP(float HP) noexcept { m_HP = HP; }

private:
	float m_MaxHP;	// 最大HP.
	float m_HP;		// 現在HP.

	DamageCallback	m_OnDamage;	// ダメージを受けた時に呼ばれる.
	DeathCallback	m_OnDeath;	// 死亡した瞬間に呼ばれる.
};
