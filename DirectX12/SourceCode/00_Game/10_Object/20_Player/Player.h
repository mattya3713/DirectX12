#pragma once

#include <DirectXMath.h>

#include "00_Game/10_Object/10_Character/Character.h"
#include "00_Game/10_Object/20_Player/PlayerAccessKeys.h"
#include "00_Game/10_Object/20_Player/State/PlayerStateID.h"
#include "00_Game/10_Object/20_Player/State/PlayerStateBase.h"
#include "99_Utility/StateMachine/StateMachine.h"

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/10.
* @brief     : プレイヤークラス. StateMachine<Player>とPlayerState以下のステート群で
*            : 入力駆動の移動(Idle/Run)を制御する.
* @pattern   : State.
**********************************************************************************/

class Player final : public Character
{
public:
	Player();
	~Player() override = default;

	// 毎フレーム更新(現在ステートのUpdate/LateUpdateを順に呼ぶ).
	void Update() override;

public: // Getter・Setter.

	// 現フレームの移動ベクトル(ワールド空間、Y成分は常に0).
	const DirectX::XMFLOAT3& GetMoveVec() const noexcept { return m_MoveVec; }
	void SetMoveVec(const DirectX::XMFLOAT3& MoveVec, PlayerAccess::MovementKey) noexcept { m_MoveVec = MoveVec; }

	// 走り移動速度(単位/秒)の取得.
	float GetRunMoveSpeed() const noexcept { return m_RunMoveSpeed; }

	// 現在ステートIDの取得(デバッグ表示等、外部からの参照用).
	PlayerState::eID GetCurrentStateID() const noexcept { return m_CurrentStateID; }

public:
	// ステートを変更する(PlayerState::eIDから対応するステートを生成しStateMachineへ渡す).
	void ChangeState(PlayerState::eID Id);

	// 目標Yaw角(度)へラープ回転する.
	void RotateToTarget(float TargetAngleDeg, float SpeedDegPerSec) noexcept;

private:
	StateMachine<Player> m_StateMachine;					// 現在ステートの保持・更新.
	DirectX::XMFLOAT3    m_MoveVec        { 0.0f, 0.0f, 0.0f };	// 現フレームの移動ベクトル.
	float                m_RunMoveSpeed   = 8.0f;				// 走り移動速度(単位/秒).
	PlayerState::eID     m_CurrentStateID = PlayerState::eID::None;	// 現在ステートID(デバッグ表示用).
};
