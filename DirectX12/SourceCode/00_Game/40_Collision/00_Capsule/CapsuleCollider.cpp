#include "CapsuleCollider.h"

#include <cmath>

#include "00_Game/40_Collision/00_Core/CollisionMath.h"
#include "99_Utility/Transform/Transform.h"

CapsuleCollider::CapsuleCollider(const Transform* pOwnerTransform) noexcept
	: ColliderBase(pOwnerTransform)
{
}

// 中心線分は構造上常にワールドY軸に平行(Yaw回転しても垂直ベクトルは不変).
void CapsuleCollider::GetSegment(DirectX::XMVECTOR& Start, DirectX::XMVECTOR& End) const noexcept
{
	using namespace DirectX;

	float segment_length = m_Height - 2.0f * m_Radius;
	if (segment_length < 0.0f) { segment_length = 0.0f; }
	const float half_segment_length = segment_length * 0.5f;

	const XMFLOAT3 center = GetPosition();

	Start = XMVectorSet(center.x, center.y - half_segment_length, center.z, 0.0f);
	End   = XMVectorSet(center.x, center.y + half_segment_length, center.z, 0.0f);
}

DirectX::XMVECTOR CapsuleCollider::GetSegmentStart() const noexcept
{
	DirectX::XMVECTOR start, end;
	GetSegment(start, end);
	return start;
}

DirectX::XMVECTOR CapsuleCollider::GetSegmentEnd() const noexcept
{
	DirectX::XMVECTOR start, end;
	GetSegment(start, end);
	return end;
}

// 半球が全高からはみ出す場合(Height < 2*Radius)は球としての半径を採用する.
float CapsuleCollider::GetBoundRadius() const noexcept
{
	return std::max(m_Height * 0.5f, m_Radius) + GetOffsetLength();
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
