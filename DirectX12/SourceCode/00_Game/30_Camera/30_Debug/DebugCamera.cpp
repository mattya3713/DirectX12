#include "stdafx.h"
#include "DebugCamera.h"

#include <algorithm>

#include "00_Game/00_GameLoop/Time/Time.h"
#include "00_Game/50_Input/Input.h"

namespace {
	constexpr float DEFAULT_MOVE_SPEED = 20.0f;
	constexpr float SLOW_MOVE_SPEED    = 2.0f;
	constexpr float ROTATION_SPEED     = 0.0035f;
	constexpr float MAX_PITCH          = DirectX::XMConvertToRadians(89.0f);
	// ホイール1ノッチあたりの前後移動量(1フレームの経過時間に依存させない.
	// ノッチは離散的な入力であり、フレームレートで移動量が変わってしまうのは不自然なため).
	constexpr float ZOOM_STEP          = 1.0f;
}

DebugCamera::DebugCamera()
	: CameraBase{}
	, m_MoveSpeed		{ DEFAULT_MOVE_SPEED }
	, m_SlowMoveSpeed	{ SLOW_MOVE_SPEED }
	, m_IsRotating		{ false }
{
	// Player(原点)・Boss(Z=8)がコライダー基準の等身大スケールに補正された後の見え方に合わせた初期値
	// (旧DirectX12.cpp時代の値は、まだ大きすぎたモデルスケール基準だったため近すぎて見えなくなっていた).
	SetPosition({ 0.0f, 4.0f, -8.0f });
	SetLook({ 0.0f, 1.5f, 4.0f });
	SetYaw(0.0f);
	SetPitch(0.205f);
}

DebugCamera::~DebugCamera()
{
}

void DebugCamera::SetMoveSpeed(float Speed) noexcept
{
	m_MoveSpeed = Speed;
}

void DebugCamera::Update()
{
	const float delta_time = GameTime::GetDeltaTime();
	const DirectX::XMFLOAT2 cursor_delta = Input::GetClientCursorDelta();

	const bool is_rmb_down = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;

	if (is_rmb_down)
	{
		if (!m_IsRotating)
		{
			// 回転開始: カーソルを隠し、毎フレーム中央に固定し直す
			// (ウィンドウ端・画面端でカーソルが止まって回転できなくなるのを防ぐため).
			Input::SetShowCursor(false);
			Input::SetCenterMouseCursor(true);
		}
		else
		{
			// 開始フレーム(まだ中央固定前)の移動量は無視し、2フレーム目以降だけ反映する.
			SetYaw(GetYaw() + cursor_delta.x * ROTATION_SPEED);
			SetPitch(std::clamp(GetPitch() + cursor_delta.y * ROTATION_SPEED, -MAX_PITCH, MAX_PITCH));
		}

		Input::CenterMouseCursor();
		m_IsRotating = true;
	}
	else if (m_IsRotating)
	{
		// 回転終了: カーソル固定を解除して表示を戻す.
		Input::SetCenterMouseCursor(false);
		Input::SetShowCursor(true);
		m_IsRotating = false;
	}

	float speed = m_MoveSpeed;
	if (GetAsyncKeyState(VK_SHIFT) & 0x8000) { speed = m_SlowMoveSpeed; }

	DirectX::XMFLOAT3 position = GetPosition();
	const DirectX::XMFLOAT3 forward = GetForward();
	const DirectX::XMFLOAT3 right   = GetRight();

	DirectX::XMVECTOR v_position = DirectX::XMLoadFloat3(&position);
	const DirectX::XMVECTOR v_forward = DirectX::XMLoadFloat3(&forward);
	const DirectX::XMVECTOR v_right   = DirectX::XMLoadFloat3(&right);

	float move_forward = 0.0f;
	float move_right   = 0.0f;
	float move_up       = 0.0f;

	if (GetAsyncKeyState('W') & 0x8000) { move_forward += 1.0f; }
	if (GetAsyncKeyState('S') & 0x8000) { move_forward -= 1.0f; }
	if (GetAsyncKeyState('D') & 0x8000) { move_right   += 1.0f; }
	if (GetAsyncKeyState('A') & 0x8000) { move_right   -= 1.0f; }
	if (GetAsyncKeyState('Q') & 0x8000) { move_up       += 1.0f; } // Qキーで上に移動(ワールドY軸方向).
	if (GetAsyncKeyState('E') & 0x8000) { move_up       -= 1.0f; } // Eキーで下に移動(ワールドY軸方向).

	DirectX::XMVECTOR v_move = DirectX::XMVectorAdd(
		DirectX::XMVectorScale(v_forward, move_forward),
		DirectX::XMVectorScale(v_right, move_right));
	v_move = DirectX::XMVectorAdd(v_move, DirectX::XMVectorSet(0.0f, move_up, 0.0f, 0.0f));
	v_move = DirectX::XMVectorScale(v_move, speed * delta_time);

	const int wheel_direction = Input::GetWheelDirection();
	Input::SetWheelDirection(0);
	v_move = DirectX::XMVectorAdd(v_move, DirectX::XMVectorScale(v_forward, static_cast<float>(wheel_direction) * ZOOM_STEP));
	v_position = DirectX::XMVectorAdd(v_position, v_move);

	DirectX::XMStoreFloat3(&position, v_position);
	const DirectX::XMFLOAT3 updated_forward = GetForward();
	DirectX::XMVECTOR v_look = DirectX::XMVectorAdd(
		v_position,
		DirectX::XMLoadFloat3(&updated_forward));
	DirectX::XMFLOAT3 look = {};
	DirectX::XMStoreFloat3(&look, v_look);

	SetPosition(position);
	SetLook(look);

	UpdateViewProjection();
}
