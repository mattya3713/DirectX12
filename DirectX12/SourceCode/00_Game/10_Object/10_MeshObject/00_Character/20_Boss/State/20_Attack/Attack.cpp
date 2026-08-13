#include "Attack.h"

#include "00_Game/10_Object/10_MeshObject/00_Character/20_Boss/Boss.h"
#include "99_System/GameLoop/Time/Time.h"

namespace {
	// Enemy::Attackより大振り・高威力(仮値. 専用攻撃パターン実装時に見直す).
	constexpr float ATTACK_WINDUP_TIME   = 0.6f;   // 予備動作(この間は攻撃判定なし).
	constexpr float ATTACK_ACTIVE_TIME   = 0.3f;   // 攻撃判定が有効な時間.
	constexpr float ATTACK_RECOVERY_TIME = 0.8f;   // 硬直(この間は動けない).
	constexpr float ATTACK_TOTAL_TIME    = ATTACK_WINDUP_TIME + ATTACK_ACTIVE_TIME + ATTACK_RECOVERY_TIME;
	constexpr float ATTACK_AMOUNT        = 25.0f;
	constexpr float ATTACK_ROTATE_SPEED  = 720.0f; // 度/秒.
}

namespace BossState {

Attack::Attack(Boss* pOwner) noexcept
	: BossStateBase(pOwner)
{
}

void Attack::Enter()
{
	m_ElapsedTime = 0.0f;
	GetBoss()->SetAttackColliderActive(false);
	GetBoss()->SetAttackAmount(ATTACK_AMOUNT);
}

void Attack::Update()
{
	m_ElapsedTime += GameTime::GetDeltaTime();

	GetBoss()->RotateToTarget(AngleToTargetDeg(), ATTACK_ROTATE_SPEED);

	const bool in_active_window =
		m_ElapsedTime >= ATTACK_WINDUP_TIME &&
		m_ElapsedTime <  ATTACK_WINDUP_TIME + ATTACK_ACTIVE_TIME;
	GetBoss()->SetAttackColliderActive(in_active_window);

	if (m_ElapsedTime >= ATTACK_TOTAL_TIME)
	{
		const bool target_in_range = DistanceToTargetXZ() <= GetBoss()->GetLoseRange();
		GetBoss()->ChangeState(target_in_range ? BossState::eID::Move : BossState::eID::Idle);
	}
}

void Attack::Exit()
{
	GetBoss()->SetAttackColliderActive(false);
}

} // namespace BossState
