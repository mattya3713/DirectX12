#include "DodgeExecute.h"

#include <algorithm>

#include "00_Game/10_Object/10_MeshObject/00_Character/00_Player/Player.h"
#include "00_Game/00_GameLoop/Time/Time.h"
#include "99_Utility/Math/Easing/Easing.h"

namespace {
	constexpr float DODGE_DISTANCE   = 25.0f;
	constexpr float DODGE_DURATION   = 1.7f;
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

	m_Distance         = DODGE_DISTANCE;
	m_MaxTime          = DODGE_DURATION;
	m_TraveledDistance = 0.0f;

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

	if (m_TraveledDistance >= m_Distance)
	{
		GetPlayer()->ChangeState(PlayerState::eID::Idle);
	}
}

} // namespace PlayerState
