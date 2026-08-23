#include "AttackCombo_1.h"

#include "00_Game/10_Object/10_MeshObject/00_Character/00_Player/Player.h"

namespace {
	constexpr float ATTACK_AMOUNT = 30.0f;
}

namespace PlayerState {

AttackCombo_1::AttackCombo_1(Player* pOwner) noexcept
	: Combat(pOwner)
{
}

void AttackCombo_1::Enter()
{
	Combat::Enter();

	ApplyComboSpeedToAnimation();
	DecideRushDirection(false); // 2段目もBoss方向へ突進(方向転換なし).

	ApplyNamedClip("player_attack2");
	GetPlayer()->SetAttackAmount(ATTACK_AMOUNT);
}

void AttackCombo_1::Update()
{
	Combat::Update();

	if (UpdateComboInput())
	{
		GetPlayer()->ChangeState(PlayerState::eID::AttackCombo_2);
		return;
	}

	if (m_CurrentTime >= m_ComboEndTime)
	{
		GetPlayer()->ChangeState(PlayerState::eID::Idle);
	}
}

} // namespace PlayerState
