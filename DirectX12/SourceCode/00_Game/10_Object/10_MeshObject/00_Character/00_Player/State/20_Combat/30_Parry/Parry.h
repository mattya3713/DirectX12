#pragma once

#include <DirectXMath.h>

#include "00_Game/10_Object/10_MeshObject/00_Character/00_Player/State/20_Combat/Combat.h"

namespace PlayerState {

	/**********************************************************************************
	* @author    : mattya3713.
	* @date      : 2026/08/11.
	* @brief     : パリィ. 一定時間、通常の被弾判定を無効化する(無敵)構え.
	*            : 構え中はm_DamageColliderの代わりにPlayer::m_ParryCollider(PlayerParryマスク)
	*            : を有効化し、EnemyAttackを検出したら成立とみなしてCombatCoordinator::
	*            : OnParrySuccess()を呼ぶ. 成立するとPlayer::HasParryReactionTarget()が立ち、
	*            : 目標位置・向きへ遷移するリアクションをこのステート自身が消費する
	*            : (BossState::ParryReactionと対になる. 詳細はCombatCoordinator参照).
	*            : SenzanのParry::Enter()はCombat::Enter()を呼んでおらずm_CurrentTimeが
	*            : リセットされない状態依存のバグに見えたため、こちらでは正しく呼ぶ.
	**********************************************************************************/

	class Parry final : public Combat
	{
	public:
		explicit Parry(Player* pOwner) noexcept;
		~Parry() override = default;

		PlayerState::eID GetStateID() const override { return PlayerState::eID::Parry; }
		// JSON設定は使わない(ColliderWindowsも持たない. SenzanのParryも未使用).

		void Enter() override;
		void Update() override;
		void Exit() override;

	private:
		float m_ElapsedTime = 0.0f; // 構えてからの経過時間.

		// パリィ成立リアクション用(Player::HasParryReactionTarget()がtrueの間だけ使う).
		bool              m_IsReacting         = false;
		float             m_ReactionElapsedTime = 0.0f;
		DirectX::XMFLOAT3 m_ReactionStartPos   {};
	};

} // namespace PlayerState
