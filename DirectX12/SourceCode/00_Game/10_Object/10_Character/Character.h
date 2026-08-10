#pragma once

#include "00_Game/10_Object/05_MeshObject/MeshObject.h"
#include "00_Game/10_Object/10_Character/CharacterAccessKeys.h"
#include "99_Utility/HealthSystem/HealthSystem.h"

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/10.
* @brief     : ゲーム内キャラクターの基底クラス. MeshObjectを継承する具象クラス
*            : (見た目(PMXMesh)を持てる). HPはHealthSystemをメンバとして持つ
*            : (コンポジション. IHealthSystemを直接継承しない理由はHealthSystem.h参照).
*            : 静的な小道具やトリガー等、生命・行動を持たないGameObjectと区別するための層.
**********************************************************************************/

class Character : public MeshObject
{
public:
	Character();
	virtual ~Character();

	Character(const Character&)            = delete;
	Character& operator=(const Character&) = delete;
	Character(Character&&)                 = delete;
	Character& operator=(Character&&)      = delete;

public: 
	// HP関連の情報取得.
	const HealthSystem& GetHealth() const noexcept { return m_Health; }

protected:

	// ダメージ/死亡コールバックの登録
	void SetOnDamage(HealthSystem::DamageCallback Callback) { m_Health.SetOnDamage(std::move(Callback)); }
	void SetOnDeath(HealthSystem::DeathCallback Callback) { m_Health.SetOnDeath(std::move(Callback)); }

protected:
	HealthSystem m_Health; // HP・ダメージ処理・コールバック.
};
