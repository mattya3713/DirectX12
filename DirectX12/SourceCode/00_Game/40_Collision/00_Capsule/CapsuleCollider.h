#pragma once

#include "00_Game/40_Collision/00_Core/ColliderBase.h"

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/11.
* @brief     : カプセル型の当たり判定(キャラクターの本体等に使う想定).
**********************************************************************************/

class CapsuleCollider final : public ColliderBase
{
public:
	explicit CapsuleCollider(const Transform* pOwnerTransform) noexcept;
	~CapsuleCollider() override = default;

	eShapeType GetShapeType() const noexcept override { return eShapeType::Capsule; }

	// 半径・高さの取得・設定.
	float GetRadius() const noexcept { return m_Radius; }
	float GetHeight() const noexcept { return m_Height; }
	void SetRadius(float Radius) noexcept { m_Radius = Radius; }
	void SetHeight(float Height) noexcept { m_Height = Height; }

	// カプセル中心線分の始点・終点をワールド座標で取得する(CollisionMathから使う).
	DirectX::XMVECTOR GetSegmentStart() const noexcept;
	DirectX::XMVECTOR GetSegmentEnd() const noexcept;

	CollisionInfo CheckCollision(const ColliderBase& Other) const override;

protected:
	CollisionInfo DispatchCollision(const BoxCollider& Other) const override;
	CollisionInfo DispatchCollision(const CapsuleCollider& Other) const override;
	CollisionInfo DispatchCollision(const SphereCollider& Other) const override;

private:
	float m_Radius = 0.5f; // カプセルの半径.
	float m_Height = 2.0f; // カプセルの全高(半球部分を含む. 中心線分の長さはHeight - 2*Radius).
};
