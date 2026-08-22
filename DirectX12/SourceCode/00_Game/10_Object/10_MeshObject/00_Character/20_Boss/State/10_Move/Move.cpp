#include "Move.h"

#include <cmath>

#include "00_Game/10_Object/10_MeshObject/00_Character/20_Boss/Boss.h"
#include "00_Game/00_GameLoop/Time/Time.h"
#include "99_Utility/Math/Random/Random.h"

namespace {
	constexpr float MOVE_ROTATE_SPEED = 360.0f; // 度/秒.
}

namespace BossState {

Move::Move(Boss* pOwner) noexcept
	: BossStateBase(pOwner)
{
}

void Move::Enter()
{
	ApplyNamedClip("boss_walk1");
}

void Move::Update()
{
	const float distance = DistanceToTargetXZ();

	if (distance > GetBoss()->GetLoseRange())
	{
		GetBoss()->ChangeState(BossState::eID::Idle);
		return;
	}

	if (distance <= GetBoss()->GetAttackRange())
	{
		// 攻撃範囲内に入ったら2パターンからランダムに選ぶ(50/50の単純な抽選).
		const int pick = MyRand::GetRandomPercentage(0, 1);
		GetBoss()->ChangeState(pick == 0 ? BossState::eID::Attack : BossState::eID::Attack2);
		return;
	}

	GetBoss()->RotateToTarget(AngleToTargetDeg(), MOVE_ROTATE_SPEED);

	// 向いている方向(Yaw)へそのまま前進する.
	const float yaw = GetBoss()->GetTransform().Rotation.y;
	const float speed_and_delta = GetBoss()->GetMoveSpeed() * GameTime::GetDeltaTime();
	GetBoss()->AddPosition({ std::sinf(yaw) * speed_and_delta, 0.0f, std::cosf(yaw) * speed_and_delta });
}

} // namespace BossState
