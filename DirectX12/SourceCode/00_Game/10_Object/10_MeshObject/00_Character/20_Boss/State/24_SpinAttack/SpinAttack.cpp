#include "SpinAttack.h"

#include "00_Game/10_Object/10_MeshObject/00_Character/20_Boss/Boss.h"
#include "00_Game/00_GameLoop/Time/Time.h"

namespace {
	// 「振り回している」印象を出すため判定有効時間を最長にした仮値. 威力は中程度.
	constexpr float ATTACK_WINDUP_TIME   = 0.5f;   // 予備動作.
	constexpr float ATTACK_ACTIVE_TIME   = 0.9f;   // 攻撃判定が有効な時間(既存攻撃の3倍前後で長い).
	constexpr float ATTACK_RECOVERY_TIME = 0.7f;   // 硬直(回って眩暈がする間).
	constexpr float ATTACK_TOTAL_TIME    = ATTACK_WINDUP_TIME + ATTACK_ACTIVE_TIME + ATTACK_RECOVERY_TIME;
	constexpr float ATTACK_AMOUNT        = 20.0f;
	constexpr float ATTACK_ROTATE_SPEED  = 90.0f;  // 度/秒(回転中はゆっくり追い回し. 長時間なので弱め).
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
		GetBoss()->SetAttackAmount(ATTACK_AMOUNT);
	}

	void SpinAttack::Update()
	{
		m_ElapsedTime += GameTime::GetDeltaTime();

		// その場で振り回すため移動はせず、向きだけゆっくり合わせる.
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

	void SpinAttack::Exit()
	{
		GetBoss()->SetAttackColliderActive(false);
	}

} // namespace BossState
