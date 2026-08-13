#include "Chase.h"

#include <cmath>

#include "00_Game/10_Object/10_MeshObject/00_Character/10_Enemy/Enemy.h"
#include "00_Game/00_GameLoop/Time/Time.h"

namespace {
	constexpr float CHASE_ROTATE_SPEED = 360.0f; // 度/秒.
}

namespace EnemyState {

Chase::Chase(Enemy* pOwner) noexcept
	: EnemyStateBase(pOwner)
{
}

void Chase::Update()
{
	const float distance = DistanceToTargetXZ();

	if (distance > GetEnemy()->GetLoseRange())
	{
		GetEnemy()->ChangeState(EnemyState::eID::Idle);
		return;
	}

	if (distance <= GetEnemy()->GetAttackRange())
	{
		GetEnemy()->ChangeState(EnemyState::eID::Attack);
		return;
	}

	GetEnemy()->RotateToTarget(AngleToTargetDeg(), CHASE_ROTATE_SPEED);

	// 向いている方向(Yaw)へそのまま前進する.
	const float yaw = GetEnemy()->GetTransform().Rotation.y;
	const float speed_and_delta = GetEnemy()->GetMoveSpeed() * GameTime::GetDeltaTime();
	GetEnemy()->AddPosition({ std::sinf(yaw) * speed_and_delta, 0.0f, std::cosf(yaw) * speed_and_delta });
}

} // namespace EnemyState
