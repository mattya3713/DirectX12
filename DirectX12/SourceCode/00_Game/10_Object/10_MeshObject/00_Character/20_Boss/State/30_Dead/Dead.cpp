#include "Dead.h"

#include "00_Game/10_Object/10_MeshObject/00_Character/20_Boss/Boss.h"

namespace BossState {

Dead::Dead(Boss* pOwner) noexcept
	: BossStateBase(pOwner)
{
}

void Dead::Enter()
{
	ApplyNamedClip("boss_die");
	GetBoss()->SetAttackColliderActive(false);
	GetBoss()->SetDamageColliderActive(false);

	// Boss撃破時にラグドールを起動する(攻撃判定/AIは既に無効化済み).
	GetBoss()->ActivateDeathRagdoll();
}

} // namespace BossState
