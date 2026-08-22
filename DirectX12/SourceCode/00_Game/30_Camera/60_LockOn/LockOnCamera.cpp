#include "stdafx.h"
#include "LockOnCamera.h"

#include "00_Game/00_GameLoop/Time/Time.h"

namespace {
	constexpr float DEFAULT_DISTANCE    = 12.0f;
	constexpr float DEFAULT_HEIGHT      = 5.0f;
	constexpr float DEFAULT_FOLLOW_SPEED = 8.0f;
	// PlayerとBossがほぼ同位置とみなす閾値(方向確定不能時の既存向き維持用).
	constexpr float MIN_SEPARATION_SQ   = 0.0001f;
}

LockOnCamera::LockOnCamera()
	: CameraBase{}
	, m_PlayerPosition{ 0.0f, 0.0f, 0.0f }
	, m_BossPosition  { 0.0f, 0.0f, 1.0f }
	, m_LookOffset    { 0.0f, 2.5f, 0.0f }
	, m_Distance      { DEFAULT_DISTANCE }
	, m_Height        { DEFAULT_HEIGHT }
	, m_FollowSpeed   { DEFAULT_FOLLOW_SPEED }
	, m_IsInitialized { false }
{
}

LockOnCamera::~LockOnCamera()
{
}

void LockOnCamera::SetPlayerPosition(const DirectX::XMFLOAT3& Position) noexcept
{
	m_PlayerPosition = Position;
}

void LockOnCamera::SetBossPosition(const DirectX::XMFLOAT3& Position) noexcept
{
	m_BossPosition = Position;
}

void LockOnCamera::SetLookOffset(const DirectX::XMFLOAT3& Offset) noexcept
{
	m_LookOffset = Offset;
}

void LockOnCamera::SetDistance(float Distance) noexcept
{
	m_Distance = Distance;
}

void LockOnCamera::SetHeight(float Height) noexcept
{
	m_Height = Height;
}

void LockOnCamera::SetFollowSpeed(float Speed) noexcept
{
	m_FollowSpeed = Speed;
}

void LockOnCamera::Update()
{
	// Bossを画面内に収めるため、PlayerからBossへ向う水平方向の逆側にカメラを置く.
	const float to_boss_x = m_BossPosition.x - m_PlayerPosition.x;
	const float to_boss_z = m_BossPosition.z - m_PlayerPosition.z;
	const float separation_sq = to_boss_x * to_boss_x + to_boss_z * to_boss_z;

	float dir_x = 0.0f;
	float dir_z = 1.0f;
	if (separation_sq > MIN_SEPARATION_SQ) {
		const float length = std::sqrt(separation_sq);
		dir_x = to_boss_x / length;
		dir_z = to_boss_z / length;
	}

	DirectX::XMFLOAT3 desired_position{};
	desired_position.x = m_PlayerPosition.x - dir_x * m_Distance;
	desired_position.y = m_PlayerPosition.y + m_Height;
	desired_position.z = m_PlayerPosition.z - dir_z * m_Distance;

	// 初回はスナップし、以降はフレームレート非依存の補間で追従させる.
	if (m_IsInitialized == false) {
		SetPosition(desired_position);
		m_IsInitialized = true;
	}
	else {
		const float delta_time = GameTime::GetDeltaTime();
		const float t = 1.0f - std::exp(-m_FollowSpeed * delta_time);

		DirectX::XMFLOAT3 position = GetPosition();
		position.x += (desired_position.x - position.x) * t;
		position.y += (desired_position.y - position.y) * t;
		position.z += (desired_position.z - position.z) * t;
		SetPosition(position);
	}

	// 注視点は常にBoss(オフセット込み).
	DirectX::XMFLOAT3 look_pos{};
	look_pos.x = m_BossPosition.x + m_LookOffset.x;
	look_pos.y = m_BossPosition.y + m_LookOffset.y;
	look_pos.z = m_BossPosition.z + m_LookOffset.z;
	SetLook(look_pos);

	UpdateViewProjection();
}
