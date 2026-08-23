#pragma once

#include <memory>

#include <DirectXMath.h>

#include "99_Utility/BehaviorTree/NodeBase.h"
#include "99_Utility/BehaviorTree/NodeStatus.h"

/**********************************************************************************
* @brief     : Behavior Tree動作確認用のデモAI。既存Enemy(Idle/Chase/AttackのFSM)と
*            : 同じ意思決定(索敵範囲で追跡開始/ロスト範囲で諦め/攻撃範囲で攻撃/
*            : 攻撃中は中断なし)をStateMachineを使わずBTのみで再構成したもの。
*            : 置き換えではなくデモであり、Enemy/Boss/Player本体には一切触れない。
*            : フェーズ切替時はOutputDebugStringへログを出すのでFSMとの比較が可能。
* @pattern   : Behavior Tree (Selector + Sequence + Action + 条件判定).
**********************************************************************************/

class BTDemoEnemy final
{
public:
	// FSMのIdle/Chase/Attackに対応する行動フェーズ.
	enum class ePhase
	{
		Idle,
		Chase,
		Attack,
	};

	BTDemoEnemy();
	~BTDemoEnemy();

	// BTを1回Tickする(実機ではGameTime::GetDeltaTime()を渡す想定).
	void Update(float DeltaTime);

public: // Getter・Setter.

	// ターゲット(Player相当)の位置.
	void SetTargetPos(const DirectX::XMFLOAT3& TargetPos) noexcept { m_TargetPos = TargetPos; }

	// 自身の位置.
	const DirectX::XMFLOAT3& GetPosition() const noexcept { return m_Position; }

	// 現在フェーズ(FSMの現在ステートIDに対応).
	ePhase GetPhase() const noexcept { return m_Phase; }

	// 攻撃判定相当が有効な期間か(AttackステートのActiveWindowに対応).
	bool IsAttackActiveWindow() const noexcept { return m_AttackElapsed >= ATTACK_WINDUP_TIME && m_AttackElapsed < ATTACK_WINDUP_TIME + ATTACK_ACTIVE_TIME; }

private:
	// Selector/Sequence/Action/Decoratorでツリーを構築する.
	void BuildTree();

	// 待機アクション(常にSuccessを返し毎フレーム再評価させる).
	NodeStatus TickIdle();

	// 追跡アクション(1フレーム分の旋回・前進を行いSuccessを返す).
	NodeStatus TickChase();

	// 攻撃アクション(攻撃中はRunningで中断されず、完了時にChase/Idleへ復帰先を決める).
	NodeStatus TickAttack();

	// 条件ノード: ターゲットが攻撃範囲内か.
	NodeStatus CondInAttackRange();

	// 条件ノード: 追跡を継続/開始できるか(追跡中はロスト範囲、未追跡なら索敵範囲で判定).
	NodeStatus CondCanChase();

	// ターゲットまでのXZ距離の2乗.
	float DistanceSqToTargetXZ() const noexcept;

	// ターゲットへの角度(度). 0度=+Z前方.
	float AngleToTargetDeg() const noexcept;

	// 目標角度へ最短経路で旋回する(GameObject::RotateToTargetと同じ規約).
	void RotateToTarget(float TargetAngleDeg, float SpeedDegPerSec) noexcept;

	// フェーズを変更し、切替ログを出力する.
	void ChangePhase(ePhase NextPhase);

private:
	static constexpr float MOVE_SPEED        = 4.0f;   // 追跡移動速度(単位/秒). Enemyの既定値と同じ.
	static constexpr float AGGRO_RANGE       = 10.0f;  // この距離以内でIdle→Chase.
	static constexpr float ATTACK_RANGE      = 2.5f;   // この距離以内でChase→Attack.
	static constexpr float LOSE_RANGE        = 20.0f;  // この距離を超えたら追跡を諦めてIdleへ戻る.
	static constexpr float CHASE_ROTATE_SPEED = 360.0f; // 度/秒.
	static constexpr float ATTACK_WINDUP_TIME   = 0.4f; // 予備動作(この間は攻撃判定なし).
	static constexpr float ATTACK_ACTIVE_TIME   = 0.2f; // 攻撃判定が有効な時間.
	static constexpr float ATTACK_RECOVERY_TIME = 0.5f; // 硬直(この間は動けない).
	static constexpr float ATTACK_TOTAL_TIME    = ATTACK_WINDUP_TIME + ATTACK_ACTIVE_TIME + ATTACK_RECOVERY_TIME;
	static constexpr float ATTACK_ROTATE_SPEED  = 720.0f; // 度/秒.

	std::unique_ptr<NodeBase> m_upRoot;                                              // BTルート(所有).
	DirectX::XMFLOAT3         m_Position      { 0.0f, 0.0f, 0.0f };                 // 自身の位置.
	DirectX::XMFLOAT3         m_TargetPos     { 0.0f, 0.0f, 0.0f };                 // ターゲットの位置.
	float                     m_YawRad        = 0.0f;                               // 自身の向き(Yaw, ラジアン).
	float                     m_DeltaTime     = 0.0f;                               // 直近Updateのデルタタイム.
	float                     m_AttackElapsed = 0.0f;                               // 攻撃経過時間.
	ePhase                    m_Phase         = ePhase::Idle;                       // 現在フェーズ.
};
