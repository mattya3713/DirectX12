#include "ColliderBase.h"

#include "99_Utility/Transform/Transform.h"

ColliderBase::ColliderBase(const Transform* pOwnerTransform) noexcept
	: m_pOwnerTransform { pOwnerTransform }
{
}

DirectX::XMFLOAT3 ColliderBase::GetPosition() const noexcept
{
	using namespace DirectX;

	const XMVECTOR v_position = XMLoadFloat3(&m_pOwnerTransform->Position);
	const XMVECTOR v_offset   = XMLoadFloat3(&m_PositionOffset);

	// 持ち主の回転(Yaw)でオフセットを回転させてから加算する.
	const XMMATRIX rotation_matrix  = XMMatrixRotationY(m_pOwnerTransform->Rotation.y);
	const XMVECTOR v_rotated_offset = XMVector3TransformCoord(v_offset, rotation_matrix);

	XMFLOAT3 result;
	XMStoreFloat3(&result, XMVectorAdd(v_position, v_rotated_offset));
	return result;
}
