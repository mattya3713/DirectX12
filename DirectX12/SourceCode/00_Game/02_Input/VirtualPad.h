#pragma once

#include <map>
#include <vector>
#include <DirectXMath.h>

#include "00_Game/02_Input/Input.h"

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/09.
* @brief     : 仮想パッド入力ラッパークラス
*            : キーボード/マウス/コントローラ等の入力を抽象化して
*            : ゲーム内のアクションへマッピングする機能を提供する。ServiceLocator経由で利用する.
**********************************************************************************/

class VirtualPad final
{
public:
	VirtualPad();
	~VirtualPad() = default;

	// ゲーム内アクション列挙.
	enum class eGameAction
	{
		None,
		MoveForward,
		MoveBackward,
		MoveRight,
		MoveLeft,
		Cancel,
		Attack,
		Parry,
		Dodge,
		Pause,
		SpecialAttack,

		// 軸入力用コンポーネント.
		Move_Axis_X,
		Move_Axis_Y,
		Camera_X,
		Camera_Y,
	};

	// 軸入力の種類.
	enum class eGameAxisAction
	{
		None,
		CameraMove,
		Move,
	};

	// アクションのタイプ(ボタンか軸か).
	enum class eActionType
	{
		Button,
		Axis
	};

	// 入力ソースを表す構造体。キーボード/マウス/コントローラ等の情報を保持する.
	struct InputSource
	{
		enum class eSourceType
		{
			KeyBorad,
			MouseButton,
			MouseMove,
			ControllerButton,
			ControllerStickDir,
			ControllerStickAxis,
			ControllerTriggerAxis
		};

		eSourceType Type;								// 入力ソースの種類.
		int KeyCode = 0;								// キーコード(キーボード用).
		XInput::Key ControllerKey = XInput::Key::None;	// コントローラボタン.

		XInput::StickState StickState = XInput::StickState::None; // スティック状態.

		enum class eStickTarget
		{
			None,
			Left,
			Right,
			LeftTrigger,
			RightTrigger
		};

		eStickTarget StickTarget = eStickTarget::None; // スティックの対象.

		float Scale = 1.0f; // 入力スケール(軸系で使用).
	};

	// アクションにバインドされた入力群を表す構造体.
	struct ActionBinding
	{
		eActionType Type = eActionType::Button;	// アクションタイプ.
		std::vector<InputSource> Sources;		// バインドされた入力ソース一覧.
	};

public:
	// アクション -> バインディングのマップ。外部から直接参照する必要があるためメンバとして公開している.
	std::map<eGameAction, ActionBinding> m_KeyMap;

public:
	// 指定アクションが押され続けているかを返す.
	bool IsActionPress(eGameAction Action) const;

	// 指定アクションが押された瞬間かを返す(入力バッファ対応).
	bool IsActionDown(eGameAction Action, float InputBufferTime = 0.0f) const;

	// 指定アクションが離された瞬間かを返す.
	bool IsActionUp(eGameAction Action) const;

	// 軸入力の取得.
	DirectX::XMFLOAT2 GetAxisInput(eGameAxisAction AxisType) const;

	// デフォルトの入力バインディングをセットアップする.
	void SetupDefaultBindings();

private:
	// アクションの状態をチェックする汎用ヘルパー.
	template <typename KeyCheckFunc, typename ButtonCheckFunc>
	bool CheckActionState(eGameAction Action, KeyCheckFunc&& KeyCheck, ButtonCheckFunc&& ButtonCheck) const;

	// 単一コンポーネント(例: Move_Axis_X等)の軸値を取得する.
	float GetSingleAxisValue(eGameAction ComponentAction) const;

private:
	// TODO(未実装): コヨーテタイム等の入力補正.
	float m_CoyoteTimeTimer = 0.0f;
};
