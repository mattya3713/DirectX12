#pragma once

#include "00_Game/05_Object/00_Base/GameObject.h"
#include "00_Game/05_Object/00_Base/HealthSystem.h"

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

	// HealthSystem本体の取得(コールバック登録等、詳細な操作はこちら経由).
	HealthSystem& GetHealth() noexcept { return m_Health; }
	const HealthSystem& GetHealth() const noexcept { return m_Health; }

	// よく使うものは薄いフォワーダーとして直接公開する.
	float GetMaxHP() const noexcept { return m_Health.GetMaxHP(); }
	float GetHP() const noexcept { return m_Health.GetHP(); }
	bool IsAlive() const noexcept { return m_Health.IsAlive(); }
	void ApplyDamage(float DamageAmount) { m_Health.ApplyDamage(DamageAmount); }

protected:
	HealthSystem m_Health; // HP・ダメージ処理・コールバック.
};
