#include "AttackCombo_0.h"

#include "00_Game/10_Object/10_MeshObject/00_Character/00_Player/Player.h"
#include "00_Game/60_Combat/CombatTuning.h"

namespace PlayerState {

AttackCombo_0::AttackCombo_0(Player* pOwner) noexcept
	: Combat(pOwner)
{
}

void AttackCombo_0::Enter()
{
	Combat::Enter();

	ApplyComboSpeedToAnimation();
	DecideRushDirection(false); // 1段目はBoss方向へ突進(方向転換なし).

	ApplyNamedClip("player_attack1");
	GetPlayer()->SetAttackAmount(CombatTuning::Get().Attack0Amount);
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
