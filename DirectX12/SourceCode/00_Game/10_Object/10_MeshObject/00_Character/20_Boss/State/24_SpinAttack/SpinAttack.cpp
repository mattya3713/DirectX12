#include "SpinAttack.h"

#include "00_Game/10_Object/10_MeshObject/00_Character/20_Boss/Boss.h"
#include "00_Game/00_GameLoop/Time/Time.h"
#include "00_Game/60_Combat/CombatTuning.h"

namespace {
	constexpr float ATTACK_ROTATE_SPEED = 90.0f; // 度/秒(回転中はゆっくり追い回し. 長時間なので弱め).
}

namespace BossState {

	SpinAttack::SpinAttack(Boss* pOwner) noexcept
		: BossStateBase(pOwner)
	{
	}

	void SpinAttack::Enter()
	{
		ApplyNamedClip("boss_spin_attack1");
		m_ElapsedTime = 0.0f;
		GetBoss()->SetAttackColliderActive(false);
		GetBoss()->SetAttackAmount(CombatTuning::Get().SpinAmount);
	}

	void SpinAttack::Update()
	{
		// 判定窓・硬直はCombatTuningから毎フレーム参照(エディタでの変更を即時反映).
		const float windup   = CombatTuning::Get().SpinWindup;
		const float active   = CombatTuning::Get().SpinActive;
		const float recovery = CombatTuning::Get().SpinRecovery;

		m_ElapsedTime += GameTime::GetDeltaTime();

		// その場で振り回すため移動はせず、向きだけゆっくり合わせる.
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

	void SpinAttack::Exit()
	{
		GetBoss()->SetAttackColliderActive(false);
	}

} // namespace BossState
