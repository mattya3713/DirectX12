#include "stdafx.h"
#include "LookAtCamera.h"

#include "99_System/GameLoop/Time/Time.h"

namespace {
	constexpr float DEFAULT_DISTANCE    = 30.0f;
	constexpr float DEFAULT_ORBIT_SPEED = 1.0f;
	constexpr float PITCH_LIMIT         = DirectX::XM_PIDIV2 * 0.95f;
}

LookAtCamera::LookAtCamera()
	: CameraBase{}
	, m_Pivot		{ 0.0f, 0.0f, 0.0f }
	, m_Distance	{ DEFAULT_DISTANCE }
	, m_OrbitSpeed	{ DEFAULT_ORBIT_SPEED }
{
}

LookAtCamera::~LookAtCamera()
{
}

void LookAtCamera::SetPivot(const DirectX::XMFLOAT3& Pivot) noexcept
{
	m_Pivot = Pivot;
}

void LookAtCamera::SetDistance(float Distance) noexcept
{
	m_Distance = Distance;
}

void LookAtCamera::SetOrbitSpeed(float Speed) noexcept
{
	m_OrbitSpeed = Speed;
}

void LookAtCamera::Update()
{
	const float delta_time = GameTime::GetDeltaTime();

	float yaw   = GetYaw();
	float pitch = GetPitch();

	// 矢印キーで中心点を軸に周回する.
	if (GetAsyncKeyState(VK_LEFT)  & 0x8000) { yaw   -= m_OrbitSpeed * delta_time; }
	if (GetAsyncKeyState(VK_RIGHT) & 0x8000) { yaw   += m_OrbitSpeed * delta_time; }
	if (GetAsyncKeyState(VK_UP)    & 0x8000) { pitch -= m_OrbitSpeed * delta_time; }
	if (GetAsyncKeyState(VK_DOWN)  & 0x8000) { pitch += m_OrbitSpeed * delta_time; }

	pitch = std::clamp(pitch, -PITCH_LIMIT, PITCH_LIMIT);

	SetYaw(yaw);
	SetPitch(pitch);

	// Yaw・Pitchから中心点を基準にした位置を算出.
	DirectX::XMVECTOR v_rotation = DirectX::XMVectorSet(pitch, yaw, 0.0f, 0.0f);
	DirectX::XMMATRIX rotation_matrix = DirectX::XMMatrixRotationRollPitchYawFromVector(v_rotation);
	DirectX::XMVECTOR v_back = DirectX::XMVectorScale(rotation_matrix.r[2], -m_Distance);

	DirectX::XMVECTOR v_pivot    = DirectX::XMLoadFloat3(&m_Pivot);
	DirectX::XMVECTOR v_position = DirectX::XMVectorAdd(v_pivot, v_back);

	DirectX::XMFLOAT3 position = {};
	DirectX::XMStoreFloat3(&position, v_position);

	SetPosition(position);
	SetLook(m_Pivot);

	UpdateViewProjection();
}
