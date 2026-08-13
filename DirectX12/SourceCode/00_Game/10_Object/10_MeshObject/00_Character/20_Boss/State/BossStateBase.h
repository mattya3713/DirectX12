#pragma once

#include "99_Utility/StateMachine/StateBase.h"
#include "00_Game/10_Object/10_MeshObject/00_Character/20_Boss/State/BossStateID.h"

class Boss;

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/13.
* @brief     : Bossのステートの基底クラス. StateBase<Boss>を継承し、
*            : ターゲット(Player)との距離・角度計算等の共通処理を追加する
*            : (EnemyStateBaseと同じ役割. BossはEnemyの索敵AI用フィールドを
*            : そのまま使うが、ステート自体はBoss専用のStateMachine<Boss>で動くため
*            : StateBase<Enemy>ではなくStateBase<Boss>を継承する).
**********************************************************************************/

class BossStateBase : public StateBase<Boss>
{
public:
	explicit BossStateBase(Boss* pOwner) noexcept;
	~BossStateBase() override = default;

	// ステートIDの取得.
	virtual BossState::eID GetStateID() const = 0;

protected:
	// オーナー(Boss)の取得.
	Boss* GetBoss() const noexcept { return m_pOwner; }

	// 自分からターゲット(Player)までの水平距離(XZ平面).
	float DistanceToTargetXZ() const noexcept;

	// 自分からターゲット(Player)へ向く角度(度. atan2f(x,z)基準、PlayerStateBaseと同じ規約).
	float AngleToTargetDeg() const noexcept;

	// Bossのメッシュへ名前でクリップを適用する.
	void ApplyNamedClip(const char* ClipName) const;
};
