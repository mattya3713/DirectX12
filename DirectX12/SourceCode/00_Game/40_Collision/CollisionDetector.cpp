#include "CollisionDetector.h"

#include <algorithm>
#include <DirectXMath.h>

#include "00_Game/40_Collision/00_Core/ColliderBase.h"

void CollisionDetector::ExecuteCollisionDetection()
{
	// 前フレームの情報をクリア.
	for (ColliderBase* p_collider : m_Colliders)
	{
		if (p_collider) { p_collider->ClearCollisionEvents(); }
	}

	// 総当たりチェック.
	for (size_t i = 0; i < m_Colliders.size(); ++i)
	{
		for (size_t j = i + 1; j < m_Colliders.size(); ++j)
		{
			ColliderBase* p_collider_a = m_Colliders[i];
			ColliderBase* p_collider_b = m_Colliders[j];
			if (!p_collider_a || !p_collider_b) { continue; }
			if (!p_collider_a->GetActive() || !p_collider_b->GetActive()) { continue; }

			CollisionInfo info = p_collider_a->CheckCollision(*p_collider_b);
			if (!info.IsHit) { continue; }

			// Aへの情報(相手Bの攻撃力を持たせる).
			info.AttackAmount = p_collider_b->GetAttackAmount();
			p_collider_a->AddCollisionInfo(info);

			// Bへは法線を反転し、コライダーの左右を入れ替えた情報を渡す.
			CollisionInfo info_reverse = info;
			DirectX::XMVECTOR v_normal_reverse = DirectX::XMLoadFloat3(&info.Normal);
			v_normal_reverse = DirectX::XMVectorNegate(v_normal_reverse);
			DirectX::XMStoreFloat3(&info_reverse.Normal, v_normal_reverse);

			info_reverse.SelfCollider    = info.OtherCollider;
			info_reverse.OtherCollider    = info.SelfCollider;
			info_reverse.AttackAmount = p_collider_a->GetAttackAmount();

			p_collider_b->AddCollisionInfo(info_reverse);
		}
	}
}

void CollisionDetector::RegisterCollider(ColliderBase& Collider)
{
	m_Colliders.push_back(&Collider);
}

void CollisionDetector::UnregisterCollider(ColliderBase* pCollider)
{
	if (!pCollider) { return; }

	const auto it = std::remove(m_Colliders.begin(), m_Colliders.end(), pCollider);
	m_Colliders.erase(it, m_Colliders.end());
}
