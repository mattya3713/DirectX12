#pragma once

#include "00_Game/05_Object/00_Base/GameObject.h"
#include "00_Game/05_Object/10_Character/CharacterAccessKeys.h"
#include "99_Utility/HealthSystem/HealthSystem.h"

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/10.
* @brief     : ゲーム内キャラクターの基底クラス. GameObjectを継承する具象クラス.
*            : HPはHealthSystemをメンバとして持つ(コンポジション. GameObjectがTransformを
*            : メンバに持つのと同じ形. IHealthSystemを直接継承しない理由はHealthSystem.h参照).
*            : 静的な小道具やトリガー等、生命・行動を持たないGameObjectと区別するための層.
**********************************************************************************/

class Character : public GameObject
{
public:
	Character();
	virtual ~Character();

	Character(const Character&)            = delete;
	Character& operator=(const Character&) = delete;
	Character(Character&&)                 = delete;
	Character& operator=(Character&&)      = delete;

public: // HealthSystemへのアクセス.

	// HealthSystem本体の読み取り専用アクセス(書き換えはCharacter経由の関数のみ許可する).
	const HealthSystem& GetHealth() const noexcept { return m_Health; }

	// よく使うものは薄いフォワーダーとして直接公開する.
	float GetMaxHP() const noexcept { return m_Health.GetMaxHP(); }
	float GetHP() const noexcept { return m_Health.GetHP(); }
	bool IsAlive() const noexcept { return m_Health.IsAlive(); }

	// ダメージを与える(CharacterAccess::DamageKeyに登録された攻撃者クラスのみ呼べる).
	void ApplyDamage(float DamageAmount, CharacterAccess::DamageKey) { m_Health.ApplyDamage(DamageAmount); }

	// ダメージ/死亡コールバックの登録(誰でも購読してよいので鍵は要求しない).
	void SetOnDamage(HealthSystem::DamageCallback Callback) { m_Health.SetOnDamage(std::move(Callback)); }
	void SetOnDeath(HealthSystem::DeathCallback Callback) { m_Health.SetOnDeath(std::move(Callback)); }

protected:
	HealthSystem m_Health; // HP・ダメージ処理・コールバック.
};
