#include "BoxCollider.h"

#include "00_Game/40_Collision/00_Core/CollisionMath.h"
#include "99_Utility/Transform/Transform.h"

BoxCollider::BoxCollider(const Transform* pOwnerTransform) noexcept
	: ColliderBase(pOwnerTransform)
{
}

DirectX::XMVECTOR BoxCollider::GetLocalAxisX() const noexcept
{
	using namespace DirectX;
	const XMMATRIX rotation_matrix = XMMatrixRotationY(m_pOwnerTransform->Rotation.y);
	return XMVector3TransformNormal(XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), rotation_matrix);
}

DirectX::XMVECTOR BoxCollider::GetLocalAxisZ() const noexcept
{
	using namespace DirectX;
	const XMMATRIX rotation_matrix = XMMatrixRotationY(m_pOwnerTransform->Rotation.y);
	return XMVector3TransformNormal(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), rotation_matrix);
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
