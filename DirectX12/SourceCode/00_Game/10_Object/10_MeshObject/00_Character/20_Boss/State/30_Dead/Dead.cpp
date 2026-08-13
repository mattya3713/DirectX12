#include "Dead.h"

#include "00_Game/10_Object/10_MeshObject/00_Character/20_Boss/Boss.h"

namespace BossState {

Dead::Dead(Boss* pOwner) noexcept
	: BossStateBase(pOwner)
{
}

void Dead::Enter()
{
	GetBoss()->SetAttackColliderActive(false);
	GetBoss()->SetDamageColliderActive(false);
}

} // namespace BossState
