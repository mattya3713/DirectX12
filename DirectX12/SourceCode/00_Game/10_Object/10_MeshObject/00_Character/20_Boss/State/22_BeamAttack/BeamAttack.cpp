#include "BeamAttack.h"

#include "00_Game/10_Object/10_MeshObject/00_Character/20_Boss/Boss.h"
#include "00_Game/00_GameLoop/Time/Time.h"
#include "00_Game/60_Combat/CombatTuning.h"

namespace {
	constexpr float ATTACK_ROTATE_SPEED = 180.0f; // 度/秒(溜め中はゆっくり追い回し).
}

namespace BossState {

	BeamAttack::BeamAttack(Boss* pOwner) noexcept
		: BossStateBase(pOwner)
	{
	}

	void BeamAttack::Enter()
	{
		ApplyNamedClip("boss_beem1");
		m_ElapsedTime = 0.0f;
		GetBoss()->SetAttackColliderActive(false);
		GetBoss()->SetAttackAmount(CombatTuning::Get().BeamAmount);
	}

	void BeamAttack::Update()
	{
		// 判定窓・硬直はCombatTuningから毎フレーム参照(エディタでの変更を即時反映).
		const float windup   = CombatTuning::Get().BeamWindup;
		const float active   = CombatTuning::Get().BeamActive;
		const float recovery = CombatTuning::Get().BeamRecovery;

		m_ElapsedTime += GameTime::GetDeltaTime();

		GetBoss()->RotateToTarget(AngleToTargetDeg(), ATTACK_ROTATE_SPEED);

		const bool in_active_window =
			m_ElapsedTime >= windup &&
			m_ElapsedTime <  windup + active;
		GetBoss()->SetAttackColliderActive(in_active_window);

		if (m_ElapsedTime >= windup + active + recovery)
		{
			const bool target_in_range = DistanceSqToTargetXZ() <= GetBoss()->GetLoseRange() * GetBoss()->GetLoseRange();
			GetBoss()->ChangeState(target_in_range ? BossState::eID::Move : BossState::eID::Idle);
		}
	}

	void BeamAttack::Exit()
	{
		GetBoss()->SetAttackColliderActive(false);
	}

} // namespace BossState
