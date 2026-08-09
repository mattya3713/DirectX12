#pragma once

#include "00_Game/05_Object/00_Base/GameObject.h"
#include "00_Game/05_Object/00_Base/IHealthSystem.h"

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/10.
* @brief     : ゲーム内キャラクターの基底クラス. GameObjectを継承し、IHealthSystemを
*            : 実装する具象クラス(HPの実体はここに置く. Senzanの`Character`を参考).
*            : 静的な小道具やトリガー等、生命・行動を持たないGameObjectと区別するための層.
**********************************************************************************/

class Character : public GameObject, public IHealthSystem
{
public:
	Character();
	virtual ~Character();

	Character(const Character&)            = delete;
	Character& operator=(const Character&) = delete;
	Character(Character&&)                 = delete;
	Character& operator=(Character&&)      = delete;

public: // IHealthSystem実装.

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

protected:
	float m_MaxHP;	// 最大HP.
	float m_HP;		// 現在HP.

	DamageCallback	m_OnDamage;	// ダメージを受けた時に呼ばれる.
	DeathCallback	m_OnDeath;	// 死亡した瞬間に呼ばれる.
};
