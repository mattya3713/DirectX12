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

	// 描画(player.msknは正面が-Zで作られているため、デバッグパネルで調整したオフセット角を描画時に加算する).
	void Draw() override;

#if _DEBUG
	// モデル正面のYawオフセット(度)を取得・設定する(デバッグパネルからの調整用).
	float GetModelFrontOffsetDeg() const noexcept { return m_ModelFrontOffsetDeg; }
	void  SetModelFrontOffsetDeg(float OffsetDeg) noexcept { m_ModelFrontOffsetDeg = OffsetDeg; }

	// 被弾/攻撃判定に加えパリィ判定コライダーもワイヤーフレームで描画する(Debugビルドのみ.
	// MainScene側で全キャラのDraw()が終わった後にまとめて呼ぶこと).
	void DrawDebugColliders() const;
#endif

public: // Getter・Setter.

	// 現フレームの移動ベクトル(ワールド空間、Y成分は常に0).
	const DirectX::XMFLOAT3& GetMoveVec() const noexcept { return m_MoveVec; }
	void SetMoveVec(const DirectX::XMFLOAT3& MoveVec, PlayerAccess::MovementKey) noexcept { m_MoveVec = MoveVec; }

	// ノックバック初速の取得(OnDamaged()が設定する. KnockBack StateがEnter時に受け取る).
	const DirectX::XMFLOAT3& GetKnockBackVelocity() const noexcept { return m_KnockBackVelocity; }

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

	// パリィ成立した攻撃の有効化IDを記録する(ProcessHitsがダメージ二重適用を防ぐために参照する).
	void NotifyParriedAttack(std::uint32_t AttackActivationId) noexcept { m_LastParriedActivationId = AttackActivationId; }

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

protected:
	// 被弾リアクション: 吹き飛び方向を計算してKnockBack Stateへ遷移する.
	// (Boss/Enemyは既定の空実装のまま. ノックバック初速はKnockBack StateがGetKnockBackVelocity()で受け取る.)
	void OnDamaged(const HitEvent& Event) override;

	// パリィ済みの攻撃(直前に成立した有効化ID)はダメージ適用をスキップする
	// (パリィ構え中も被弾判定が有効なため、同一攻撃がParryColliderとDamageColliderの両方に当たる).
	bool ShouldIgnoreHit(const CollisionInfo& Info) const noexcept override
	{
		return Info.AttackActivationId != 0 && Info.AttackActivationId == m_LastParriedActivationId;
	}

private:
	StateMachine<Player> m_StateMachine;					// 現在ステートの保持・更新.
	CapsuleCollider       m_ParryCollider;					// パリィ判定専用(m_DamageColliderとは別物. Parry中のみ有効).
	DirectX::XMFLOAT3    m_MoveVec        { 0.0f, 0.0f, 0.0f };	// 現フレームの移動ベクトル.
	DirectX::XMFLOAT3    m_KnockBackVelocity{ 0.0f, 0.0f, 0.0f };	// ノックバック初速(OnDamagedが計算し、KnockBack Stateが消費する).
	float                m_RunMoveSpeed   = 8.0f;				// 走り移動速度(単位/秒).
	PlayerState::eID     m_CurrentStateID = PlayerState::eID::None;	// 現在ステートID(デバッグ表示用).
	std::uint32_t        m_LastParriedActivationId = 0;	// 直前にパリィ成立した攻撃の有効化ID(0=なし).
#if _DEBUG
	float m_ModelFrontOffsetDeg = 180.0f;	// モデル正面軸のズレ補正角(度). player.msknは-Z正面のため既定で180.
#endif

	int   m_Combo           = 0;		// 現在のコンボ数.
	float m_CurrentUltValue = 0.0f;	// 必殺ゲージ(現在値).
	float m_MaxUltValue     = 10000.0f;	// 必殺ゲージ(最大値).

	// パリィ成立時のリアクション目標(CombatCoordinatorが設定、PlayerState::Parryが消費する).
	bool              m_HasParryReactionTarget    = false;
	DirectX::XMFLOAT3 m_ParryReactionTargetPos    { 0.0f, 0.0f, 0.0f };
	float             m_ParryReactionTargetYawDeg = 0.0f;
	float             m_ParryReactionDuration     = 0.0f;
};
