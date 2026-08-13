#include "stdafx.h"
#include "FirstPersonCamera.h"

#include "00_Game/00_GameLoop/Time/Time.h"

namespace {
	constexpr float DEFAULT_MOVE_SPEED = 5.0f;
	constexpr float DEFAULT_LOOK_SPEED = 2.0f;
	constexpr float SPRINT_MULTIPLIER  = 2.0f;
	constexpr float PITCH_LIMIT        = DirectX::XM_PIDIV2 * 0.95f;
	constexpr float LOOK_DISTANCE      = 1.0f;
}

FirstPersonCamera::FirstPersonCamera()
	: CameraBase{}
	, m_MoveSpeed	{ DEFAULT_MOVE_SPEED }
	, m_LookSpeed	{ DEFAULT_LOOK_SPEED }
	, m_PitchLimit	{ PITCH_LIMIT }
{
}

FirstPersonCamera::~FirstPersonCamera()
{
}

void FirstPersonCamera::SetMoveSpeed(float Speed) noexcept
{
	m_MoveSpeed = Speed;
}

void FirstPersonCamera::SetLookSpeed(float Speed) noexcept
{
	m_LookSpeed = Speed;
}

void FirstPersonCamera::Update()
{
	const float delta_time = GameTime::GetDeltaTime();

	UpdateLook(delta_time);
	UpdateMove(delta_time);

	UpdateViewProjection();
}

void FirstPersonCamera::UpdateLook(float DeltaTime)
{
	float yaw   = GetYaw();
	float pitch = GetPitch();

	// マウスがまだ無いので、矢印キーで視点回転する.
	if (GetAsyncKeyState(VK_LEFT)  & 0x8000) { yaw   -= m_LookSpeed * DeltaTime; }
	if (GetAsyncKeyState(VK_RIGHT) & 0x8000) { yaw   += m_LookSpeed * DeltaTime; }
	if (GetAsyncKeyState(VK_UP)    & 0x8000) { pitch -= m_LookSpeed * DeltaTime; }
	if (GetAsyncKeyState(VK_DOWN)  & 0x8000) { pitch += m_LookSpeed * DeltaTime; }

	pitch = std::clamp(pitch, -m_PitchLimit, m_PitchLimit);

	SetYaw(yaw);
	SetPitch(pitch);
}

void FirstPersonCamera::UpdateMove(float DeltaTime)
{
	DirectX::XMFLOAT3 forward = GetForward();
	DirectX::XMFLOAT3 right   = GetRight();

	DirectX::XMVECTOR v_forward = DirectX::XMLoadFloat3(&forward);
	DirectX::XMVECTOR v_right   = DirectX::XMLoadFloat3(&right);

	// 水平移動のみ許可するため、Y成分を除去して正規化する.
	DirectX::XMVECTOR v_forward_horizontal = DirectX::XMVector3Normalize(DirectX::XMVectorSetY(v_forward, 0.0f));
	DirectX::XMVECTOR v_right_horizontal   = DirectX::XMVector3Normalize(DirectX::XMVectorSetY(v_right, 0.0f));

	float move_forward = 0.0f;
	float move_right   = 0.0f;
	if (GetAsyncKeyState('W') & 0x8000) { move_forward += 1.0f; }
	if (GetAsyncKeyState('S') & 0x8000) { move_forward -= 1.0f; }
	if (GetAsyncKeyState('D') & 0x8000) { move_right   += 1.0f; }
	if (GetAsyncKeyState('A') & 0x8000) { move_right   -= 1.0f; }

	float move_up = 0.0f;
	if (GetAsyncKeyState('E') & 0x8000) { move_up += 1.0f; }
	if (GetAsyncKeyState('Q') & 0x8000) { move_up -= 1.0f; }

	float speed = m_MoveSpeed;
	if (GetAsyncKeyState(VK_SHIFT) & 0x8000) { speed *= SPRINT_MULTIPLIER; }

	DirectX::XMVECTOR v_move = DirectX::XMVectorAdd(
		DirectX::XMVectorScale(v_forward_horizontal, move_forward),
		DirectX::XMVectorScale(v_right_horizontal, move_right));
	v_move = DirectX::XMVectorAdd(v_move, DirectX::XMVectorSet(0.0f, move_up, 0.0f, 0.0f));
	v_move = DirectX::XMVectorScale(v_move, speed * DeltaTime);

	const DirectX::XMFLOAT3& position = GetPosition();
	DirectX::XMVECTOR v_position = DirectX::XMVectorAdd(DirectX::XMLoadFloat3(&position), v_move);

	DirectX::XMFLOAT3 new_position = {};
	DirectX::XMStoreFloat3(&new_position, v_position);
	SetPosition(new_position);

	// 注視点はカメラ位置 + 視線方向(ピッチを反映した本来の前方ベクトル).
	DirectX::XMVECTOR v_look = DirectX::XMVectorAdd(v_position, DirectX::XMVectorScale(v_forward, LOOK_DISTANCE));

	DirectX::XMFLOAT3 look = {};
	DirectX::XMStoreFloat3(&look, v_look);
	SetLook(look);
}
