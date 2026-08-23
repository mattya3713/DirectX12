#pragma once

#include "00_Game/40_Collision/00_Core/ColliderBase.h"

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/11.
* @brief     : 直方体(OBB)の当たり判定. 持ち主の回転はYaw(Y軸周り)のみを反映する
*            : (このプロジェクトの他のコライダーと同じ制約. Pitch/Rollには対応しない).
**********************************************************************************/

class BoxCollider final : public ColliderBase
{
public:
	explicit BoxCollider(const Transform* pOwnerTransform) noexcept;
	~BoxCollider() override = default;

	eShapeType GetShapeType() const noexcept override { return eShapeType::Box; }

	// 辺の長さ(全長. 半径ではない)の取得・設定.
	const DirectX::XMFLOAT3& GetSize() const noexcept { return m_Size; }
	void SetSize(const DirectX::XMFLOAT3& Size) noexcept { m_Size = Size; }

	// ワールド空間でのローカルX軸・Z軸(単位ベクトル、持ち主のYawで回転済み)を取得する
	// (CollisionMathから使う. Y軸はYaw回転で不変なため常に(0,1,0)).
	DirectX::XMVECTOR GetLocalAxisX() const noexcept;
	DirectX::XMVECTOR GetLocalAxisZ() const noexcept;

	// 両軸をsin/cos1回でまとめて取得する(判定関数側のホットパス用).
	void GetLocalAxes(DirectX::XMVECTOR& AxisX, DirectX::XMVECTOR& AxisZ) const noexcept;

	float GetBoundRadius() const noexcept override;

	CollisionInfo CheckCollision(const ColliderBase& Other) const override;

protected:
	CollisionInfo DispatchCollision(const BoxCollider& Other) const override;
	CollisionInfo DispatchCollision(const CapsuleCollider& Other) const override;
	CollisionInfo DispatchCollision(const SphereCollider& Other) const override;

private:
	DirectX::XMFLOAT3 m_Size { 1.0f, 1.0f, 1.0f }; // 辺の長さ(X/Y/Z、全長).
};
