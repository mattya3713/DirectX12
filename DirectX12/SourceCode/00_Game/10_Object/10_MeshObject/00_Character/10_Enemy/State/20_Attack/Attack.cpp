#include "Attack.h"

#include "00_Game/10_Object/10_MeshObject/00_Character/10_Enemy/Enemy.h"
#include "00_Game/00_GameLoop/Time/Time.h"

namespace {
	constexpr float ATTACK_WINDUP_TIME   = 0.4f;   // 予備動作(この間は攻撃判定なし).
	constexpr float ATTACK_ACTIVE_TIME   = 0.2f;   // 攻撃判定が有効な時間.
	constexpr float ATTACK_RECOVERY_TIME = 0.5f;   // 硬直(この間は動けない).
	constexpr float ATTACK_TOTAL_TIME    = ATTACK_WINDUP_TIME + ATTACK_ACTIVE_TIME + ATTACK_RECOVERY_TIME;
	constexpr float ATTACK_AMOUNT        = 10.0f;
	constexpr float ATTACK_ROTATE_SPEED  = 720.0f; // 度/秒.
}

namespace EnemyState {

Attack::Attack(Enemy* pOwner) noexcept
	: EnemyStateBase(pOwner)
{
}

void Attack::Enter()
{
	m_ElapsedTime = 0.0f;
	GetEnemy()->SetAttackColliderActive(false);
	GetEnemy()->SetAttackAmount(ATTACK_AMOUNT);
}

void Attack::Update()
{
	m_ElapsedTime += GameTime::GetDeltaTime();

	GetEnemy()->RotateToTarget(AngleToTargetDeg(), ATTACK_ROTATE_SPEED);

	const bool in_active_window =
		m_ElapsedTime >= ATTACK_WINDUP_TIME &&
		m_ElapsedTime <  ATTACK_WINDUP_TIME + ATTACK_ACTIVE_TIME;
	GetEnemy()->SetAttackColliderActive(in_active_window);

	if (m_ElapsedTime >= ATTACK_TOTAL_TIME)
	{
		const bool target_in_range = DistanceSqToTargetXZ() <= GetEnemy()->GetLoseRange() * GetEnemy()->GetLoseRange();
		GetEnemy()->ChangeState(target_in_range ? EnemyState::eID::Chase : EnemyState::eID::Idle);
	}
}

void Attack::Exit()
{
	GetEnemy()->SetAttackColliderActive(false);
}

} // namespace EnemyState
