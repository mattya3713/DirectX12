#include "stdafx.h"
#include "ThirdPersonCamera.h"

#include "99_System/GameLoop/Time/Time.h"
#include "99_Utility/DirectXMath/DirectXMathExpansion.h"

namespace {
	constexpr float DEFAULT_DISTANCE    = 15.0f;
	constexpr float DEFAULT_ORBIT_SPEED = 2.0f;
	constexpr float PITCH_LIMIT_MIN     = -DirectX::XM_PIDIV2 * 0.05f;
	constexpr float PITCH_LIMIT_MAX     =  DirectX::XM_PIDIV2 * 0.6f;
}

ThirdPersonCamera::ThirdPersonCamera()
	: CameraBase{}
	, m_TargetPosition	{ 0.0f, 0.0f, 0.0f }
	, m_LookOffset		{ 0.0f, 2.5f, 0.0f }
	, m_Distance		{ DEFAULT_DISTANCE }
	, m_OrbitSpeed		{ DEFAULT_ORBIT_SPEED }
{
}

ThirdPersonCamera::~ThirdPersonCamera()
{
}

void ThirdPersonCamera::SetTargetPosition(const DirectX::XMFLOAT3& Position) noexcept
{
	m_TargetPosition = Position;
}

void ThirdPersonCamera::SetLookOffset(const DirectX::XMFLOAT3& Offset) noexcept
{
	m_LookOffset = Offset;
}

void ThirdPersonCamera::SetDistance(float Distance) noexcept
{
	m_Distance = Distance;
}

void ThirdPersonCamera::SetOrbitSpeed(float Speed) noexcept
{
	m_OrbitSpeed = Speed;
}

void ThirdPersonCamera::Update()
{
	const float delta_time = GameTime::GetDeltaTime();

	float yaw   = GetYaw();
	float pitch = GetPitch();

	// 矢印キーでターゲットを軸に周回する.
	if (GetAsyncKeyState(VK_LEFT)  & 0x8000) { yaw   -= m_OrbitSpeed * delta_time; }
	if (GetAsyncKeyState(VK_RIGHT) & 0x8000) { yaw   += m_OrbitSpeed * delta_time; }
	if (GetAsyncKeyState(VK_UP)    & 0x8000) { pitch -= m_OrbitSpeed * delta_time; }
	if (GetAsyncKeyState(VK_DOWN)  & 0x8000) { pitch += m_OrbitSpeed * delta_time; }

	pitch = std::clamp(pitch, PITCH_LIMIT_MIN, PITCH_LIMIT_MAX);

	SetYaw(yaw);
	SetPitch(pitch);

	// 注視点 = ターゲット座標 + オフセット.
	DirectX::XMFLOAT3 look_pos = m_TargetPosition + m_LookOffset;

	// Yaw・Pitchから注視点を基準にした位置を算出.
	DirectX::XMVECTOR v_rotation = DirectX::XMVectorSet(pitch, yaw, 0.0f, 0.0f);
	DirectX::XMMATRIX rotation_matrix = DirectX::XMMatrixRotationRollPitchYawFromVector(v_rotation);
	DirectX::XMVECTOR v_back = DirectX::XMVectorScale(rotation_matrix.r[2], -m_Distance);

	DirectX::XMVECTOR v_look     = DirectX::XMLoadFloat3(&look_pos);
	DirectX::XMVECTOR v_position = DirectX::XMVectorAdd(v_look, v_back);

	DirectX::XMFLOAT3 position = {};
	DirectX::XMStoreFloat3(&position, v_position);

	SetPosition(position);
	SetLook(look_pos);

	UpdateViewProjection();
}
