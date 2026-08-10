#include "CapsuleCollider.h"

#include "00_Game/40_Collision/00_Core/CollisionMath.h"
#include "99_Utility/Transform/Transform.h"

CapsuleCollider::CapsuleCollider(const Transform* pOwnerTransform) noexcept
	: ColliderBase(pOwnerTransform)
{
}

DirectX::XMVECTOR CapsuleCollider::GetSegmentStart() const noexcept
{
	using namespace DirectX;

	float segment_length = m_Height - 2.0f * m_Radius;
	if (segment_length < 0.0f) { segment_length = 0.0f; }
	const float half_segment_length = segment_length * 0.5f;

	const XMFLOAT3 center   = GetPosition();
	const XMVECTOR v_center = XMLoadFloat3(&center);

	const XMMATRIX rotation_matrix = XMMatrixRotationY(m_pOwnerTransform->Rotation.y);
	const XMVECTOR v_local          = XMVectorSet(0.0f, -half_segment_length, 0.0f, 0.0f);
	const XMVECTOR v_rotated        = XMVector3TransformNormal(v_local, rotation_matrix);

	return XMVectorAdd(v_center, v_rotated);
}

DirectX::XMVECTOR CapsuleCollider::GetSegmentEnd() const noexcept
{
	using namespace DirectX;

	float segment_length = m_Height - 2.0f * m_Radius;
	if (segment_length < 0.0f) { segment_length = 0.0f; }
	const float half_segment_length = segment_length * 0.5f;

	const XMFLOAT3 center   = GetPosition();
	const XMVECTOR v_center = XMLoadFloat3(&center);

	const XMMATRIX rotation_matrix = XMMatrixRotationY(m_pOwnerTransform->Rotation.y);
	const XMVECTOR v_local          = XMVectorSet(0.0f, half_segment_length, 0.0f, 0.0f);
	const XMVECTOR v_rotated        = XMVector3TransformNormal(v_local, rotation_matrix);

	return XMVectorAdd(v_center, v_rotated);
}

CollisionInfo CapsuleCollider::CheckCollision(const ColliderBase& Other) const
{
	if (!ShouldCollide(Other)) { return {}; }

	return Other.DispatchCollision(*this);
}

CollisionInfo CapsuleCollider::DispatchCollision(const BoxCollider& Other) const
{
	// CollisionMathの引数順(Boxが先)を守るため、ここでOther(Box)を第1引数にする.
	return CollisionMath::TestBoxVsCapsule(Other, *this);
}

CollisionInfo CapsuleCollider::DispatchCollision(const CapsuleCollider& Other) const
{
	return CollisionMath::TestCapsuleVsCapsule(*this, Other);
}

CollisionInfo CapsuleCollider::DispatchCollision(const SphereCollider& Other) const
{
	return CollisionMath::TestCapsuleVsSphere(*this, Other);
}
