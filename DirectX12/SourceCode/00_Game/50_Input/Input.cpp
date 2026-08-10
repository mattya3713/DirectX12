#include "Input.h"
#include "00_Game/50_Input/KeyInput/KeyInput.h"
#include "00_Game/50_Input/Mouse/Mouse.h"
#include "99_Utility/ServiceLocator/ServiceLocator.h"
#include <cassert>

Input::Input()
	: m_hWnd			{}
	, m_spControllers	{}
{
	// コントローラーの作成.
	CreateController();
}

Input::~Input()
{
}

void Input::Update()
{
	// キーボードの更新.
	KeyInput::Update();

	// マウスの更新.
	Mouse::Update();

	// コントローラーの更新.
	ServiceLocator::Get<Input>()->UpdateForAllController();
}

void Input::SethWnd(HWND hWnd)
{
	ServiceLocator::Get<Input>()->m_hWnd = hWnd;
	Mouse::SethWnd(hWnd);
}

bool Input::IsKeyPress(const int& Key)
{
	return KeyInput::IsKeyPress(Key);
}

bool Input::IsKeyPress(const std::vector<int>& KeyList)
{
	return KeyInput::IsKeyPress(KeyList);
}

bool Input::IsKeyDown(const int& Key)
{
	return KeyInput::IsKeyDown(Key);
}

bool Input::IsKeyDown(const std::vector<int>& KeyList)
{
	return KeyInput::IsKeyDown(KeyList);
}

bool Input::IsKeyUp(const int& Key)
{
	return KeyInput::IsKeyUp(Key);
}

bool Input::IsKeyRepeat(const int& Key)
{
	return KeyInput::IsKeyRepeat(Key);
}

bool Input::IsKeyRepeat(const std::vector<int>& KeyList)
{
	return KeyInput::IsKeyRepeat(KeyList);
}

void Input::CenterMouseCursor()
{
	Mouse::CenterMouseCursor();
}

void Input::WrapCursorInScreen()
{
	Mouse::WrapCursorInScreen();
}

const bool Input::IsCursorInWindow()
{
	return Mouse::IsCursorInWindow();
}

const bool Input::IsCursorInRegion(const DirectX::XMFLOAT2& Position, const DirectX::XMFLOAT2& Size)
{
	return Mouse::IsCursorInRegion(Position, Size);
}

const DirectX::XMFLOAT2 Input::GetCursorPosition()
{
	return Mouse::GetCursorPosition();
}

const DirectX::XMFLOAT2 Input::GetClientCursorPosition()
{
	return Mouse::GetClientCursorPosition();
}

const DirectX::XMFLOAT2 Input::GetPastCursorPosition()
{
	return Mouse::GetPastCursorPosition();
}

const DirectX::XMFLOAT2 Input::GetPastClientCursorPosition()
{
	return Mouse::GetPastClientCursorPosition();
}

const DirectX::XMFLOAT2 Input::GetClientCursorDelta()
{
	return Mouse::GetClientCursorDelta();
}

const int Input::GetWheelDirection()
{
	return Mouse::GetWheelDirection();
}

void Input::SetWheelDirection(const int Direction)
{
	Mouse::SetWheelDirection(Direction);
}

const bool Input::IsCenterMouseCursor()
{
	return Mouse::IsCenterMouseCursor();
}

void Input::SetCenterMouseCursor(const bool IsCenter)
{
	Mouse::SetCenterMouseCursor(IsCenter);
}

const bool Input::IsMouseGrab()
{
	return Mouse::IsMouseGrab();
}

void Input::SetMouseGrab(const bool IsGrab)
{
	Mouse::SetMouseGrab(IsGrab);
}

void Input::SetShowCursor(const bool& IsShowCursor)
{
	Mouse::SetShowCursor(IsShowCursor);
}

const bool Input::IsButtonDown(const XInput::Key Key, const int Id)
{
	return ServiceLocator::Get<Input>()->m_spControllers[Id]->IsDown(Key);
}

const bool Input::IsButtonUp(const XInput::Key Key, const int Id)
{
	return ServiceLocator::Get<Input>()->m_spControllers[Id]->IsUp(Key);
}

const bool Input::IsButtonRepeat(const XInput::Key Key, const int Id)
{
	return ServiceLocator::Get<Input>()->m_spControllers[Id]->IsRepeat(Key);
}

const bool Input::IsLStickDirectionDown(const XInput::StickState Dir, const bool IsFirstPress, const int Id)
{
	return ServiceLocator::Get<Input>()->m_spControllers[Id]->IsLStickDirectionDown(Dir, IsFirstPress);
}

const bool Input::IsRStickDirectionDown(const XInput::StickState Dir, const bool IsFirstPress, const int Id)
{
	return ServiceLocator::Get<Input>()->m_spControllers[Id]->IsRStickDirectionDown(Dir, IsFirstPress);
}

const bool Input::IsLStickDirectionUp(const int Id)
{
	return ServiceLocator::Get<Input>()->m_spControllers[Id]->IsLStickDirectionUp();
}

const bool Input::IsRStickDirectionUp(const int Id)
{
	return ServiceLocator::Get<Input>()->m_spControllers[Id]->IsRStickDirectionUp();
}

const bool Input::IsLStickDirectionRepeat(const XInput::StickState Dir, const int Id)
{
	return ServiceLocator::Get<Input>()->m_spControllers[Id]->IsLStickDirectionRepeat(Dir);
}

const bool Input::IsRStickDirectionRepeat(const XInput::StickState Dir, const int Id)
{
	return ServiceLocator::Get<Input>()->m_spControllers[Id]->IsRStickDirectionRepeat(Dir);
}

const bool Input::IsLStickActive(const float DeadZone, const int Id)
{
	return ServiceLocator::Get<Input>()->m_spControllers[Id]->IsLStickActive(DeadZone);
}

const bool Input::IsRStickActive(const float DeadZone, const int Id)
{
	return ServiceLocator::Get<Input>()->m_spControllers[Id]->IsRStickActive(DeadZone);
}

const std::shared_ptr<XInput> Input::GetController(const int Id)
{
	if (Id >= CONTROLLER_MAX)
	{
		assert(0 && "参照出来ないコントローラーの番号です");
	}

	return ServiceLocator::Get<Input>()->m_spControllers[Id];
}

const DirectX::XMFLOAT2 Input::GetLStickDirection(const int Id)
{
	return ServiceLocator::Get<Input>()->m_spControllers[Id]->GetLStickDirection();
}

const DirectX::XMFLOAT2 Input::GetRStickDirection(const int Id)
{
	return ServiceLocator::Get<Input>()->m_spControllers[Id]->GetRStickDirection();
}

const float Input::GetLTriggerRaw(const int Id)
{
	return ServiceLocator::Get<Input>()->m_spControllers[Id]->GetLTriggerRaw();
}

const float Input::GetRTriggerRaw(const int Id)
{
	return ServiceLocator::Get<Input>()->m_spControllers[Id]->GetRTriggerRaw();
}

const float Input::GetLTrigger(const int Id)
{
	return ServiceLocator::Get<Input>()->m_spControllers[Id]->GetLeftTrigger();
}

const float Input::GetRTrigger(const int Id)
{
	return ServiceLocator::Get<Input>()->m_spControllers[Id]->GetRightTrigger();
}

void Input::UpdateForAllController()
{
	for (const auto& controller : m_spControllers)
	{
		controller->Update();
	}
}

void Input::CreateController()
{
	for (int i = 0; i < CONTROLLER_MAX; ++i)
	{
		DWORD pad_id = static_cast<DWORD>(i);
		m_spControllers[i] = std::make_shared<XInput>(pad_id);
	}
}
