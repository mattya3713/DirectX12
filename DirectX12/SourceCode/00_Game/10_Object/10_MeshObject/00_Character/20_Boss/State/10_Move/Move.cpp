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
	const float distance_sq = DistanceSqToTargetXZ();

	if (distance_sq > GetBoss()->GetLoseRange() * GetBoss()->GetLoseRange())
	{
		GetBoss()->ChangeState(BossState::eID::Idle);
		return;
	}

	if (distance_sq <= GetBoss()->GetAttackRange() * GetBoss()->GetAttackRange())
	{
		// 攻撃範囲内に入ったら5パターンを重み付きで抽選する(近距離ほど近接攻撃が出やすい).
		// 内訳(仮値): Attack40% / Attack2 25% / Spin20% / Jump10% / Beam5%.
		const int roll = MyRand::GetRandomPercentage(0, 99);
		if      (roll < 40) { GetBoss()->ChangeState(BossState::eID::Attack); }
		else if (roll < 65) { GetBoss()->ChangeState(BossState::eID::Attack2); }
		else if (roll < 85) { GetBoss()->ChangeState(BossState::eID::SpinAttack); }
		else if (roll < 95) { GetBoss()->ChangeState(BossState::eID::JumpAttack); }
		else                { GetBoss()->ChangeState(BossState::eID::BeamAttack); }
		return;
	}

	// 中距離では稀にビームで牽制する(遠距離攻撃の存在を知らせる役割).
	if (MyRand::GetRandomPercentage(0, 99) < 20)
	{
		GetBoss()->ChangeState(BossState::eID::BeamAttack);
		return;
	}

	GetBoss()->RotateToTarget(AngleToTargetDeg(), MOVE_ROTATE_SPEED);

	// 向いている方向(Yaw)へそのまま前進する.
	const float yaw = GetBoss()->GetTransform().Rotation.y;
	const float speed_and_delta = GetBoss()->GetMoveSpeed() * GameTime::GetDeltaTime();
	GetBoss()->AddPosition({ std::sinf(yaw) * speed_and_delta, 0.0f, std::cosf(yaw) * speed_and_delta });
}

} // namespace BossState
