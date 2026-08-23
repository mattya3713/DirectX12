#include "DodgeExecute.h"

#include <algorithm>
#include <cmath>

#include "00_Game/10_Object/10_MeshObject/00_Character/00_Player/Player.h"
#include "00_Game/00_GameLoop/Time/Time.h"
#include "00_Game/40_Collision/CollisionDetector.h"
#include "00_Game/40_Collision/00_Core/ColliderBase.h"
#include "00_Game/60_Combat/CombatTuning.h"
#include "99_Utility/Debug/Log/DebugLog.h"
#include "99_Utility/Math/Easing/Easing.h"
#include "99_Utility/ServiceLocator/ServiceLocator.h"

namespace {
	constexpr float EASE_BLEND_RATIO = 0.5f; // InOutCubicとLinerを半々でブレンドする.

	// InOutCubicとLinerを半々でブレンドした移動距離(0〜Distance)を求める.
	float BlendedEasedDistance(float Time, float MaxTime, float Distance)
	{
		float eased  = 0.0f;
		float linear = 0.0f;
		MyEasing::UpdateEasing(MyEasing::Type::InOutCubic, Time, MaxTime, 0.0f, Distance, eased);
		MyEasing::UpdateEasing(MyEasing::Type::Liner, Time, MaxTime, 0.0f, Distance, linear);
		return eased * EASE_BLEND_RATIO + linear * (1.0f - EASE_BLEND_RATIO);
	}
}

namespace PlayerState {

DodgeExecute::DodgeExecute(Player* pOwner) noexcept
	: Dodge(pOwner)
{
}

void DodgeExecute::Enter()
{
	Dodge::Enter();

	m_Distance         = CombatTuning::Get().DodgeDistance;
	m_MaxTime          = CombatTuning::Get().DodgeDuration;
	m_TraveledDistance = 0.0f;
	m_IsJustDodgeJudged = false;

	ApplyNamedClip("player_perfect_dodge");
}

void DodgeExecute::LateUpdate()
{
	const float prev_time = m_CurrentTime;
	m_CurrentTime = std::min(m_CurrentTime + GameTime::GetDeltaTime(), m_MaxTime);

	// 前フレーム・今フレームそれぞれのイージング位置の差分だけ移動する
	// (加減速のある滑らかな回避移動になる).
	const float prev_dist    = BlendedEasedDistance(prev_time, m_MaxTime, m_Distance);
	const float current_dist = BlendedEasedDistance(m_CurrentTime, m_MaxTime, m_Distance);
	const float move_amount  = current_dist - prev_dist;

	GetPlayer()->AddPosition({ m_InputVec.x * move_amount, 0.0f, m_InputVec.y * move_amount });

	m_TraveledDistance = current_dist;

	CheckJustDodge();

	if (m_TraveledDistance >= m_Distance)
	{
		GetPlayer()->ChangeState(PlayerState::eID::Idle);
	}
}

void DodgeExecute::CheckJustDodge()
{
	if (m_IsJustDodgeJudged) { return; }

	CollisionDetector* p_detector = ServiceLocator::Get<CollisionDetector>();
	if (!p_detector) { return; }

	// 被弾カプセルの中心(足元+1.0m)と敵攻撃判定との水平距離で「すれ違い」を判定する
	// (無敵中は本来の衝突イベントが飛んでこないため、別途の軽量な半径チェックで代用する).
	const DirectX::XMFLOAT3& player_pos = GetPlayer()->GetPosition();

	for (ColliderBase* p_collider : p_detector->GetColliders())
	{
		if (!p_collider || !p_collider->GetActive()) { continue; }
		if (p_collider->GetMyMask() != eCollisionGroup::EnemyAttack) { continue; }

		const DirectX::XMFLOAT3 attack_pos = p_collider->GetPosition();
		const float dx = attack_pos.x - player_pos.x;
		const float dz = attack_pos.z - player_pos.z;

		// 成立距離はCombatTuningのJustDodgeRadius(エディタで調整可能).
		const float radius = CombatTuning::Get().JustDodgeRadius;
		if (dx * dx + dz * dz <= radius * radius)
		{
			m_IsJustDodgeJudged = true;

			if (DebugLog* p_debug_log = ServiceLocator::Get<DebugLog>()) {
				p_debug_log->LogInfo("Just Dodge!");
			}
			return;
		}
	}
}

} // namespace PlayerState
