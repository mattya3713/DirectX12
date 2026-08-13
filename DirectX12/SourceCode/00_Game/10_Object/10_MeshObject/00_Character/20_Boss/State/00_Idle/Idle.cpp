#include "Idle.h"

#include "00_Game/10_Object/10_MeshObject/00_Character/20_Boss/Boss.h"

namespace BossState {

Idle::Idle(Boss* pOwner) noexcept
	: BossStateBase(pOwner)
{
}

void Idle::Enter()
{
	ApplyNamedClip("boss_idle");
}

void Idle::Update()
{
	if (DistanceToTargetXZ() <= GetBoss()->GetAggroRange())
	{
		GetBoss()->ChangeState(BossState::eID::Move);
	}
}

} // namespace BossState
