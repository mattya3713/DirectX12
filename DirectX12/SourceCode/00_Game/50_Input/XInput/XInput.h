#pragma once

#include <Xinput.h>
#include <DirectXMath.h>

#pragma comment(lib, "xinput.lib")

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/09.
* @brief     : Xboxコントローラー1台分の入力を扱うクラス.
**********************************************************************************/

class XInput final
{
public:

	enum Key
	{
		None = -1,
		Up,		// 方向パッド:上.
		Down,	// 方向パッド:下.
		Left,	// 方向パッド:左.
		Right,	// 方向パッド:右.
		Start,	// ボタン:スタート.
		Back,	// ボタン:バック.
		LStick,	// ボタン:左スティック.
		RStick,	// ボタン:右スティック.
		LB,		// ボタン:LB.
		RB,		// ボタン:RB.
		A,		// ボタン:A.
		B,		// ボタン:B.
		X,		// ボタン:X.
		Y,		// ボタン:Y.

		Max,
		First = Up,
		Last = Y,
	};

	enum class StickState
	{
		None = -1,	// 未入力.
		Up = 0,		// 上.
		Down,		// 下.
		Left,		// 左.
		Right,		// 右.

		Max,
	};

public:
	XInput(DWORD PadId);
	~XInput();

	bool Update();
	void EndProc();

	// ボタンを押下した瞬間か判定.
	bool IsDown(const Key TargetKey);
	// ボタンを離した瞬間か判定.
	bool IsUp(const Key TargetKey);
	// ボタンを押下し続けているか判定.
	bool IsRepeat(const Key TargetKey);

	// 指定方向にスティックが入力された瞬間か判定.
	bool IsLStickDirectionDown(StickState Dir, const bool IsFirstPress = false);
	bool IsRStickDirectionDown(StickState Dir, const bool IsFirstPress = false);

	// 指定方向にスティックが未入力になった瞬間か判定.
	bool IsLStickDirectionUp();
	bool IsRStickDirectionUp();

	// 指定方向にスティックが入力し続けているか判定.
	bool IsLStickDirectionRepeat(StickState Dir);
	bool IsRStickDirectionRepeat(StickState Dir);

	// スティック入力があるか判定.
	bool IsLStickActive(const float DeadZone = 0.0f);
	bool IsRStickActive(const float DeadZone = 0.0f);

public: // Getter、Setter.

	// トリガーの生の値(0～255)を取得.
	const BYTE GetLTriggerRaw() const noexcept;
	const BYTE GetRTriggerRaw() const noexcept;

	// トリガーの正規化された値(0.0f～1.0f)を取得.
	const float GetLeftTrigger() const noexcept;
	const float GetRightTrigger() const noexcept;

	// スティックの入力方向を取得(デッドゾーン考慮・正規化済み).
	const DirectX::XMFLOAT2 GetLStickDirection() const noexcept;
	const DirectX::XMFLOAT2 GetRStickDirection() const noexcept;

	// 左スティックの生入力値を取得.
	const DirectX::XMFLOAT2 GetLThumb() const noexcept;
	const float GetLThumbX() const noexcept;
	const float GetLThumbY() const noexcept;

	// 左スティックの-1～1にクランプされた入力値を取得.
	const DirectX::XMFLOAT2 GetLThumb_Clamp() const noexcept;
	const float GetLThumbX_Clamp() const noexcept;
	const float GetLThumbY_Clamp() const noexcept;

	// 右スティックの生入力値を取得.
	const DirectX::XMFLOAT2 GetRThumb() const noexcept;
	const float GetRThumbX() const noexcept;
	const float GetRThumbY() const noexcept;

	// 右スティックの-1～1にクランプされた入力値を取得.
	const DirectX::XMFLOAT2 GetRThumb_Clamp() const noexcept;
	const float GetRThumbX_Clamp() const noexcept;
	const float GetRThumbY_Clamp() const noexcept;

	// パッド番号を取得.
	const DWORD GetPadID() const noexcept;

	// 接続状態を取得.
	const bool IsConnect() const noexcept;

	// 振動を設定.
	const bool SetVibration(const WORD& LeftMotorSpd, const WORD& RightMotorSpd);

private:
	// 入力状態の更新.
	bool UpdateStatus();
	// 入力判定.
	bool IsKeyCore(WORD GamePad, const XINPUT_STATE& State);
	// ボタン入力をWORD型に変換.
	WORD GenerateGamePadValue(const Key TargetKey);
	// スティック入力がデッドゾーンを超えているか判定.
	bool IsOutsideDeadZone(const float& DeadZone, const DirectX::XMFLOAT2& StickSlope);
	// スティック入力の方向を判定.
	bool IsStickInput(const DirectX::XMFLOAT2& UseStickDir, StickState& OutState);

public: // 定数.

	// トリガー入力範囲.
	static constexpr BYTE TRIGGER_MIN = 0;
	static constexpr BYTE TRIGGER_MAX = 255;

	// スティック入力範囲.
	static constexpr SHORT THUMB_MIN = -32768;
	static constexpr SHORT THUMB_MAX = 32767;

	// 振動範囲.
	static constexpr WORD VIBRATION_MIN = 0;
	static constexpr WORD VIBRATION_MAX = 65535;

private:
	DWORD				m_PadID;			// パッド番号(0～3).
	XINPUT_STATE        m_State;			// キー入力情報.
	XINPUT_STATE		m_PastState;		// キー入力情報(キーストローク判定用).
	XINPUT_VIBRATION    m_Vibration;		// 振動.
	StickState			m_LStickState;		// 左スティック入力情報.
	StickState			m_RStickState;		// 右スティック入力情報.
	StickState			m_PastLStickState;	// 前回の左スティック入力情報.
	StickState			m_PastRStickState;	// 前回の右スティック入力情報.
	int					m_VibrationTime;	// 振動処理の時間計測用.
	bool				m_IsConnect;		// 接続判定.
	bool				m_IsVibration;		// 振動があったか.
};
