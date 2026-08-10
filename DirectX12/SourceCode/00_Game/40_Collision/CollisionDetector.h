#pragma once

#include <vector>

class ColliderBase;

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/11.
* @brief     : 登録された全Colliderの総当たり判定を毎フレーム実行するマネージャー.
*            : ServiceLocator経由で利用する(Mainが所有・登録する想定. Senzanでは
*            : シングルトンだったが、本プロジェクトの方針(マネージャーはサービス
*            : ロケーター経由)に合わせた).
* @pattern   : ServiceLocator.
**********************************************************************************/

class CollisionDetector final
{
public:
	CollisionDetector()  = default;
	~CollisionDetector() = default;

	CollisionDetector(const CollisionDetector&)            = delete;
	CollisionDetector& operator=(const CollisionDetector&) = delete;

	// 毎フレーム呼び出し、登録済みColliderの総当たり判定を実行する.
	void ExecuteCollisionDetection();

	// コライダーの登録・解除(所有権は移らない).
	void RegisterCollider(ColliderBase& Collider);
	void UnregisterCollider(ColliderBase* pCollider);

private:
	std::vector<ColliderBase*> m_Colliders; // 登録されたコライダー(非所有).
};
