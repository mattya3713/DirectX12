#pragma once

#include <array>
#include <memory>
#include <vector>
#include <Windows.h>
#include <DirectXMath.h>

#include "00_Game/50_Input/XInput/XInput.h"

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/09.
* @brief     : 入力機のラッパークラス, ServiceLocator経由で利用する.
*            : キーボード/マウスはKeyInput/Mouse(いずれも別途ServiceLocator登録)へ委譲し、
*            : コントローラー(最大4台)はこのクラスが直接所有する.
**********************************************************************************/

class Input final
{
public:
	Input();
	~Input();

	static void Update();

	// ウィンドウハンドルを設定.
	static void SethWnd(HWND hWnd);

public: // キーボード.
	static bool IsKeyPress(const int& Key);
	static bool IsKeyPress(const std::vector<int>& KeyList);
	static bool IsKeyDown(const int& Key);
	static bool IsKeyDown(const std::vector<int>& KeyList);
	static bool IsKeyUp(const int& Key);
	static bool IsKeyRepeat(const int& Key);
	static bool IsKeyRepeat(const std::vector<int>& KeyList);

public: // マウス.
	static void CenterMouseCursor();
	static void WrapCursorInScreen();
	static const bool IsCursorInWindow();
	static const bool IsCursorInRegion(const DirectX::XMFLOAT2& Position, const DirectX::XMFLOAT2& Size);

	// GetCursorPosition       : ディスプレイ基準.
	// GetClientCursorPosition : ウィンドウ基準.
	static const DirectX::XMFLOAT2 GetCursorPosition();
	static const DirectX::XMFLOAT2 GetClientCursorPosition();
	static const DirectX::XMFLOAT2 GetPastCursorPosition();
	static const DirectX::XMFLOAT2 GetPastClientCursorPosition();
	static const DirectX::XMFLOAT2 GetClientCursorDelta();

	static const int GetWheelDirection();
	static void SetWheelDirection(const int Direction);

	static const bool IsCenterMouseCursor();
	static void SetCenterMouseCursor(const bool IsCenter);

	static const bool IsMouseGrab();
	static void SetMouseGrab(const bool IsGrab);

	static void SetShowCursor(const bool& IsShowCursor);

public: // コントローラー.

	// ボタンを押下した瞬間か判定(Idはコントローラー番号0～3).
	static const bool IsButtonDown(const XInput::Key Key, const int Id = 0);
	// ボタンを離した瞬間か判定.
	static const bool IsButtonUp(const XInput::Key Key, const int Id = 0);
	// ボタンを押下し続けているか判定.
	static const bool IsButtonRepeat(const XInput::Key Key, const int Id = 0);

	// 指定方向にスティックが入力された瞬間か判定.
	static const bool IsLStickDirectionDown(const XInput::StickState Dir, const bool IsFirstPress = false, const int Id = 0);
	static const bool IsRStickDirectionDown(const XInput::StickState Dir, const bool IsFirstPress = false, const int Id = 0);

	// 指定方向にスティックが未入力になった瞬間か判定.
	static const bool IsLStickDirectionUp(const int Id = 0);
	static const bool IsRStickDirectionUp(const int Id = 0);

	// 指定方向にスティックが入力し続けているか判定.
	static const bool IsLStickDirectionRepeat(const XInput::StickState Dir, const int Id = 0);
	static const bool IsRStickDirectionRepeat(const XInput::StickState Dir, const int Id = 0);

	// スティック入力があるか判定.
	static const bool IsLStickActive(const float DeadZone = 0.0f, const int Id = 0);
	static const bool IsRStickActive(const float DeadZone = 0.0f, const int Id = 0);

	// コントローラーを取得.
	static const std::shared_ptr<XInput> GetController(const int Id = 0);

	// スティックの入力方向を取得.
	static const DirectX::XMFLOAT2 GetLStickDirection(const int Id = 0);
	static const DirectX::XMFLOAT2 GetRStickDirection(const int Id = 0);

	// トリガーの入力値を取得.
	static const float GetLTriggerRaw(const int Id = 0);
	static const float GetRTriggerRaw(const int Id = 0);
	static const float GetLTrigger(const int Id = 0);
	static const float GetRTrigger(const int Id = 0);

private:
	// 全てのコントローラーの更新.
	void UpdateForAllController();
	// コントローラーの作成.
	void CreateController();

public: // 定数.
	static constexpr int CONTROLLER_MAX = 4; // コントローラーの最大数.

private:
	HWND m_hWnd;
	std::array<std::shared_ptr<XInput>, CONTROLLER_MAX> m_spControllers;
};
