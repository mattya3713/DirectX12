#include "stdafx.h"
#include "ThirdPersonCamera.h"

#include "00_Game/00_GameLoop/Time/Time.h"
#include "00_Game/50_Input/Input.h"
#include "99_Utility/DirectXMath/DirectXMathExpansion.h"

namespace {
	constexpr float DEFAULT_DISTANCE    = 15.0f;
	constexpr float DEFAULT_ORBIT_SPEED = 2.0f;
	constexpr float PITCH_LIMIT_MIN     = -DirectX::XM_PIDIV2 * 0.05f;
	constexpr float PITCH_LIMIT_MAX     =  DirectX::XM_PIDIV2 * 0.6f;
	// マウス1ピクセルあたりの回転量(ラジアン). DebugCameraと共通の係数.
	constexpr float MOUSE_ROTATION_SPEED = 0.0035f;
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

void ThirdPersonCamera::OnActivated()
{
	// カーソルを隠し、毎フレーム中央に固定する
	// (ウィンドウ端・画面端でカーソルが止まって回転できなくなるのを防ぐため).
	Input::SetShowCursor(false);
	Input::SetCenterMouseCursor(true);
	Input::CenterMouseCursor();
}

void ThirdPersonCamera::OnDeactivated()
{
	// カーソル固定を解除して表示を戻す.
	Input::SetCenterMouseCursor(false);
	Input::SetShowCursor(true);
}

void ThirdPersonCamera::Update()
{
	const float delta_time = GameTime::GetDeltaTime();

	float yaw   = GetYaw();
	float pitch = GetPitch();

	// マウス移動量でターゲットを軸に周回する.
	const DirectX::XMFLOAT2 cursor_delta = Input::GetClientCursorDelta();
	yaw   += cursor_delta.x * MOUSE_ROTATION_SPEED;
	pitch += cursor_delta.y * MOUSE_ROTATION_SPEED;
	Input::CenterMouseCursor();

	// 矢印キーでも周回できる(デバッグ用の予備操作として共存).
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
