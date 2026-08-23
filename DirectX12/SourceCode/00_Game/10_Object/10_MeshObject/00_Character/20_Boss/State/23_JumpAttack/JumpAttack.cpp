#include "JumpAttack.h"

#include <algorithm>
#include <cmath>

#include "00_Game/10_Object/10_MeshObject/00_Character/20_Boss/Boss.h"
#include "00_Game/00_GameLoop/Time/Time.h"

namespace {
	constexpr float ATTACK_WINDUP_TIME   = 0.4f;   // 予備動作(しゃがみ溜め. 地面に留まる).
	constexpr float AIR_TIME             = 0.9f;   // 滞空時間(この間放物線移動し、終端で必ず着地する).
	constexpr float JUMP_HEIGHT          = 1.8f;   // 最大跳躍高度(単位).
	constexpr float LANDING_ACTIVE_BEFORE = 0.1f;  // 着地の何秒前から判定を出すか.
	constexpr float ATTACK_ACTIVE_AFTER   = 0.15f; // 着地後も判定を残す時間(衝撃波の見た目).
	constexpr float ATTACK_RECOVERY_TIME = 0.6f;   // 着地後の硬直.
	constexpr float ATTACK_AMOUNT        = 30.0f;
}

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
		GetBoss()->SetAttackAmount(ATTACK_AMOUNT);
	}

	void JumpAttack::Update()
	{
		const float prev_elapsed = m_ElapsedTime;
		m_ElapsedTime += GameTime::GetDeltaTime();

		// ===== 跳躍(滞空区間は解析式で高度を決めるため必ず地面へ戻る) =====
		if (m_ElapsedTime > ATTACK_WINDUP_TIME && prev_elapsed <= ATTACK_WINDUP_TIME + AIR_TIME)
		{
			// 放物線: h(t) = 4 * H * u * (1 - u) / ... (u=進行率0..1で0→H→0).
			auto height_at = [](float air_progress) {
				return 4.0f * JUMP_HEIGHT * air_progress * (1.0f - air_progress);
			};
			const float progress_prev = std::clamp((prev_elapsed - ATTACK_WINDUP_TIME) / AIR_TIME, 0.0f, 1.0f);
			const float progress_now  = std::clamp((m_ElapsedTime - ATTACK_WINDUP_TIME) / AIR_TIME, 0.0f, 1.0f);

			const float delta_height = height_at(progress_now) - height_at(progress_prev);
			GetBoss()->AddPosition({ 0.0f, delta_height, 0.0f });
			m_PrevHeight = height_at(progress_now);
		}

		// ===== 判定ウィンドウ(着地の直前〜着地後わずか) =====
		const float landing_time = ATTACK_WINDUP_TIME + AIR_TIME;
		const bool in_active_window =
			m_ElapsedTime >= landing_time - LANDING_ACTIVE_BEFORE &&
			m_ElapsedTime <  landing_time + ATTACK_ACTIVE_AFTER;
		GetBoss()->SetAttackColliderActive(in_active_window);

		// ===== 硬直→復帰 =====
		const float total_time = landing_time + ATTACK_ACTIVE_AFTER + ATTACK_RECOVERY_TIME;
		if (m_ElapsedTime >= total_time)
		{
			const bool target_in_range = DistanceToTargetXZ() <= GetBoss()->GetLoseRange();
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
