#pragma once

#include <vector>
#include <Windows.h>

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/09.
* @brief     : キー入力判定クラス, ServiceLocator経由で利用する.
**********************************************************************************/

class KeyInput final
{
public:
	KeyInput();
	~KeyInput();

	// 更新.
	static void Update();

	// 押下しているか.
	static bool IsKeyPress(const int& Key);
	static bool IsKeyPress(const std::vector<int>& KeyList);

	// 押下した瞬間.
	static bool IsKeyDown(const int& Key);
	static bool IsKeyDown(const std::vector<int>& KeyList);

	// 離した瞬間.
	static bool IsKeyUp(const int& Key);

	// 押下し続けているか.
	static bool IsKeyRepeat(const int& Key);
	static bool IsKeyRepeat(const std::vector<int>& KeyList);

private:
	static constexpr int KEY_MAX = 256; // キーの最大値.
private:
	BYTE m_NowKeyState[KEY_MAX]; // 現在の入力状態.
	BYTE m_OldKeyState[KEY_MAX]; // 1F前の入力状態.
};
