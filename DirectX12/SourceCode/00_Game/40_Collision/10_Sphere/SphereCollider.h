#pragma once

#include "00_Game/40_Collision/00_Core/ColliderBase.h"

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/11.
* @brief     : 球型の当たり判定(範囲攻撃の判定等に使う想定).
**********************************************************************************/

class SphereCollider final : public ColliderBase
{
public:
	explicit SphereCollider(const Transform* pOwnerTransform) noexcept;
	~SphereCollider() override = default;

	eShapeType GetShapeType() const noexcept override { return eShapeType::Sphere; }

	// 半径の取得・設定.
	float GetRadius() const noexcept { return m_Radius; }
	void SetRadius(float Radius) noexcept { m_Radius = Radius; }

	float GetBoundRadius() const noexcept override;

	CollisionInfo CheckCollision(const ColliderBase& Other) const override;

protected:
	CollisionInfo DispatchCollision(const BoxCollider& Other) const override;
	CollisionInfo DispatchCollision(const CapsuleCollider& Other) const override;
	CollisionInfo DispatchCollision(const SphereCollider& Other) const override;

private:
	float m_Radius = 0.5f; // 球の半径.
};
