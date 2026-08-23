#pragma once

#include "00_Game/30_Camera/00_Base/CameraBase.h"

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/07.
* @brief     : ターゲットを追従して周回する三人称カメラ.
**********************************************************************************/

class ThirdPersonCamera final
	: public CameraBase
{
public:
	ThirdPersonCamera();
	virtual ~ThirdPersonCamera() override;

	virtual void Update() override;

	// アクティブ化: カーソルを中央固定・非表示にする.
	virtual void OnActivated() override;
	// 非アクティブ化: カーソルの中央固定・非表示を解除する.
	virtual void OnDeactivated() override;

	// 追従対象の座標を設定(毎フレーム呼び出す想定).
	void SetTargetPosition(const DirectX::XMFLOAT3& Position) noexcept;
	// 注視点オフセットの設定.
	void SetLookOffset(const DirectX::XMFLOAT3& Offset) noexcept;
	// 追従距離の設定.
	void SetDistance(float Distance) noexcept;
	// 周回速度の設定(ラジアン/秒).
	void SetOrbitSpeed(float Speed) noexcept;

	// マウス回転係数の設定(Settings.jsonへ永続化される).
	void SetMouseRotationSpeed(float Speed) noexcept { m_MouseRotationSpeed = Speed; }

private:
	DirectX::XMFLOAT3 m_TargetPosition;	// 追従対象の座標.
	DirectX::XMFLOAT3 m_LookOffset;		// 注視点オフセット.
	float             m_Distance;			// 追従対象からの距離.
	float             m_OrbitSpeed;		// 周回速度(ラジアン/秒).
	float             m_MouseRotationSpeed = 0.0035f; // マウス1pxあたりの回転量(ラジアン). Settings.jsonから復元.
};
