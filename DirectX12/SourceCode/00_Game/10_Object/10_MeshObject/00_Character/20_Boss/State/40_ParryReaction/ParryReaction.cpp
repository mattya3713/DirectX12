#include "ParryReaction.h"

#include <algorithm>

#include "00_Game/10_Object/10_MeshObject/00_Character/20_Boss/Boss.h"
#include "99_System/GameLoop/Time/Time.h"
#include "99_Utility/DirectXMath/DirectXMathExpansion.h"
#include "99_Utility/Math/Easing/Easing.h"

namespace {
	constexpr float PARRY_REACTION_ROTATE_SPEED = 720.0f; // 度/秒.
}

namespace BossState {

ParryReaction::ParryReaction(Boss* pOwner, const DirectX::XMFLOAT3& TargetPosition, float TargetYawDeg, float Duration) noexcept
	: BossStateBase(pOwner)
	, m_TargetPosition { TargetPosition }
	, m_TargetYawDeg   { TargetYawDeg }
	, m_Duration       { Duration }
{
}

void ParryReaction::Enter()
{
	m_ElapsedTime   = 0.0f;
	m_StartPosition = GetBoss()->GetPosition();

	GetBoss()->SetAttackColliderActive(false); // パリィされた硬直中は攻撃判定を出さない.
}

void ParryReaction::Update()
{
	m_ElapsedTime += GameTime::GetDeltaTime();

	GetBoss()->RotateToTarget(m_TargetYawDeg, PARRY_REACTION_ROTATE_SPEED);

	const float clamped_time = std::min(m_ElapsedTime, m_Duration);
	DirectX::XMFLOAT3 position = {};
	MyEasing::UpdateEasing(MyEasing::Type::OutCubic, clamped_time, m_Duration, m_StartPosition, m_TargetPosition, position);
	GetBoss()->SetPosition(position);

	if (m_ElapsedTime >= m_Duration)
	{
		const bool target_in_range = DistanceToTargetXZ() <= GetBoss()->GetLoseRange();
		GetBoss()->ChangeState(target_in_range ? BossState::eID::Move : BossState::eID::Idle);
	}
}

} // namespace BossState
