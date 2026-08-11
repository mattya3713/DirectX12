#pragma once

#include <algorithm>
#include <DirectXMath.h>

#include "00_Game/10_Object/10_MeshObject/00_Character/Character.h"
#include "00_Game/10_Object/10_MeshObject/00_Character/00_Player/PlayerAccessKeys.h"
#include "00_Game/10_Object/10_MeshObject/00_Character/00_Player/State/PlayerStateID.h"
#include "00_Game/10_Object/10_MeshObject/00_Character/00_Player/State/PlayerStateBase.h"
#include "99_Utility/StateMachine/StateMachine.h"

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/10.
* @brief     : プレイヤークラス. 
*			 : StateMachine<Player>とPlayerStateで構成されるステートマシンを持つ.
* @pattern   : FSM.
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

	// コンボ数・必殺ゲージの取得.
	int GetCombo() const noexcept { return m_Combo; }
	float GetCurrentUltValue() const noexcept { return m_CurrentUltValue; }
	float GetMaxUltValue() const noexcept { return m_MaxUltValue; }

	// コンボ数に応じた倍率(コンボが増えるほど獲得量が増える. 攻撃/必殺ゲージ獲得量の計算に使う).
	float ComboMultiplier() const noexcept { return 1.0f + (static_cast<float>(m_Combo) / 100.0f) * 0.05f; }

	// コンボ数・必殺ゲージの変更(攻撃系Stateのみ呼べる).
	void AddCombo(int Amount, PlayerAccess::ComboEconomyKey) noexcept { m_Combo += Amount; }
	void ResetCombo(PlayerAccess::ComboEconomyKey) noexcept { m_Combo = 0; }
	void AddUltValue(float Amount, PlayerAccess::ComboEconomyKey) noexcept { m_CurrentUltValue = std::clamp(m_CurrentUltValue + Amount, 0.0f, m_MaxUltValue); }
	void ResetUltValue(PlayerAccess::ComboEconomyKey) noexcept { m_CurrentUltValue = 0.0f; }

public:
	// ステートを変更する(PlayerState::eIDから対応するステートを生成しStateMachineへ渡す).
	void ChangeState(PlayerState::eID Id);

private:
	StateMachine<Player> m_StateMachine;					// 現在ステートの保持・更新.
	DirectX::XMFLOAT3    m_MoveVec        { 0.0f, 0.0f, 0.0f };	// 現フレームの移動ベクトル.
	float                m_RunMoveSpeed   = 8.0f;				// 走り移動速度(単位/秒).
	PlayerState::eID     m_CurrentStateID = PlayerState::eID::None;	// 現在ステートID(デバッグ表示用).

	int   m_Combo           = 0;		// 現在のコンボ数.
	float m_CurrentUltValue = 0.0f;	// 必殺ゲージ(現在値).
	float m_MaxUltValue     = 10000.0f;	// 必殺ゲージ(最大値).
};
