#pragma once

#include <algorithm>
#include <DirectXMath.h>

#include "00_Game/10_Object/10_MeshObject/00_Character/Character.h"
#include "00_Game/10_Object/10_MeshObject/00_Character/00_Player/PlayerAccessKeys.h"
#include "00_Game/10_Object/10_MeshObject/00_Character/00_Player/State/PlayerStateID.h"
#include "00_Game/10_Object/10_MeshObject/00_Character/00_Player/State/PlayerStateBase.h"
#include "00_Game/40_Collision/00_Capsule/CapsuleCollider.h"
#include "99_Utility/StateMachine/StateMachine.h"

class PlayerCombatView;

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/10.
* @brief     : プレイヤークラス. 
*			 : StateMachine<Player>とPlayerStateで構成されるステートマシンを持つ.
* @pattern   : FSM.
**********************************************************************************/

class Player final : public Character
{
	friend class PlayerCombatView; // CombatCoordinator用の限定公開Viewにだけリアクション設定を許可する.

public:
	Player();
	~Player() override; // defaultから変更(パリィ判定コライダーの登録解除が必要なため).

	// 毎フレーム更新(現在ステートのUpdate/LateUpdateを順に呼ぶ).
	void Update() override;

	// 描画(モデルが180度反転した状態で作られているため、描画時だけ正面を合わせてから戻す).
	void Draw() override;

#if _DEBUG
	// 被弾/攻撃判定に加えパリィ判定コライダーもワイヤーフレームで描画する(Debugビルドのみ.
	// MainScene側で全キャラのDraw()が終わった後にまとめて呼ぶこと).
	void DrawDebugColliders() const;
#endif

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

	// リアクション目標が設定されているか(PlayerState::Parryが毎フレーム確認する).
	bool HasParryReactionTarget() const noexcept { return m_HasParryReactionTarget; }
	const DirectX::XMFLOAT3& GetParryReactionTargetPos() const noexcept { return m_ParryReactionTargetPos; }
	float GetParryReactionTargetYawDeg() const noexcept { return m_ParryReactionTargetYawDeg; }
	float GetParryReactionDuration() const noexcept { return m_ParryReactionDuration; }

	// リアクション消費完了をPlayerState::Parry自身が通知する(次回また使えるようフラグを戻すだけ).
	void ClearParryReactionTarget() noexcept { m_HasParryReactionTarget = false; }

	// パリィ判定コライダーの有効/無効(PlayerState::Parryが構え中だけ有効化する).
	void SetParryColliderActive(bool IsActive) noexcept { m_ParryCollider.SetActive(IsActive); }

	// パリィ判定の結果(PlayerState::Parryが毎フレーム確認する. EnemyAttackを検出したら成立とみなす).
	const std::vector<CollisionInfo>& GetParryCollisionEvents() const noexcept { return m_ParryCollider.GetCollisionEvents(); }

public:
	// ステートを変更する(PlayerState::eIDから対応するステートを生成しStateMachineへ渡す).
	void ChangeState(PlayerState::eID Id);

private:
	void EnterParryReaction(const DirectX::XMFLOAT3& TargetPosition, float TargetYawDeg, float Duration) noexcept
	{
		m_ParryReactionTargetPos    = TargetPosition;
		m_ParryReactionTargetYawDeg = TargetYawDeg;
		m_ParryReactionDuration     = Duration;
		m_HasParryReactionTarget    = true;
	}

private:
	StateMachine<Player> m_StateMachine;					// 現在ステートの保持・更新.
	CapsuleCollider       m_ParryCollider;					// パリィ判定専用(m_DamageColliderとは別物. Parry中のみ有効).
	DirectX::XMFLOAT3    m_MoveVec        { 0.0f, 0.0f, 0.0f };	// 現フレームの移動ベクトル.
	float                m_RunMoveSpeed   = 8.0f;				// 走り移動速度(単位/秒).
	PlayerState::eID     m_CurrentStateID = PlayerState::eID::None;	// 現在ステートID(デバッグ表示用).

	int   m_Combo           = 0;		// 現在のコンボ数.
	float m_CurrentUltValue = 0.0f;	// 必殺ゲージ(現在値).
	float m_MaxUltValue     = 10000.0f;	// 必殺ゲージ(最大値).

	// パリィ成立時のリアクション目標(CombatCoordinatorが設定、PlayerState::Parryが消費する).
	bool              m_HasParryReactionTarget    = false;
	DirectX::XMFLOAT3 m_ParryReactionTargetPos    { 0.0f, 0.0f, 0.0f };
	float             m_ParryReactionTargetYawDeg = 0.0f;
	float             m_ParryReactionDuration     = 0.0f;
};
