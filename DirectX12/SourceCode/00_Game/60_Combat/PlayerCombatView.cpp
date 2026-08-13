#include "PlayerCombatView.h"

#include "00_Game/10_Object/10_MeshObject/00_Character/00_Player/Player.h"

PlayerCombatView::PlayerCombatView(Player& Player) noexcept
	: m_pPlayer{ &Player }
{
}

DirectX::XMFLOAT3 PlayerCombatView::GetPosition() const noexcept
{
	return m_pPlayer->GetPosition();
}

void PlayerCombatView::EnterParryReaction(const DirectX::XMFLOAT3& TargetPosition, float TargetYawDeg, float Duration) const noexcept
{
	m_pPlayer->EnterParryReaction(TargetPosition, TargetYawDeg, Duration);
}
