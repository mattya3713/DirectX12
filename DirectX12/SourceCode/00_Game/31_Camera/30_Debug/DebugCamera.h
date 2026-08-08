#pragma once

#include "00_Game/31_Camera/00_Base/CameraBase.h"

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/07.
* @brief     : 自由に飛び回れるデバッグ用カメラ(視点と注視点が常に平行移動する).
**********************************************************************************/

class DebugCamera final
	: public CameraBase
{
public:
	DebugCamera();
	virtual ~DebugCamera() override;

	virtual void Update() override;

	// 移動速度の設定.
	void SetMoveSpeed(float Speed) noexcept;

private:
	float m_MoveSpeed;		// 移動速度.
	float m_SlowMoveSpeed;	// Shift押下時の低速移動速度.
};
