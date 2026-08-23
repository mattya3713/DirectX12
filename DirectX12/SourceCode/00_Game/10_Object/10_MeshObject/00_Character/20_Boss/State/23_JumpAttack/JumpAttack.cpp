#include "JumpAttack.h"

#include <algorithm>
#include <cmath>

#include "00_Game/10_Object/10_MeshObject/00_Character/20_Boss/Boss.h"
#include "00_Game/00_GameLoop/Time/Time.h"
#include "00_Game/60_Combat/CombatTuning.h"

namespace BossState {

	JumpAttack::JumpAttack(Boss* pOwner) noexcept
		: BossStateBase(pOwner)
	{
	}

	void JumpAttack::Enter()
	{
		ApplyNamedClip("boss_jump_attack1");
		m_ElapsedTime = 0.0f;
		m_PrevHeight  = 0.0f;
		GetBoss()->SetAttackColliderActive(false);
		GetBoss()->SetAttackAmount(CombatTuning::Get().JumpAmount);
	}

	void JumpAttack::Update()
	{
		// タイミング系はCombatTuningから毎フレーム参照(エディタでの変更を即時反映).
		const float windup        = CombatTuning::Get().JumpCrouch;
		const float air_time      = CombatTuning::Get().JumpAirTime;
		const float max_height    = CombatTuning::Get().JumpHeight;
		const float active_before = CombatTuning::Get().JumpLandingActiveBefore;
		const float active_after  = CombatTuning::Get().JumpLandingActiveAfter;
		const float recovery      = CombatTuning::Get().JumpRecovery;

		const float prev_elapsed = m_ElapsedTime;
		m_ElapsedTime += GameTime::GetDeltaTime();

		// ===== 跳躍(滞空区間は解析式で高度を決めるため必ず地面へ戻る) =====
		if (m_ElapsedTime > windup && prev_elapsed <= windup + air_time)
		{
			// 放物線: h(u) = 4 * H * u * (1 - u) (u=進行率0..1で0→H→0).
			auto height_at = [&](float air_progress) {
				return 4.0f * max_height * air_progress * (1.0f - air_progress);
			};
			const float progress_prev = std::clamp((prev_elapsed - windup) / air_time, 0.0f, 1.0f);
			const float progress_now  = std::clamp((m_ElapsedTime - windup) / air_time, 0.0f, 1.0f);

			const float delta_height = height_at(progress_now) - height_at(progress_prev);
			GetBoss()->AddPosition({ 0.0f, delta_height, 0.0f });
			m_PrevHeight = height_at(progress_now);
		}

		// ===== 判定ウィンドウ(着地の直前〜着地後わずか) =====
		const float landing_time = windup + air_time;
		const bool in_active_window =
			m_ElapsedTime >= landing_time - active_before &&
			m_ElapsedTime <  landing_time + active_after;
		GetBoss()->SetAttackColliderActive(in_active_window);

		// ===== 硬直→復帰 =====
		const float total_time = landing_time + active_after + recovery;
		if (m_ElapsedTime >= total_time)
		{
			const bool target_in_range = DistanceSqToTargetXZ() <= GetBoss()->GetLoseRange() * GetBoss()->GetLoseRange();
			GetBoss()->ChangeState(target_in_range ? BossState::eID::Move : BossState::eID::Idle);
		}
	}

	void JumpAttack::Exit()
	{
		// 念のため高度を地面へ戻しておく(解析式なので通常ズレはない).
		Transform transform = GetBoss()->GetTransform();
		transform.Position.y = 0.0f;
		GetBoss()->SetTransform(transform);

		GetBoss()->SetAttackColliderActive(false);
	}

} // namespace BossState
