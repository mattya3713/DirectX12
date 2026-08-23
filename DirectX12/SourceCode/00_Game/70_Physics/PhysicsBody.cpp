#include "stdafx.h"
#include "PhysicsBody.h"

#include <cmath>

PhysicsBody::PhysicsBody() noexcept
	: m_Velocity{ 0.0f, 0.0f, 0.0f }
{
}

void PhysicsBody::ApplyGravity(float DeltaTime, float Gravity) noexcept
{
	m_Velocity.y -= Gravity * DeltaTime;
}

void PhysicsBody::ApplyDamping(float DeltaTime, float DampingRate) noexcept
{
	// 指数減衰(フレームレート非依存). 水平成分のみ(落下は減衰させない).
	const float damping = std::exp(-DampingRate * DeltaTime);
	m_Velocity.x *= damping;
	m_Velocity.z *= damping;
}

DirectX::XMFLOAT3 PhysicsBody::Integrate(float DeltaTime) const noexcept
{
	return { m_Velocity.x * DeltaTime, m_Velocity.y * DeltaTime, m_Velocity.z * DeltaTime };
}
