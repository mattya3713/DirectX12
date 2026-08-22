#pragma once

#include "00_Game/30_Camera/00_Base/CameraBase.h"

/**********************************************************************************
* @author    : Coder.
* @date      : 2026/08/23.
* @brief     : Playerを追従しつつ常にBossを注視するロックオンカメラ.
**********************************************************************************/

class LockOnCamera final
	: public CameraBase
{
public:
	LockOnCamera();
	virtual ~LockOnCamera() override;

	virtual void Update() override;

	// 追従対象(Player)の座標を設定(毎フレーム呼び出す想定).
	void SetPlayerPosition(const DirectX::XMFLOAT3& Position) noexcept;
	// 注視対象(Boss)の座標を設定(毎フレーム呼び出す想定).
	void SetBossPosition(const DirectX::XMFLOAT3& Position) noexcept;
	// Boss注視点の高さオフセットの設定.
	void SetLookOffset(const DirectX::XMFLOAT3& Offset) noexcept;
	// Playerからの追従距離の設定.
	void SetDistance(float Distance) noexcept;
	// カメラの高さオフセットの設定.
	void SetHeight(float Height) noexcept;
	// 追従補間速度の設定(大きいほど速く追いつく).
	void SetFollowSpeed(float Speed) noexcept;

private:
	DirectX::XMFLOAT3 m_PlayerPosition;	// 追従対象(Player)の座標.
	DirectX::XMFLOAT3 m_BossPosition;	// 注視対象(Boss)の座標.
	DirectX::XMFLOAT3 m_LookOffset;		// Boss注視点オフセット.
	float             m_Distance;		// Player後方への距離.
	float             m_Height;			// Playerからの高さオフセット.
	float             m_FollowSpeed;	// 位置補間速度(1/秒).
	bool              m_IsInitialized;	// 初回Update直後のスナップ用.
};
