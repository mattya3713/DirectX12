#include "CollisionDetector.h"

#include <algorithm>
#include <cmath>
#include <DirectXMath.h>

#include "00_Game/40_Collision/00_Core/ColliderBase.h"

void CollisionDetector::ExecuteCollisionDetection()
{
	// 前フレームの情報をクリア.
	for (ColliderBase* p_collider : m_Colliders)
	{
		if (p_collider) { p_collider->ClearCollisionEvents(); }
	}

	// Broad Phase: 詳細判定は形状計算が重いので、まずXZ平面の外接球距離で弾く.
	// (コライダーはYaw回転のみだが高さ方向の分離は見ない. 弾き漏れ=保守的なので安全).
	struct Entry
	{
		ColliderBase* p_collider;
		float x;
		float z;
		float radius;
	};
	static std::vector<Entry> entries; // 再確保を避けるため使い回す(シングルスレッド専用).
	entries.clear();
	entries.reserve(m_Colliders.size());

	for (ColliderBase* p_collider : m_Colliders)
	{
		if (!p_collider || !p_collider->GetActive()) { continue; }

		const DirectX::XMFLOAT3 position = p_collider->GetPosition();
		entries.push_back({ p_collider, position.x, position.z, p_collider->GetBoundRadius() });
	}

	const size_t entry_count = entries.size();
	for (size_t i = 0; i < entry_count; ++i)
	{
		const Entry& a = entries[i];
		for (size_t j = i + 1; j < entry_count; ++j)
		{
			const Entry& b = entries[j];

			const float dx = b.x - a.x;
			const float dz = b.z - a.z;
			const float reach = a.radius + b.radius;
			if (dx * dx + dz * dz > reach * reach) { continue; } // XZで分離 → 詳細判定不要.

			CollisionInfo info = a.p_collider->CheckCollision(*b.p_collider);
			if (!info.IsHit) { continue; }

			// Aへの情報(相手Bの攻撃力を持たせる).
			info.AttackAmount = b.p_collider->GetAttackAmount();
			info.AttackActivationId = b.p_collider->GetActivationId();
			a.p_collider->AddCollisionInfo(info);

			// Bへは法線を反転し、コライダーの左右を入れ替えた情報を渡す.
			CollisionInfo info_reverse = info;
			DirectX::XMVECTOR v_normal_reverse = DirectX::XMLoadFloat3(&info.Normal);
			v_normal_reverse = DirectX::XMVectorNegate(v_normal_reverse);
			DirectX::XMStoreFloat3(&info_reverse.Normal, v_normal_reverse);

			info_reverse.SelfCollider    = info.OtherCollider;
			info_reverse.OtherCollider    = info.SelfCollider;
			info_reverse.AttackAmount = a.p_collider->GetAttackAmount();
			info_reverse.AttackActivationId = a.p_collider->GetActivationId();

			b.p_collider->AddCollisionInfo(info_reverse);
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
