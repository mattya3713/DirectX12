#pragma once

#include "00_Game/10_Object/10_MeshObject/00_Character/10_Enemy/Enemy.h"
#include "00_Game/10_Object/10_MeshObject/00_Character/20_Boss/State/BossStateID.h"
#include "99_Utility/Ragdoll/RagdollComponent.h"
#include "99_Utility/StateMachine/StateMachine.h"

class BossCombatView;

// EventBus用デモイベント(Boss死亡時に発行される. UI/サウンド/実績等の購読想定).
struct BossDefeatedEvent
{
	class Boss* Who = nullptr;
};

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/13.
* @brief     : ボスキャラクラス. Enemyの索敵AI用フィールド(TargetPos/AggroRange等)は
*            : そのまま使うが、ステート自体はBoss専用のStateMachine<Boss>で動かす
*            : (将来Boss専用の攻撃パターンを増やしていくため、EnemyStateとは別の
*            : BossState/BossStateIDを持つ. Enemy側のStateMachine<Enemy>は
*            : 未使用のまま残る(ChangeState(EnemyState::eID)はBossからは隠蔽される)).
*            : 現時点ではIdle/Move/Attack/Deadの最小構成(パリィ・専用攻撃パターン・
*            : CombatCoordinator連携は未実装).
**********************************************************************************/

class Boss final : public Enemy
{
	friend class BossCombatView; // CombatCoordinator用の限定公開Viewにだけリアクション開始を許可する.

public:
	Boss();
	~Boss() override;

	// 毎フレーム更新(Boss専用StateMachineのUpdate/LateUpdateを順に呼ぶ).
	void Update() override;

public:
	// ステートを変更する(BossState::eIDから対応するステートを生成しStateMachineへ渡す).
	void ChangeState(BossState::eID Id);

	// 現在ステートIDの取得(デバッグ表示等、外部からの参照用).
	BossState::eID GetCurrentStateID() const noexcept { return m_CurrentStateID; }


	bool ActivateDeathRagdoll();

	RagdollComponent& GetRagdoll() noexcept { return m_Ragdoll; }
	const RagdollComponent& GetRagdoll() const noexcept { return m_Ragdoll; }
private:
	void EnterParryReaction(const DirectX::XMFLOAT3& TargetPosition, float TargetYawDeg, float Duration);

	RagdollComponent m_Ragdoll; // 死亡時ラグドール.
	StateMachine<Boss> m_StateMachine;                        // 現在ステートの保持・更新.
	BossState::eID      m_CurrentStateID = BossState::eID::None;
};
