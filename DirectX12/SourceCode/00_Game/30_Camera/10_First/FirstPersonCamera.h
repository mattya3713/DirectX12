#pragma once

#include "00_Game/30_Camera/00_Base/CameraBase.h"

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/07.
* @brief     : 一人称カメラ.
**********************************************************************************/

class FirstPersonCamera final
	: public CameraBase
{
public:
	FirstPersonCamera();
	virtual ~FirstPersonCamera() override;

	virtual void Update() override;

	// 移動速度の設定.
	void SetMoveSpeed(float Speed) noexcept;
	// 視点回転速度の設定(ラジアン/秒).
	void SetLookSpeed(float Speed) noexcept;

private:
	// 視点回転入力を反映する.
	void UpdateLook(float DeltaTime);
	// 移動入力を反映する.
	void UpdateMove(float DeltaTime);

private:
	float m_MoveSpeed;	// 移動速度.
	float m_LookSpeed;	// 視点回転速度(ラジアン/秒).
	float m_PitchLimit;	// ピッチの可動域制限(ラジアン).
};
