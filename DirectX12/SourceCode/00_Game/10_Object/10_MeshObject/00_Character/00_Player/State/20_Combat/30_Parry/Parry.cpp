#include "Parry.h"

#include <algorithm>

#include "00_Game/10_Object/10_MeshObject/00_Character/00_Player/Player.h"
#include "00_Game/60_Combat/CombatCoordinator.h"
#include "00_Game/00_GameLoop/Time/Time.h"
#include "00_Game/60_Combat/CombatTuning.h"
#include "99_Utility/DirectXMath/DirectXMathExpansion.h"
#include "99_Utility/Math/Easing/Easing.h"
#include "99_Utility/ServiceLocator/ServiceLocator.h"

namespace {
	constexpr float PARRY_REACTION_ROTATE_SPEED = 720.0f; // リアクション中の向き直り速度(度/秒).
}

namespace PlayerState {

Parry::Parry(Player* pOwner) noexcept
	: Combat(pOwner)
{
}

void Parry::Enter()
{
	Combat::Enter();

	m_ElapsedTime = 0.0f;
	m_IsReacting  = false;
	// パリィはリスクのある一発勝負(Sekiro型)のため、構え中も被弾判定は有効のまま
	// (SetDamageColliderActive(false)は呼ばない. 成立した攻撃のみProcessHits側で無視する).
	GetPlayer()->SetParryColliderActive(true); // パリィ判定を有効化.

	ApplyNamedClip("player_parry");
}

void Parry::Update()
{
	Combat::Update();

	// パリィ判定コライダーがEnemyAttackを検出したら成立とみなし、CombatCoordinatorへ通知する
	// (この呼び出しでPlayer::HasParryReactionTarget()がtrueになり、下のリアクション処理へ入る).
	if (!GetPlayer()->HasParryReactionTarget())
	{
		for (const CollisionInfo& info : GetPlayer()->GetParryCollisionEvents())
		{
			if (!info.IsHit) { continue; }

			// 成立した攻撃の有効化IDを記録し、同一攻撃のダメージ適用を防ぐ
			// (パリィ構え中も被弾判定が有効なため、同じ攻撃がDamageColliderにも検出されうる).
			GetPlayer()->NotifyParriedAttack(info.AttackActivationId);

			if (CombatCoordinator* p_coordinator = ServiceLocator::Get<CombatCoordinator>())
			{
				p_coordinator->OnParrySuccess();
			}
			break; // 1フレームに1回で十分.
		}
	}

	// パリィ成立リアクション中(BossState::ParryReactionと対の処理. CombatCoordinator参照).
	if (GetPlayer()->HasParryReactionTarget())
	{
		if (!m_IsReacting)
		{
			m_IsReacting          = true;
			m_ReactionElapsedTime = 0.0f;
			m_ReactionStartPos    = GetPlayer()->GetPosition();
		}

		m_ReactionElapsedTime += GameTime::GetDeltaTime();

		const float duration      = GetPlayer()->GetParryReactionDuration();
		const float clamped_time  = std::min(m_ReactionElapsedTime, duration);

		DirectX::XMFLOAT3 position = {};
		MyEasing::UpdateEasing(MyEasing::Type::OutCubic, clamped_time, duration,
			m_ReactionStartPos, GetPlayer()->GetParryReactionTargetPos(), position);
		GetPlayer()->SetPosition(position);
		GetPlayer()->RotateToTarget(GetPlayer()->GetParryReactionTargetYawDeg(), PARRY_REACTION_ROTATE_SPEED);

		if (m_ReactionElapsedTime >= duration)
		{
			GetPlayer()->ClearParryReactionTarget();
			m_IsReacting = false;
			GetPlayer()->ChangeState(PlayerState::eID::Idle);
		}
		return; // リアクション中は下の通常タイムアウト判定を行わない.
	}

	m_ElapsedTime += GameTime::GetDeltaTime();
	if (m_ElapsedTime >= CombatTuning::Get().ParryMaxWaitTime)
	{
		GetPlayer()->ChangeState(PlayerState::eID::Idle);
	}
}

void Parry::Exit()
{
	GetPlayer()->SetParryColliderActive(false);

	// リアクション未消化のまま抜けた場合に備え、次回の構えへ持ち越さないようクリアする.
	GetPlayer()->ClearParryReactionTarget();
	m_IsReacting = false;

	GetPlayer()->SetDamageColliderActive(true);

	Combat::Exit();
}

} // namespace PlayerState
