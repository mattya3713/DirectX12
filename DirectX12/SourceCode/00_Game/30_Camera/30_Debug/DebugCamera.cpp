#include "stdafx.h"
#include "DebugCamera.h"

#include "00_Game/00_GameLoop/Time/Time.h"

namespace {
	constexpr float DEFAULT_MOVE_SPEED = 20.0f;
	constexpr float SLOW_MOVE_SPEED    = 2.0f;
}

DebugCamera::DebugCamera()
	: CameraBase{}
	, m_MoveSpeed		{ DEFAULT_MOVE_SPEED }
	, m_SlowMoveSpeed	{ SLOW_MOVE_SPEED }
{
	// 元々DirectX12.cpp側に直書きされていたデバッグカメラの初期値を踏襲.
	SetPosition({ 0.0f, 20.0f, -40.0f });
	SetLook({ 0.0f, 15.0f, 0.0f });
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

	float speed = m_MoveSpeed;
	if (GetAsyncKeyState(VK_SHIFT) & 0x8000) { speed = m_SlowMoveSpeed; }

	DirectX::XMFLOAT3 position = GetPosition();
	DirectX::XMFLOAT3 look     = GetLook();

	DirectX::XMVECTOR v_eye    = DirectX::XMLoadFloat3(&position);
	DirectX::XMVECTOR v_target = DirectX::XMLoadFloat3(&look);
	DirectX::XMVECTOR v_up     = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	// 現在のビュー行列の逆行列から、カメラのローカル軸(前・右)を求める.
	DirectX::XMMATRIX view_matrix     = DirectX::XMMatrixLookAtLH(v_eye, v_target, v_up);
	DirectX::XMMATRIX inv_view_matrix = DirectX::XMMatrixInverse(nullptr, view_matrix);

	DirectX::XMVECTOR v_forward = DirectX::XMVector3Normalize(inv_view_matrix.r[2]);
	DirectX::XMVECTOR v_right   = DirectX::XMVector3Normalize(inv_view_matrix.r[0]);

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

	v_eye    = DirectX::XMVectorAdd(v_eye, v_move);
	v_target = DirectX::XMVectorAdd(v_target, v_move);

	DirectX::XMStoreFloat3(&position, v_eye);
	DirectX::XMStoreFloat3(&look, v_target);

	SetPosition(position);
	SetLook(look);

	UpdateViewProjection();
}
