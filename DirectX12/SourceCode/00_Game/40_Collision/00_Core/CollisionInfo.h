#pragma once

#include <DirectXMath.h>

class ColliderBase;

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/11.
* @brief     : 衝突判定の結果. SelfCollider/OtherColliderは「誰から見た結果か」で
*            : 意味が変わる(CollisionDetectorが両者それぞれの視点の値を配る).
**********************************************************************************/

struct CollisionInfo
{
	bool IsHit = false; // 衝突が発生したか.

	DirectX::XMFLOAT3 Normal = {};			// 衝突法線(SelfCollider→OtherCollider方向).
	float PenetrationDepth = 0.0f;			// めり込みの深さ.
	DirectX::XMFLOAT3 ContactPoint = {};	// 接触点(ワールド座標).

	float AttackAmount = 0.0f; // 相手の攻撃力(CollisionDetectorが設定する).

	const ColliderBase* SelfCollider = nullptr; // 衝突に関わったコライダー.
	const ColliderBase* OtherCollider = nullptr;
};
