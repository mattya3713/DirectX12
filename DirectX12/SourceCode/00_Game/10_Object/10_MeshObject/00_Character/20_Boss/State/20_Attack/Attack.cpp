#include "Attack.h"

#include "00_Game/10_Object/10_MeshObject/00_Character/20_Boss/Boss.h"
#include "00_Game/00_GameLoop/Time/Time.h"
#include "00_Game/60_Combat/CombatTuning.h"

namespace {
	constexpr float ATTACK_ROTATE_SPEED = 720.0f; // 度/秒.
}

namespace BossState {

Attack::Attack(Boss* pOwner) noexcept
	: BossStateBase(pOwner)
{
}

void Attack::Enter()
{
	ApplyNamedClip("boss_attack1");
	m_ElapsedTime = 0.0f;
	GetBoss()->SetAttackColliderActive(false);
	GetBoss()->SetAttackAmount(CombatTuning::Get().Boss1Amount);
}

void Attack::Update()
{
	// 判定窓・硬直はCombatTuningから毎フレーム参照(エディタでの変更を即時反映).
	const float windup   = CombatTuning::Get().Boss1Windup;
	const float active   = CombatTuning::Get().Boss1Active;
	const float recovery = CombatTuning::Get().Boss1Recovery;

	m_ElapsedTime += GameTime::GetDeltaTime();

	GetBoss()->RotateToTarget(AngleToTargetDeg(), ATTACK_ROTATE_SPEED);

	const bool in_active_window =
		m_ElapsedTime >= windup &&
		m_ElapsedTime <  windup + active;
	GetBoss()->SetAttackColliderActive(in_active_window);

	if (m_ElapsedTime >= windup + active + recovery)
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
