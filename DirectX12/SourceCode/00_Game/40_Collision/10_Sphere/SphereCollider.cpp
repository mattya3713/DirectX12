#include "SphereCollider.h"

#include "00_Game/40_Collision/00_Core/CollisionMath.h"

SphereCollider::SphereCollider(const Transform* pOwnerTransform) noexcept
	: ColliderBase(pOwnerTransform)
{
}

CollisionInfo SphereCollider::CheckCollision(const ColliderBase& Other) const
{
	if (!ShouldCollide(Other)) { return {}; }

	return Other.DispatchCollision(*this);
}

CollisionInfo SphereCollider::DispatchCollision(const BoxCollider& Other) const
{
	// CollisionMathの引数順(Boxが先)を守るため、ここでOther(Box)を第1引数にする.
	return CollisionMath::TestBoxVsSphere(Other, *this);
}

CollisionInfo SphereCollider::DispatchCollision(const CapsuleCollider& Other) const
{
	// CollisionMathの引数順(Capsuleが先)を守るため、ここでOther(Capsule)を第1引数にする.
	return CollisionMath::TestCapsuleVsSphere(Other, *this);
}

CollisionInfo SphereCollider::DispatchCollision(const SphereCollider& Other) const
{
	return CollisionMath::TestSphereVsSphere(*this, Other);
}
