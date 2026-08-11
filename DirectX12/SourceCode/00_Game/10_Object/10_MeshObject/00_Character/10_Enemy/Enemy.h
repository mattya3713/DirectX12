#pragma once

#include <DirectXMath.h>

#include "00_Game/10_Object/10_MeshObject/00_Character/Character.h"
#include "00_Game/10_Object/10_MeshObject/00_Character/10_Enemy/State/EnemyStateID.h"
#include "99_Utility/StateMachine/StateMachine.h"

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/11.
* @brief     : 敵キャラクターの基底クラス. StateMachine<Enemy>とEnemyState以下の
*            : ステート群(Idle/Chase/Attack/Dead)でPlayerを追跡・攻撃する
*            : 最小限のAIを持つ(索敵・複数種の攻撃パターン等は未実装).
*            : 雑魚敵はこのクラスをそのまま/派生して使い、Bossはこれを継承してさらに拡張する想定.
**********************************************************************************/

class Enemy : public Character
{
public:
	Enemy();
	~Enemy() override;

	// 毎フレーム更新(現在ステートのUpdate/LateUpdateを順に呼ぶ).
	void Update() override;

public: // Getter・Setter.

	// ターゲット(Player)の位置. ロックオン等は無く、シーン側が毎フレーム設定する想定.
	const DirectX::XMFLOAT3& GetTargetPos() const noexcept { return m_TargetPos; }
	void SetTargetPos(const DirectX::XMFLOAT3& TargetPos) noexcept { m_TargetPos = TargetPos; }

	// AI用の調整値の取得(現状は固定値. 将来種類ごとに変えたくなったらコンストラクタ引数化する).
	float GetMoveSpeed() const noexcept { return m_MoveSpeed; }
	float GetAggroRange() const noexcept { return m_AggroRange; }
	float GetAttackRange() const noexcept { return m_AttackRange; }
	float GetLoseRange() const noexcept { return m_LoseRange; }

	// 現在ステートIDの取得(デバッグ表示等、外部からの参照用).
	EnemyState::eID GetCurrentStateID() const noexcept { return m_CurrentStateID; }

public:
	// ステートを変更する(EnemyState::eIDから対応するステートを生成しStateMachineへ渡す).
	void ChangeState(EnemyState::eID Id);

private:
	StateMachine<Enemy> m_StateMachine;                          // 現在ステートの保持・更新.
	DirectX::XMFLOAT3    m_TargetPos      { 0.0f, 0.0f, 0.0f };  // ターゲット(Player)の位置.
	EnemyState::eID       m_CurrentStateID = EnemyState::eID::None;

	float m_MoveSpeed   = 4.0f;  // 追跡移動速度(単位/秒).
	float m_AggroRange  = 10.0f; // この距離以内でIdle→Chase.
	float m_AttackRange = 2.5f;  // この距離以内でChase→Attack.
	float m_LoseRange   = 20.0f; // この距離を超えたら追跡を諦めてIdleへ戻る.
};
