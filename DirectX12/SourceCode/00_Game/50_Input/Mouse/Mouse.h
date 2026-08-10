#pragma once

#include <Windows.h>
#include <DirectXMath.h>

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/09.
* @brief     : マウス入力クラス, ServiceLocator経由で利用する.
**********************************************************************************/

class Mouse final
{
public:
	Mouse();
	~Mouse();

	// 更新.
	static void Update();

	// マウスカーソルをウィンドウの中心に固定する.
	static void CenterMouseCursor();

	// マウスカーソルがスクリーン内に収まるようにする.
	static void WrapCursorInScreen();

	// マウスカーソルがウィンドウ内にあるか判定.
	static bool IsCursorInWindow();

	// マウスカーソルが指定領域にあるか判定.
	static bool IsCursorInRegion(const DirectX::XMFLOAT2& Position, const DirectX::XMFLOAT2& Size);

public: // Getter、Setter.

	// ウィンドウハンドルを設定.
	static void SethWnd(HWND hWnd);

	// 現在のカーソル座標を取得.
	// GetCursorPosition       : ディスプレイ基準.
	// GetClientCursorPosition : ウィンドウ基準.
	static DirectX::XMFLOAT2 GetCursorPosition();
	static DirectX::XMFLOAT2 GetClientCursorPosition();

	// 前回のカーソル座標を取得.
	static DirectX::XMFLOAT2 GetPastCursorPosition();
	static DirectX::XMFLOAT2 GetPastClientCursorPosition();

	// 前フレームから今フレームへの移動量を取得.
	static DirectX::XMFLOAT2 GetClientCursorDelta();

	// マウスホイールの操作方向を取得・設定.
	static int& GetWheelDirection();
	static void SetWheelDirection(const int& Direction);

	// カーソルをウィンドウの中心に固定するか判定・設定.
	static bool& IsCenterMouseCursor();
	static void SetCenterMouseCursor(const bool& IsCenter);

	// マウスの掴み状態を判定・設定.
	static bool& IsMouseGrab();
	static void SetMouseGrab(const bool& IsGrab);

	// カーソルの表示を設定.
	static void SetShowCursor(const bool& IsShowCursor);

private:
	HWND	m_hWnd;
	POINT	m_NowMousePoint;		// 現在のマウス座標.
	POINT	m_PastMousePoint;		// 過去のマウス座標.
	POINT	m_NowClientMousePoint;	// 現在のマウス座標(クライアント座標).
	POINT	m_PastClientMousePoint;	// 過去のマウス座標(クライアント座標).
	DirectX::XMFLOAT2 m_ClientCursorDelta;	// マウスの移動量.
	int		m_WheelDirection;		// マウスホイールを動かした方向.
	bool	m_IsGrab;				// マウスの掴み状態.
	bool	m_IsCenterMouseCursor;	// マウスカーソルを固定するか.
	bool	m_IsShowCursor;			// カーソルを表示するか.
};
