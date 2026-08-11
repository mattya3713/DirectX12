#pragma once

#include "99_Utility/StateMachine/StateBase.h"
#include "00_Game/10_Object/10_MeshObject/00_Character/10_Enemy/State/EnemyStateID.h"

class Enemy;

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/11.
* @brief     : Enemyのステートの基底クラス. StateBase<Enemy>を継承し、
*            : ターゲット(Player)との距離・角度計算等の共通処理を追加する.
**********************************************************************************/

class EnemyStateBase : public StateBase<Enemy>
{
public:
	explicit EnemyStateBase(Enemy* pOwner) noexcept;
	~EnemyStateBase() override = default;

	// ステートIDの取得.
	virtual EnemyState::eID GetStateID() const = 0;

protected:
	// オーナー(Enemy)の取得.
	Enemy* GetEnemy() const noexcept { return m_pOwner; }

	// 自分からターゲット(Player)までの水平距離(XZ平面).
	float DistanceToTargetXZ() const noexcept;

	// 自分からターゲット(Player)へ向く角度(度. atan2f(x,z)基準、PlayerStateBaseと同じ規約).
	float AngleToTargetDeg() const noexcept;
};
