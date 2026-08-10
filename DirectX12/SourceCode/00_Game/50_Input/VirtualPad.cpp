#include "VirtualPad.h"
#include <algorithm>
#include <cmath>

VirtualPad::VirtualPad()
{
}

// アクションの状態をチェックする汎用ヘルパー.
template <typename KeyCheckFunc, typename ButtonCheckFunc>
bool VirtualPad::CheckActionState(eGameAction Action, KeyCheckFunc&& KeyCheck, ButtonCheckFunc&& ButtonCheck) const
{
	// アクションのバインディングを取得.
	auto it = m_KeyMap.find(Action);
	if (it == m_KeyMap.end() || it->second.Type != eActionType::Button)
	{
		return false;
	}

	const ActionBinding& binding = it->second;

	for (const auto& source : binding.Sources)
	{
		switch (source.Type)
		{
			case InputSource::eSourceType::KeyBorad:
			case InputSource::eSourceType::MouseButton:
				if (KeyCheck(source.KeyCode))
				{
					return true;
				}
				break;

			case InputSource::eSourceType::ControllerButton:
				if (ButtonCheck(source.ControllerKey, source.KeyCode))
				{
					return true;
				}
				break;

			// コントローラートリガー軸をボタンとして扱う場合の特殊なケース.
			case InputSource::eSourceType::ControllerTriggerAxis:
			{
				float trigger_value = 0.0f;
				if (source.StickTarget == InputSource::eStickTarget::LeftTrigger) {
					trigger_value = Input::GetLTrigger();
				}
				else if (source.StickTarget == InputSource::eStickTarget::RightTrigger) {
					trigger_value = Input::GetRTrigger();
				}

				// トリガーが閾値(0.4f)以上であれば押されていると判定.
				if (trigger_value >= 0.4f) return true;
				break;
			}

			default:
				break;
		}
	}

	return false;
}

// 押され続けているか.
bool VirtualPad::IsActionPress(eGameAction Action) const
{
	auto key_check = [](const int& code) {
		return Input::IsKeyRepeat(code);
		};

	auto button_check = [](XInput::Key key, const int code) {
		return Input::IsButtonRepeat(key);
		};

	return CheckActionState(Action, key_check, button_check);
}

// 押された瞬間か.
bool VirtualPad::IsActionDown(eGameAction Action, float InputBufferTime) const
{
	auto key_check = [](const int& code) {
		// TODO(未実装): InputBufferTimeを使う場合ここに追加.
		return Input::IsKeyDown(code);
		};

	auto button_check = [](XInput::Key key, const int code) {
		return Input::IsButtonDown(key);
		};

	return CheckActionState(Action, key_check, button_check);
}

// 離された瞬間か.
bool VirtualPad::IsActionUp(eGameAction Action) const
{
	auto key_check = [](const int& code) {
		return Input::IsKeyUp(code);
		};

	auto button_check = [](XInput::Key key, const int code) {
		return Input::IsButtonUp(key);
		};

	return CheckActionState(Action, key_check, button_check);
}

// 軸アクションの合計値を取得.
float VirtualPad::GetSingleAxisValue(eGameAction ComponentAction) const
{
	auto it = m_KeyMap.find(ComponentAction);
	if (it == m_KeyMap.end() || it->second.Type != eActionType::Axis)
	{
		return 0.0f;
	}

	const ActionBinding& binding = it->second;
	float total_value = 0.0f;

	for (const auto& source : binding.Sources)
	{
		float value = 0.0f;

		switch (source.Type)
		{
			case InputSource::eSourceType::KeyBorad:
				if (Input::IsKeyRepeat(source.KeyCode))
				{
					value = 1.0f;
				}
				break;

			case InputSource::eSourceType::ControllerStickAxis:
			case InputSource::eSourceType::ControllerTriggerAxis:
				if (source.StickTarget == InputSource::eStickTarget::Left)
				{
					value = (ComponentAction == eGameAction::Move_Axis_X || ComponentAction == eGameAction::Camera_X) ?
						Input::GetLStickDirection().x : Input::GetLStickDirection().y;
				}
				else if (source.StickTarget == InputSource::eStickTarget::Right)
				{
					value = (ComponentAction == eGameAction::Move_Axis_X || ComponentAction == eGameAction::Camera_X) ?
						Input::GetRStickDirection().x : Input::GetRStickDirection().y;
				}
				else if (source.StickTarget == InputSource::eStickTarget::LeftTrigger)
				{
					value = Input::GetLTrigger();
				}
				else if (source.StickTarget == InputSource::eStickTarget::RightTrigger)
				{
					value = Input::GetRTrigger();
				}
				break;

			case InputSource::eSourceType::MouseMove:
				if (ComponentAction == eGameAction::Camera_X)
				{
					value = Input::GetClientCursorDelta().x;
				}
				else if (ComponentAction == eGameAction::Camera_Y)
				{
					value = Input::GetClientCursorDelta().y;
				}
				break;

			default:
				continue;
		}

		total_value += value * source.Scale;
	}

	// 軸の値の最大値を1.0fに制限.
	return std::min(1.0f, std::max(-1.0f, total_value));
}

// 複合軸取得.
DirectX::XMFLOAT2 VirtualPad::GetAxisInput(eGameAxisAction AxisType) const
{
	DirectX::XMFLOAT2 result = { 0.0f, 0.0f };

	// それぞれの軸を取得.
	if (AxisType == eGameAxisAction::Move)
	{
		result.x = GetSingleAxisValue(eGameAction::Move_Axis_X);
		result.y = GetSingleAxisValue(eGameAction::Move_Axis_Y);
	}
	else if (AxisType == eGameAxisAction::CameraMove)
	{
		result.x = GetSingleAxisValue(eGameAction::Camera_X);
		result.y = GetSingleAxisValue(eGameAction::Camera_Y);
	}
	else
	{
		return { 0.0f, 0.0f };
	}

	// 正規化.
	float length_sq = result.x * result.x + result.y * result.y;

	if (length_sq > 1.0f)
	{
		float length = std::sqrt(length_sq);
		result.x /= length;
		result.y /= length;
	}

	return result;
}

// デフォルトバインディング.
void VirtualPad::SetupDefaultBindings()
{
	using EKey = XInput::Key;
	using EStickState = XInput::StickState;
	using ESource = InputSource::eSourceType;
	using ETarget = InputSource::eStickTarget;
	using Action = eGameAction;

	m_KeyMap[Action::MoveForward] = {
		eActionType::Button,
		{
			{ ESource::KeyBorad, 'W' },
			{ ESource::ControllerStickDir, 0, EKey::None, EStickState::Up, ETarget::Left }
		}
	};

	m_KeyMap[Action::MoveBackward] = {
		eActionType::Button,
		{
			{ ESource::KeyBorad, 'S' },
			{ ESource::ControllerStickDir, 0, EKey::None, EStickState::Down, ETarget::Left }
		}
	};

	m_KeyMap[Action::MoveRight] = {
		eActionType::Button,
		{
			{ ESource::KeyBorad, 'D' },
			{ ESource::KeyBorad, VK_RIGHT },
			{ ESource::ControllerStickDir, 0, EKey::None, EStickState::Right, ETarget::Left }
		}
	};

	m_KeyMap[Action::MoveLeft] = {
		eActionType::Button,
		{
			{ ESource::KeyBorad, 'A' },
			{ ESource::KeyBorad, VK_LEFT },
			{ ESource::ControllerStickDir, 0, EKey::None, EStickState::Left, ETarget::Left }
		}
	};

	m_KeyMap[Action::Cancel] = {
		eActionType::Button,
		{
			{ ESource::KeyBorad, VK_SPACE },
			{ ESource::ControllerButton, 0, EKey::A }
		}
	};

	m_KeyMap[Action::Attack] = {
		eActionType::Button,
		{
			{ ESource::MouseButton, VK_LBUTTON },
			{ ESource::ControllerButton, 0, EKey::X }
		}
	};

	m_KeyMap[Action::Parry] = {
		eActionType::Button,
		{
			{ ESource::MouseButton, VK_RBUTTON },
			{ ESource::ControllerTriggerAxis, 0, EKey::None, EStickState::None, ETarget::RightTrigger, 1.0f },
			{ ESource::ControllerTriggerAxis, 0, EKey::None, EStickState::None, ETarget::LeftTrigger, 1.0f }
		}
	};

	m_KeyMap[Action::Dodge] = {
		eActionType::Button,
		{
			{ ESource::KeyBorad, VK_LSHIFT },
			{ ESource::ControllerButton, 0, EKey::LB },
			{ ESource::ControllerButton, 0, EKey::RB },
			{ ESource::ControllerButton, 0, EKey::A }
		}
	};

	m_KeyMap[Action::SpecialAttack] = {
		eActionType::Button,
		{
			{ ESource::KeyBorad, 'Q' },
			{ ESource::ControllerButton, 0, EKey::Y }
		}
	};

	m_KeyMap[Action::Pause] = {
		eActionType::Button,
		{
			{ ESource::KeyBorad, VK_ESCAPE },
			{ ESource::ControllerButton, 0, EKey::Start }
		}
	};

	// ----- [ Axis Actions: 軸入力の内部コンポーネント ] -----.

	m_KeyMap[Action::Move_Axis_X] = {
		eActionType::Axis,
		{
			{ ESource::ControllerStickAxis, 0, EKey::None, EStickState::Right, ETarget::Left, 1.0f },
			{ ESource::KeyBorad, 'D', EKey::None, EStickState::None, ETarget::None, 1.0f },
			{ ESource::KeyBorad, 'A', EKey::None, EStickState::None, ETarget::None, -1.0f },
		}
	};

	m_KeyMap[Action::Move_Axis_Y] = {
		eActionType::Axis,
		{
			{ ESource::ControllerStickAxis, 0, EKey::None, EStickState::Up, ETarget::Left, 1.0f },
			{ ESource::KeyBorad, 'W', EKey::None, EStickState::None, ETarget::None, 1.0f },
			{ ESource::KeyBorad, 'S', EKey::None, EStickState::None, ETarget::None, -1.0f },
		}
	};

	m_KeyMap[Action::Camera_X] = {
		eActionType::Axis,
		{
			{ ESource::ControllerStickAxis, 0, EKey::None, EStickState::Right, ETarget::Right, 1.0f },
			{ ESource::MouseMove, 0, EKey::None, EStickState::None, ETarget::None, 0.5f }
		}
	};

	// (Y軸は反転).
	m_KeyMap[Action::Camera_Y] = {
		eActionType::Axis,
		{
			{ ESource::ControllerStickAxis, 0, EKey::None, EStickState::Up, ETarget::Right, -1.0f },
			{ ESource::MouseMove, 0, EKey::None, EStickState::None, ETarget::None, -0.5f }
		}
	};
}
