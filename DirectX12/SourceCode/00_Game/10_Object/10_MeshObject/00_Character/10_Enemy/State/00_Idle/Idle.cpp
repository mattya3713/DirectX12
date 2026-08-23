#include "Idle.h"

#include "00_Game/10_Object/10_MeshObject/00_Character/10_Enemy/Enemy.h"

namespace EnemyState {

Idle::Idle(Enemy* pOwner) noexcept
	: EnemyStateBase(pOwner)
{
}

void Idle::Update()
{
	if (DistanceSqToTargetXZ() <= GetEnemy()->GetAggroRange() * GetEnemy()->GetAggroRange())
	{
		GetEnemy()->ChangeState(EnemyState::eID::Chase);
	}
}

} // namespace EnemyState
