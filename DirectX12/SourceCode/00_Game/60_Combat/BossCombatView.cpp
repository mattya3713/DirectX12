#include "BossCombatView.h"

#include "00_Game/10_Object/10_MeshObject/00_Character/20_Boss/Boss.h"

BossCombatView::BossCombatView(Boss& Boss) noexcept
	: m_pBoss{ &Boss }
{
}

DirectX::XMFLOAT3 BossCombatView::GetPosition() const noexcept
{
	return m_pBoss->GetPosition();
}

void BossCombatView::EnterParryReaction(const DirectX::XMFLOAT3& TargetPosition, float TargetYawDeg, float Duration) const
{
	m_pBoss->EnterParryReaction(TargetPosition, TargetYawDeg, Duration);
}
