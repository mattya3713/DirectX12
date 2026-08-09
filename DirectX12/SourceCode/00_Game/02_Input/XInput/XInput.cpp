#include "XInput.h"
#include <algorithm>
#include <cassert>
#include <cmath>

namespace {
	static constexpr WORD KEY_TABLE[XInput::Key::Max] =
	{
		XINPUT_GAMEPAD_DPAD_UP,			// 方向パッド:上.
		XINPUT_GAMEPAD_DPAD_DOWN,		// 方向パッド:下.
		XINPUT_GAMEPAD_DPAD_LEFT,		// 方向パッド:左.
		XINPUT_GAMEPAD_DPAD_RIGHT,		// 方向パッド:右.
		XINPUT_GAMEPAD_START,			// ボタン:スタート.
		XINPUT_GAMEPAD_BACK,			// ボタン:バック.
		XINPUT_GAMEPAD_LEFT_THUMB,		// ボタン:左スティック.
		XINPUT_GAMEPAD_RIGHT_THUMB,		// ボタン:右スティック.
		XINPUT_GAMEPAD_LEFT_SHOULDER,	// ボタン:LB.
		XINPUT_GAMEPAD_RIGHT_SHOULDER,	// ボタン:RB.
		XINPUT_GAMEPAD_A,				// ボタン:A.
		XINPUT_GAMEPAD_B,				// ボタン:B.
		XINPUT_GAMEPAD_X,				// ボタン:X.
		XINPUT_GAMEPAD_Y,				// ボタン:Y.
	};

	static constexpr float DEADZONE = 7849.0f;
	static constexpr float MAX_VALUE = 32767.0f;
	static constexpr float INVERSE_RANGE = 1.0f / (MAX_VALUE - DEADZONE);
	static constexpr float STICK_THRESHOLD = 0.15f; // スティック入力ありと見なす閾値.
}

XInput::XInput(DWORD PadId)
	: m_PadID			{ PadId }
	, m_State			{}
	, m_PastState		{}
	, m_Vibration		{}
	, m_LStickState		{ StickState::None }
	, m_RStickState		{ StickState::None }
	, m_PastLStickState	{ StickState::None }
	, m_PastRStickState	{ StickState::None }
	, m_VibrationTime	{}
	, m_IsConnect		{ false }
	, m_IsVibration		{ false }
{
}

XInput::~XInput()
{
}

bool XInput::Update()
{
	m_PastState = m_State;
	m_PastLStickState = m_LStickState;
	m_PastRStickState = m_RStickState;

	// スティックの入力判定.
	IsStickInput(GetLStickDirection(), m_LStickState);
	IsStickInput(GetRStickDirection(), m_RStickState);

	EndProc();

	return UpdateStatus();
}

void XInput::EndProc()
{
	if (m_IsVibration) {
		m_VibrationTime++;
	}

	if (m_VibrationTime >= 30) {
		// 振動を停止.
		SetVibration(0, 0);
		m_IsVibration = false;
		m_VibrationTime = 0;
	}
}

bool XInput::IsDown(const Key TargetKey)
{
	WORD game_pad = GenerateGamePadValue(TargetKey);

	// 今回入力で前回未入力の場合、押した瞬間.
	return IsKeyCore(game_pad, m_State) && !IsKeyCore(game_pad, m_PastState);
}

bool XInput::IsUp(const Key TargetKey)
{
	WORD game_pad = GenerateGamePadValue(TargetKey);

	// 今回未入力で前回入力の場合、離した瞬間.
	return !IsKeyCore(game_pad, m_State) && IsKeyCore(game_pad, m_PastState);
}

bool XInput::IsRepeat(const Key TargetKey)
{
	WORD game_pad = GenerateGamePadValue(TargetKey);

	// 今回入力で前回入力の場合、押し続けている.
	return IsKeyCore(game_pad, m_State) && IsKeyCore(game_pad, m_PastState);
}

bool XInput::IsLStickDirectionDown(StickState Dir, const bool IsFirstPress)
{
	// 初回入力の判定.
	if (IsFirstPress)
	{
		return (m_LStickState == Dir) && (m_PastLStickState == StickState::None);
	}

	// 今回と前回の入力が違うなら入力した瞬間.
	return (m_LStickState == Dir) && (m_PastLStickState != Dir);
}

bool XInput::IsRStickDirectionDown(StickState Dir, const bool IsFirstPress)
{
	// 初回入力の判定.
	if (IsFirstPress)
	{
		return (m_RStickState == Dir) && (m_PastRStickState == StickState::None);
	}

	// 今回と前回の入力が違うなら入力した瞬間.
	return (m_RStickState == Dir) && (m_PastRStickState != Dir);
}

bool XInput::IsLStickDirectionUp()
{
	// 今回未入力、前回入力なら未入力になった瞬間.
	return (m_LStickState == StickState::None) && (m_PastLStickState != StickState::None);
}

bool XInput::IsRStickDirectionUp()
{
	// 今回未入力、前回入力なら未入力になった瞬間.
	return (m_RStickState == StickState::None) && (m_PastRStickState != StickState::None);
}

bool XInput::IsLStickDirectionRepeat(StickState Dir)
{
	// 今回と前回の入力が同じなら入力し続けている.
	return (m_LStickState == Dir) && (m_PastLStickState == Dir);
}

bool XInput::IsRStickDirectionRepeat(StickState Dir)
{
	// 今回と前回の入力が同じなら入力し続けている.
	return (m_RStickState == Dir) && (m_PastRStickState == Dir);
}

bool XInput::IsLStickActive(const float DeadZone)
{
	return IsOutsideDeadZone(DeadZone, GetLThumb_Clamp());
}

bool XInput::IsRStickActive(const float DeadZone)
{
	return IsOutsideDeadZone(DeadZone, GetRThumb_Clamp());
}

const BYTE XInput::GetLTriggerRaw() const
{
	return m_State.Gamepad.bLeftTrigger;
}

const BYTE XInput::GetRTriggerRaw() const
{
	return m_State.Gamepad.bRightTrigger;
}

const float XInput::GetLeftTrigger() const
{
	return static_cast<float>(m_State.Gamepad.bLeftTrigger) / 255.0f;
}

const float XInput::GetRightTrigger() const
{
	return static_cast<float>(m_State.Gamepad.bRightTrigger) / 255.0f;
}

const DirectX::XMFLOAT2 XInput::GetLStickDirection() const
{
	DirectX::XMFLOAT2 raw_thumb = GetLThumb();
	DirectX::XMFLOAT2 result = { 0.0f, 0.0f };

	// X軸の処理.
	float x = raw_thumb.x;
	if (std::abs(x) > DEADZONE)
	{
		if (x > 0)
		{
			// デッドゾーンを差し引き、[0, 32767-DEADZONE]を[0, 1.0]に正規化.
			result.x = (x - DEADZONE) * INVERSE_RANGE;
		}
		else // x < 0.
		{
			// マイナス方向のデッドゾーンを考慮し、[-1.0, 0]に正規化.
			result.x = (x + DEADZONE) * INVERSE_RANGE;
		}
	}
	// Y軸の処理.
	float y = raw_thumb.y;
	if (std::abs(y) > DEADZONE)
	{
		if (y > 0)
		{
			result.y = (y - DEADZONE) * INVERSE_RANGE;
		}
		else // y < 0.
		{
			result.y = (y + DEADZONE) * INVERSE_RANGE;
		}
	}

	return result;
}

const DirectX::XMFLOAT2 XInput::GetRStickDirection() const
{
	DirectX::XMFLOAT2 raw_thumb = GetRThumb();
	DirectX::XMFLOAT2 result = { 0.0f, 0.0f };

	// X軸の処理.
	float x = raw_thumb.x;
	if (std::abs(x) > DEADZONE)
	{
		if (x > 0)
		{
			result.x = (x - DEADZONE) * INVERSE_RANGE;
		}
		else // x < 0.
		{
			result.x = (x + DEADZONE) * INVERSE_RANGE;
		}
	}
	// Y軸の処理.
	float y = raw_thumb.y;
	if (std::abs(y) > DEADZONE)
	{
		if (y > 0)
		{
			result.y = (y - DEADZONE) * INVERSE_RANGE;
		}
		else // y < 0.
		{
			result.y = (y + DEADZONE) * INVERSE_RANGE;
		}
	}

	return result;
}

const DirectX::XMFLOAT2 XInput::GetLThumb() const
{
	return DirectX::XMFLOAT2(GetLThumbX(), GetLThumbY());
}

const float XInput::GetLThumbX() const
{
	return static_cast<float>(m_State.Gamepad.sThumbLX);
}

const float XInput::GetLThumbY() const
{
	return static_cast<float>(m_State.Gamepad.sThumbLY);
}

const DirectX::XMFLOAT2 XInput::GetRThumb_Clamp() const
{
	// 生の入力値を取得.
	float raw_x = static_cast<float>(m_State.Gamepad.sThumbRX);
	float raw_y = static_cast<float>(m_State.Gamepad.sThumbRY);
	DirectX::XMVECTOR v_input = DirectX::XMVectorSet(raw_x, raw_y, 0.0f, 0.0f);

	// ベクトルの長さを計算.
	DirectX::XMVECTOR v_magnitude = DirectX::XMVector2Length(v_input);
	float magnitude = 0.0f;
	DirectX::XMStoreFloat(&magnitude, v_magnitude);

	// 円形デッドゾーン処理.
	if (magnitude > DEADZONE)
	{
		// 正規化の実行.
		float normalized_magnitude = (magnitude - DEADZONE) / (THUMB_MAX - DEADZONE);

		// ベクトルを正規化し、正規化された長さを掛ける.
		DirectX::XMVECTOR v_normalized_dir = DirectX::XMVector2Normalize(v_input);
		DirectX::XMVECTOR v_clamped = DirectX::XMVectorScale(v_normalized_dir, normalized_magnitude);

		DirectX::XMFLOAT2 result;
		DirectX::XMStoreFloat2(&result, v_clamped);
		return result;
	}

	// デッドゾーン内の場合、(0.0, 0.0)を返す.
	return DirectX::XMFLOAT2(0.0f, 0.0f);
}

// X軸のクランプ値.
const float XInput::GetRThumbX_Clamp() const
{
	return GetRThumb_Clamp().x;
}

// Y軸のクランプ値.
const float XInput::GetRThumbY_Clamp() const
{
	return GetRThumb_Clamp().y;
}

const DirectX::XMFLOAT2 XInput::GetRThumb() const
{
	return DirectX::XMFLOAT2(GetRThumbX(), GetRThumbY());
}

const float XInput::GetRThumbX() const
{
	return static_cast<float>(m_State.Gamepad.sThumbRX);
}

const float XInput::GetRThumbY() const
{
	return static_cast<float>(m_State.Gamepad.sThumbRY);
}

const DirectX::XMFLOAT2 XInput::GetLThumb_Clamp() const
{
	// 生の入力値を取得.
	float raw_x = static_cast<float>(m_State.Gamepad.sThumbLX);
	float raw_y = static_cast<float>(m_State.Gamepad.sThumbLY);
	DirectX::XMVECTOR v_input = DirectX::XMVectorSet(raw_x, raw_y, 0.0f, 0.0f);

	// ベクトルの長さを計算.
	DirectX::XMVECTOR v_magnitude = DirectX::XMVector2Length(v_input);
	float magnitude = 0.0f;
	DirectX::XMStoreFloat(&magnitude, v_magnitude);

	// 円形デッドゾーン処理.
	if (magnitude > DEADZONE)
	{
		// 正規化の実行.
		float normalized_magnitude = (magnitude - DEADZONE) / (THUMB_MAX - DEADZONE);

		// ベクトルを正規化し、正規化された長さを掛ける.
		DirectX::XMVECTOR v_normalized_dir = DirectX::XMVector2Normalize(v_input);
		DirectX::XMVECTOR v_clamped = DirectX::XMVectorScale(v_normalized_dir, normalized_magnitude);

		DirectX::XMFLOAT2 result;
		DirectX::XMStoreFloat2(&result, v_clamped);
		return result;
	}

	// デッドゾーン内の場合、(0.0, 0.0)を返す.
	return DirectX::XMFLOAT2(0.0f, 0.0f);
}

// X軸のクランプ値.
const float XInput::GetLThumbX_Clamp() const
{
	return GetLThumb_Clamp().x;
}

// Y軸のクランプ値.
const float XInput::GetLThumbY_Clamp() const
{
	return GetLThumb_Clamp().y;
}

const DWORD XInput::GetPadID() const
{
	return m_PadID;
}

const bool XInput::IsConnect() const
{
	return m_IsConnect;
}

const bool XInput::SetVibration(const WORD& LeftMotorSpd, const WORD& RightMotorSpd)
{
	m_Vibration.wLeftMotorSpeed  = std::clamp(LeftMotorSpd, VIBRATION_MIN, VIBRATION_MAX);
	m_Vibration.wRightMotorSpeed = std::clamp(RightMotorSpd, VIBRATION_MIN, VIBRATION_MAX);

	// 振動設定があった場合フラグをtrueにする.
	if ((LeftMotorSpd != 0) || (RightMotorSpd != 0))
	{
		m_IsVibration = true;
	}

	return XInputSetState(m_PadID, &m_Vibration) == ERROR_SUCCESS;
}

bool XInput::UpdateStatus()
{
	m_IsConnect = false;

	if (XInputGetState(m_PadID, &m_State) == ERROR_SUCCESS)
	{
		m_IsConnect = true;
		return true;
	}
	return false;
}

bool XInput::IsKeyCore(WORD GamePad, const XINPUT_STATE& State)
{
	return (State.Gamepad.wButtons & GamePad) != 0;
}

WORD XInput::GenerateGamePadValue(const Key TargetKey)
{
	if (TargetKey < Key::First || Key::Last < TargetKey) {
		assert(0 && "キーの範囲外");
	}
	return KEY_TABLE[static_cast<int>(TargetKey)];
}

bool XInput::IsOutsideDeadZone(const float& DeadZone, const DirectX::XMFLOAT2& StickSlope)
{
	DirectX::XMVECTOR v_direction = DirectX::XMLoadFloat2(&StickSlope);
	DirectX::XMVECTOR v_length_sq = DirectX::XMVector2LengthSq(v_direction);

	float length_sq = 0.0f;
	DirectX::XMStoreFloat(&length_sq, v_length_sq);

	return length_sq > DeadZone;
}

bool XInput::IsStickInput(const DirectX::XMFLOAT2& UseStickDir, StickState& OutState)
{
	const DirectX::XMVECTOR v_stick_direction = DirectX::XMLoadFloat2(&UseStickDir);

	// デッドゾーン判定.
	const float length_sq = DirectX::XMVectorGetX(DirectX::XMVector2LengthSq(v_stick_direction));
	if (length_sq < STICK_THRESHOLD)
	{
		OutState = StickState::None;
		return false;
	}

	const DirectX::XMVECTOR v_right = DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
	const DirectX::XMVECTOR v_up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	const DirectX::XMVECTOR v_left = DirectX::XMVectorSet(-1.0f, 0.0f, 0.0f, 0.0f);
	const DirectX::XMVECTOR v_down = DirectX::XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f);

	// 角度のしきい値(cos 45°).
	constexpr float THRESHOLD = 0.707f;

	const float dot_right = DirectX::XMVectorGetX(DirectX::XMVector2Dot(v_stick_direction, v_right));
	const float dot_up = DirectX::XMVectorGetX(DirectX::XMVector2Dot(v_stick_direction, v_up));
	const float dot_left = DirectX::XMVectorGetX(DirectX::XMVector2Dot(v_stick_direction, v_left));
	const float dot_down = DirectX::XMVectorGetX(DirectX::XMVector2Dot(v_stick_direction, v_down));

	// 入力方向を決定.
	if (dot_right > THRESHOLD) {
		OutState = StickState::Right;
	}
	else if (dot_left > THRESHOLD) {
		OutState = StickState::Left;
	}
	else if (dot_up > THRESHOLD) {
		OutState = StickState::Up;
	}
	else if (dot_down > THRESHOLD) {
		OutState = StickState::Down;
	}
	else {
		return false;
	}

	return true;
}
