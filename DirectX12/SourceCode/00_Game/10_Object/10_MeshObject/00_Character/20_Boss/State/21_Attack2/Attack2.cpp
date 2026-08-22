#include "Attack2.h"

#include "00_Game/10_Object/10_MeshObject/00_Character/20_Boss/Boss.h"
#include "00_Game/00_GameLoop/Time/Time.h"

namespace {
	// Attack(0.6/0.3/0.8, 威力25)との差別化用の仮値. 予備動作が長く大振り・高威力.
	constexpr float ATTACK_WINDUP_TIME   = 0.9f;   // 予備動作(この間は攻撃判定なし).
	constexpr float ATTACK_ACTIVE_TIME   = 0.25f;  // 攻撃判定が有効な時間.
	constexpr float ATTACK_RECOVERY_TIME = 0.6f;   // 硬直(この間は動けない).
	constexpr float ATTACK_TOTAL_TIME    = ATTACK_WINDUP_TIME + ATTACK_ACTIVE_TIME + ATTACK_RECOVERY_TIME;
	constexpr float ATTACK_AMOUNT        = 40.0f;
	constexpr float ATTACK_ROTATE_SPEED  = 360.0f; // 度/秒(大振りな分、攻撃中の追い回しはAttackより遅い).
}

namespace BossState {

Attack2::Attack2(Boss* pOwner) noexcept
	: BossStateBase(pOwner)
{
}

void Attack2::Enter()
{
	ApplyNamedClip("boss_attack2");
	m_ElapsedTime = 0.0f;
	GetBoss()->SetAttackColliderActive(false);
	GetBoss()->SetAttackAmount(ATTACK_AMOUNT);
}

void Attack2::Update()
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

void Attack2::Exit()
{
	GetBoss()->SetAttackColliderActive(false);
}

} // namespace BossState
