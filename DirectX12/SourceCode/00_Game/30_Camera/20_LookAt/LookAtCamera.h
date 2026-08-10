#pragma once

#include "00_Game/30_Camera/00_Base/CameraBase.h"

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/07.
* @brief     : 固定注視点を中心に周回するカメラ(モデル閲覧用).
**********************************************************************************/

class LookAtCamera final
	: public CameraBase
{
public:
	LookAtCamera();
	virtual ~LookAtCamera() override;

	virtual void Update() override;

	// 周回の中心(注視点)を設定.
	void SetPivot(const DirectX::XMFLOAT3& Pivot) noexcept;
	// 中心からの距離を設定.
	void SetDistance(float Distance) noexcept;
	// 周回速度の設定(ラジアン/秒).
	void SetOrbitSpeed(float Speed) noexcept;

private:
	DirectX::XMFLOAT3 m_Pivot;			// 周回の中心.
	float             m_Distance;		// 中心からの距離.
	float             m_OrbitSpeed;	// 周回速度(ラジアン/秒).
};
