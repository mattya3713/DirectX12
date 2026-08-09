#include "KeyInput.h"
#include "99_Utility/ServiceLocator/ServiceLocator.h"

KeyInput::KeyInput()
	: m_NowKeyState {}
	, m_OldKeyState {}
{
}

KeyInput::~KeyInput()
{
}

void KeyInput::Update()
{
	KeyInput* p_instance = ServiceLocator::Get<KeyInput>();

	// 更新前の現在の状態をコピー.
	memcpy_s(p_instance->m_OldKeyState, sizeof(p_instance->m_OldKeyState), p_instance->m_NowKeyState, sizeof(p_instance->m_NowKeyState));

	// 入力されているキーを調べる.
	if (GetKeyboardState(p_instance->m_NowKeyState) == false) { return; }
}

bool KeyInput::IsKeyPress(const int& Key)
{
	KeyInput* p_instance = ServiceLocator::Get<KeyInput>();
	if ((p_instance->m_NowKeyState[Key] & 0x80) != 0)
	{
		return true;
	}
	return false;
}

bool KeyInput::IsKeyPress(const std::vector<int>& KeyList)
{
	for (const auto& key : KeyList)
	{
		if (IsKeyPress(key) == false)
		{
			return false;
		}
	}
	return true;
}

bool KeyInput::IsKeyDown(const int& Key)
{
	KeyInput* p_instance = ServiceLocator::Get<KeyInput>();

	// 現在入力で前回未入力なら押した瞬間.
	if ((p_instance->m_NowKeyState[Key] & 0x80) != 0 &&
		(p_instance->m_OldKeyState[Key] & 0x80) == 0)
	{
		return true;
	}
	return false;
}

bool KeyInput::IsKeyDown(const std::vector<int>& KeyList)
{
	if (IsKeyDown(KeyList.back()) == false) { return false; }
	for (const auto& key : KeyList)
	{
		if (IsKeyPress(key) == false)
		{
			return false;
		}
	}
	return true;
}

bool KeyInput::IsKeyUp(const int& Key)
{
	KeyInput* p_instance = ServiceLocator::Get<KeyInput>();

	// 現在未入力で前回入力なら離した瞬間.
	if ((p_instance->m_NowKeyState[Key] & 0x80) == 0 &&
		(p_instance->m_OldKeyState[Key] & 0x80) != 0)
	{
		return true;
	}
	return false;
}

bool KeyInput::IsKeyRepeat(const int& Key)
{
	KeyInput* p_instance = ServiceLocator::Get<KeyInput>();

	// 現在入力で前回入力なら押し続けている.
	if ((p_instance->m_NowKeyState[Key] & 0x80) != 0 &&
		(p_instance->m_OldKeyState[Key] & 0x80) != 0)
	{
		return true;
	}
	return false;
}

bool KeyInput::IsKeyRepeat(const std::vector<int>& KeyList)
{
	for (const auto& key : KeyList)
	{
		if (IsKeyRepeat(key) == false)
		{
			return false;
		}
	}
	return true;
}
