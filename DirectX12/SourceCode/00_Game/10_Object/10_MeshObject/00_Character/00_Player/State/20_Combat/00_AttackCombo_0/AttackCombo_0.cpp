#include "AttackCombo_0.h"

#include "00_Game/10_Object/10_MeshObject/00_Character/00_Player/Player.h"

namespace {
	constexpr float ATTACK_AMOUNT = 25.0f;
}

namespace PlayerState {

AttackCombo_0::AttackCombo_0(Player* pOwner) noexcept
	: Combat(pOwner)
{
}

void AttackCombo_0::Enter()
{
	Combat::Enter();

	ApplyNamedClip("AttackCombo_0");
	GetPlayer()->SetAttackAmount(ATTACK_AMOUNT);
}

void AttackCombo_0::Update()
{
	Combat::Update();

	if (UpdateComboInput())
	{
		GetPlayer()->ChangeState(PlayerState::eID::AttackCombo_1);
		return;
	}

	if (m_CurrentTime >= m_ComboEndTime)
	{
		GetPlayer()->ChangeState(PlayerState::eID::Idle);
	}
}

} // namespace PlayerState
