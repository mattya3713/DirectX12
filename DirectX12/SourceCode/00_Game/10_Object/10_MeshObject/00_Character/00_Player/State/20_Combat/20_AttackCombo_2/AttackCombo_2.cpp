#include "AttackCombo_2.h"

#include "00_Game/10_Object/10_MeshObject/00_Character/00_Player/Player.h"

namespace {
	constexpr float ATTACK_AMOUNT = 40.0f;
}

namespace PlayerState {

AttackCombo_2::AttackCombo_2(Player* pOwner) noexcept
	: Combat(pOwner)
{
}

void AttackCombo_2::Enter()
{
	Combat::Enter();

	ApplyNamedClip("AttackCombo_2");
	GetPlayer()->SetAttackAmount(ATTACK_AMOUNT);
}

void AttackCombo_2::Update()
{
	Combat::Update();

	// コンボ入力が続く限り1段目へループする(3段で終わりの固定コンボではない).
	if (UpdateComboInput())
	{
		GetPlayer()->ChangeState(PlayerState::eID::AttackCombo_0);
		return;
	}

	if (m_CurrentTime >= m_ComboEndTime)
	{
		GetPlayer()->ChangeState(PlayerState::eID::Idle);
	}
}

} // namespace PlayerState
