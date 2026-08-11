#include "Dead.h"

#include "00_Game/10_Object/10_MeshObject/00_Character/10_Enemy/Enemy.h"

namespace EnemyState {

Dead::Dead(Enemy* pOwner) noexcept
	: EnemyStateBase(pOwner)
{
}

void Dead::Enter()
{
	GetEnemy()->SetAttackColliderActive(false);
	GetEnemy()->SetDamageColliderActive(false);
}

} // namespace EnemyState
