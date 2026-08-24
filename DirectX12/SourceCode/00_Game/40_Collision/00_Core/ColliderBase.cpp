#include "ColliderBase.h"

#include <cmath>

#include "99_Utility/Transform/Transform.h"

ColliderBase::ColliderBase(const Transform* pOwnerTransform) noexcept
	: m_pOwnerTransform { pOwnerTransform }
{
}

// Yawのみの回転のため行列を作らずsin/cosで直接回す(XMMatrixRotationYと同じ結果).
DirectX::XMFLOAT3 ColliderBase::GetPosition() const noexcept
{
	using namespace DirectX;

	const float yaw   = m_pOwnerTransform->Rotation.y;
	const float sin_y = std::sin(yaw);
	const float cos_y = std::cos(yaw);

	const XMFLOAT3& offset = m_PositionOffset;

	XMFLOAT3 result;
	result.x = m_pOwnerTransform->Position.x + cos_y * offset.x + sin_y * offset.z;
	result.y = m_pOwnerTransform->Position.y + offset.y;
	result.z = m_pOwnerTransform->Position.z - sin_y * offset.x + cos_y * offset.z;
	return result;
}

// オフセットは回転不変な長さでしか使わないためYawに依存しない.
float ColliderBase::GetOffsetLength() const noexcept
{
	const DirectX::XMFLOAT3& o = m_PositionOffset;
	return std::sqrt(o.x * o.x + o.y * o.y + o.z * o.z);
}
