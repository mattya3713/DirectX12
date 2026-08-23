#include "BossStateBase.h"

#include <cmath>

#include "00_Game/10_Object/10_MeshObject/00_Character/20_Boss/Boss.h"

BossStateBase::BossStateBase(Boss* pOwner) noexcept
	: StateBase<Boss>(pOwner)
{
}

void BossStateBase::ApplyNamedClip(const char* ClipName) const
{
	GetBoss()->PlayNamedClip(ClipName);
}

float BossStateBase::DistanceSqToTargetXZ() const noexcept
{
	const DirectX::XMFLOAT3& self_pos   = m_pOwner->GetPosition();
	const DirectX::XMFLOAT3& target_pos = m_pOwner->GetTargetPos();

	const float dx = target_pos.x - self_pos.x;
	const float dz = target_pos.z - self_pos.z;

	return dx * dx + dz * dz;
}

float BossStateBase::DistanceToTargetXZ() const noexcept
{
	return std::sqrtf(DistanceSqToTargetXZ());
}

float BossStateBase::AngleToTargetDeg() const noexcept
{
	const DirectX::XMFLOAT3& self_pos   = m_pOwner->GetPosition();
	const DirectX::XMFLOAT3& target_pos = m_pOwner->GetTargetPos();

	const float dx = target_pos.x - self_pos.x;
	const float dz = target_pos.z - self_pos.z;

	// atan2f(x, z): 0度 = +Z軸方向(前方)を正面とする(PlayerStateBaseと同じ規約).
	return std::atan2f(dx, dz) * (180.0f / DirectX::XM_PI);
}
