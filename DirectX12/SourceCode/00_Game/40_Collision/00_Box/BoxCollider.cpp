#include "BoxCollider.h"

#include <cmath>

#include "00_Game/40_Collision/00_Core/CollisionMath.h"
#include "99_Utility/Transform/Transform.h"

BoxCollider::BoxCollider(const Transform* pOwnerTransform) noexcept
	: ColliderBase(pOwnerTransform)
{
}

// XMMatrixRotationYのX/Z基底と同じ結果をsin/cos1回で出す(旧実装は軸ごとに行列を生成していた).
void BoxCollider::GetLocalAxes(DirectX::XMVECTOR& AxisX, DirectX::XMVECTOR& AxisZ) const noexcept
{
	using namespace DirectX;

	const float yaw   = m_pOwnerTransform->Rotation.y;
	const float sin_y = std::sin(yaw);
	const float cos_y = std::cos(yaw);

	AxisX = XMVectorSet(cos_y, 0.0f, -sin_y, 0.0f);
	AxisZ = XMVectorSet(sin_y, 0.0f,  cos_y, 0.0f);
}

DirectX::XMVECTOR BoxCollider::GetLocalAxisX() const noexcept
{
	DirectX::XMVECTOR axis_x, axis_z;
	GetLocalAxes(axis_x, axis_z);
	return axis_x;
}

DirectX::XMVECTOR BoxCollider::GetLocalAxisZ() const noexcept
{
	DirectX::XMVECTOR axis_x, axis_z;
	GetLocalAxes(axis_x, axis_z);
	return axis_z;
}

// Yaw回転での最大半径は半対角線長で押さえられる.
float BoxCollider::GetBoundRadius() const noexcept
{
	using namespace DirectX;
	const float half_x = m_Size.x * 0.5f;
	const float half_y = m_Size.y * 0.5f;
	const float half_z = m_Size.z * 0.5f;
	return std::sqrt(half_x * half_x + half_y * half_y + half_z * half_z) + GetOffsetLength();
}

CollisionInfo BoxCollider::CheckCollision(const ColliderBase& Other) const
{
	if (!ShouldCollide(Other)) { return {}; }

	return Other.DispatchCollision(*this);
}

CollisionInfo BoxCollider::DispatchCollision(const BoxCollider& Other) const
{
	return CollisionMath::TestBoxVsBox(*this, Other);
}

CollisionInfo BoxCollider::DispatchCollision(const CapsuleCollider& Other) const
{
	return CollisionMath::TestBoxVsCapsule(*this, Other);
}

CollisionInfo BoxCollider::DispatchCollision(const SphereCollider& Other) const
{
	return CollisionMath::TestBoxVsSphere(*this, Other);
}
