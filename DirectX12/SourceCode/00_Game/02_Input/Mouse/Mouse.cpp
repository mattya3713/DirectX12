#include "Mouse.h"
#include "99_Utility/ServiceLocator/ServiceLocator.h"

Mouse::Mouse()
	: m_hWnd				{}
	, m_NowMousePoint       {}
	, m_PastMousePoint      {}
	, m_NowClientMousePoint	{}
	, m_PastClientMousePoint{}
	, m_ClientCursorDelta	{}
	, m_WheelDirection		{ 0 }
	, m_IsGrab              { false }
	, m_IsCenterMouseCursor { false }
	, m_IsShowCursor		{ true }
{
}

Mouse::~Mouse()
{
}

void Mouse::Update()
{
	Mouse* p_instance = ServiceLocator::Get<Mouse>();

	// 過去の座標を保存.
	p_instance->m_PastMousePoint       = p_instance->m_NowMousePoint;
	p_instance->m_PastClientMousePoint = p_instance->m_NowClientMousePoint;

	// マウス座標を取得.
	GetCursorPos(&p_instance->m_NowMousePoint);

	// マウス座標をクライアント座標に変換.
	p_instance->m_NowClientMousePoint = p_instance->m_NowMousePoint;
	ScreenToClient(p_instance->m_hWnd, &p_instance->m_NowClientMousePoint);

	// 移動距離を調べる.
	p_instance->m_ClientCursorDelta.x = static_cast<float>(p_instance->m_NowClientMousePoint.x - p_instance->m_PastClientMousePoint.x);
	p_instance->m_ClientCursorDelta.y = static_cast<float>(p_instance->m_NowClientMousePoint.y - p_instance->m_PastClientMousePoint.y);
}

// マウスカーソルをウィンドウの中心に固定する.
void Mouse::CenterMouseCursor()
{
	Mouse* p_instance = ServiceLocator::Get<Mouse>();
	if (p_instance->m_IsCenterMouseCursor == false) { return; }

	// クライアント領域の位置とサイズを取得.
	RECT rect;
	GetClientRect(p_instance->m_hWnd, &rect);

	// ウィンドウの中心を計算.
	int center_x = (rect.right - rect.left) / 2;
	int center_y = (rect.bottom - rect.top) / 2;

	// クライアント座標をスクリーン座標に変換.
	POINT center_point = { center_x, center_y };
	ClientToScreen(p_instance->m_hWnd, &center_point);

	// マウス座標をウィンドウの中心に固定.
	SetCursorPos(center_point.x, center_point.y);

	// 中心の位置をNowClientMousePointとして更新.
	p_instance->m_NowClientMousePoint.x = center_x;
	p_instance->m_NowClientMousePoint.y = center_y;
}

// マウスカーソルがスクリーン内に収まるようにする.
void Mouse::WrapCursorInScreen()
{
	Mouse* p_instance = ServiceLocator::Get<Mouse>();

	// 画面の解像度を取得.
	int screen_width = GetSystemMetrics(SM_CXSCREEN);
	int screen_height = GetSystemMetrics(SM_CYSCREEN);

	int new_cursor_pos_x = p_instance->m_NowMousePoint.x;
	int new_cursor_pos_y = p_instance->m_NowMousePoint.y;

	if (new_cursor_pos_x <= 0) {
		new_cursor_pos_x = screen_width - 1;
	}
	else if (new_cursor_pos_x >= screen_width - 1) {
		new_cursor_pos_x = 0;
	}

	if (new_cursor_pos_y <= 0) {
		new_cursor_pos_y = screen_height - 1;
	}
	else if (new_cursor_pos_y >= screen_height - 1) {
		new_cursor_pos_y = 0;
	}

	SetCursorPos(new_cursor_pos_x, new_cursor_pos_y);
}

// マウスカーソルがウィンドウ内にあるか判定.
bool Mouse::IsCursorInWindow()
{
	Mouse* p_instance = ServiceLocator::Get<Mouse>();

	// クライアント領域の位置とサイズを取得.
	RECT rect;
	GetClientRect(p_instance->m_hWnd, &rect);

	bool is_right_of_left = p_instance->m_NowMousePoint.x >= rect.left;	// x座標が左端より右にあるか.
	bool is_left_of_right = p_instance->m_NowMousePoint.x <= rect.right;	// x座標が右端より左にあるか.

	bool is_bottom_of_top = p_instance->m_NowMousePoint.y >= rect.top;	// y座標が上端より下にあるか.
	bool is_top_of_bottom = p_instance->m_NowMousePoint.y <= rect.bottom;	// y座標が下端より上にあるか.

	// X軸とY軸の境界を判定.
	bool is_within_x = is_right_of_left && is_left_of_right;
	bool is_within_y = is_bottom_of_top && is_top_of_bottom;

	// クライアント領域内か判定.
	return is_within_x && is_within_y;
}

// マウスカーソルが指定領域にあるか判定.
bool Mouse::IsCursorInRegion(const DirectX::XMFLOAT2& Position, const DirectX::XMFLOAT2& Size)
{
	Mouse* p_instance = ServiceLocator::Get<Mouse>();

	return p_instance->m_NowClientMousePoint.x >= Position.x && p_instance->m_NowClientMousePoint.x <= Position.x + Size.x &&
		p_instance->m_NowClientMousePoint.y >= Position.y && p_instance->m_NowClientMousePoint.y <= Position.y + Size.y;
}

void Mouse::SethWnd(HWND hWnd)
{
	ServiceLocator::Get<Mouse>()->m_hWnd = hWnd;
}

DirectX::XMFLOAT2 Mouse::GetCursorPosition()
{
	Mouse* p_instance = ServiceLocator::Get<Mouse>();
	float x = static_cast<float>(p_instance->m_NowMousePoint.x);
	float y = static_cast<float>(p_instance->m_NowMousePoint.y);

	return DirectX::XMFLOAT2(x, y);
}

DirectX::XMFLOAT2 Mouse::GetClientCursorPosition()
{
	Mouse* p_instance = ServiceLocator::Get<Mouse>();
	float x = static_cast<float>(p_instance->m_NowClientMousePoint.x);
	float y = static_cast<float>(p_instance->m_NowClientMousePoint.y);

	return DirectX::XMFLOAT2(x, y);
}

DirectX::XMFLOAT2 Mouse::GetPastCursorPosition()
{
	Mouse* p_instance = ServiceLocator::Get<Mouse>();
	float x = static_cast<float>(p_instance->m_PastMousePoint.x);
	float y = static_cast<float>(p_instance->m_PastMousePoint.y);

	return DirectX::XMFLOAT2(x, y);
}

DirectX::XMFLOAT2 Mouse::GetPastClientCursorPosition()
{
	Mouse* p_instance = ServiceLocator::Get<Mouse>();
	float x = static_cast<float>(p_instance->m_PastClientMousePoint.x);
	float y = static_cast<float>(p_instance->m_PastClientMousePoint.y);

	return DirectX::XMFLOAT2(x, y);
}

DirectX::XMFLOAT2 Mouse::GetClientCursorDelta()
{
	Mouse* p_instance = ServiceLocator::Get<Mouse>();
	float x = static_cast<float>(p_instance->m_ClientCursorDelta.x);
	float y = static_cast<float>(p_instance->m_ClientCursorDelta.y);

	return DirectX::XMFLOAT2(x, y);
}

int& Mouse::GetWheelDirection()
{
	return ServiceLocator::Get<Mouse>()->m_WheelDirection;
}

void Mouse::SetWheelDirection(const int& Direction)
{
	ServiceLocator::Get<Mouse>()->m_WheelDirection = Direction;
}

bool& Mouse::IsCenterMouseCursor()
{
	return ServiceLocator::Get<Mouse>()->m_IsCenterMouseCursor;
}

void Mouse::SetCenterMouseCursor(const bool& IsCenter)
{
	ServiceLocator::Get<Mouse>()->m_IsCenterMouseCursor = IsCenter;
}

bool& Mouse::IsMouseGrab()
{
	return ServiceLocator::Get<Mouse>()->m_IsGrab;
}

void Mouse::SetMouseGrab(const bool& IsGrab)
{
	ServiceLocator::Get<Mouse>()->m_IsGrab = IsGrab;
}

void Mouse::SetShowCursor(const bool& IsShowCursor)
{
	Mouse* p_instance = ServiceLocator::Get<Mouse>();

	if (p_instance->m_IsShowCursor == IsShowCursor) { return; }
	p_instance->m_IsShowCursor = IsShowCursor;
	ShowCursor(IsShowCursor);
}
