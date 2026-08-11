#include "Character.h"

#include "00_Game/40_Collision/CollisionDetector.h"
#include "99_Utility/ServiceLocator/ServiceLocator.h"

Character::Character()
	: m_Health         { 100.0f }
	, m_DamageCollider { &GetTransform() }
	, m_AttackCollider { &GetTransform() }
{
	m_DamageCollider.SetRadius(0.5f);
	m_DamageCollider.SetHeight(2.0f);
	m_DamageCollider.SetPositionOffset({ 0.0f, 1.0f, 0.0f });

	m_AttackCollider.SetRadius(1.0f);
	m_AttackCollider.SetHeight(2.0f);
	m_AttackCollider.SetPositionOffset({ 0.0f, 1.0f, 1.5f }); // 正面側に配置.
	m_AttackCollider.SetActive(false); // 攻撃系Stateが有効化するまで無効.

	if (CollisionDetector* p_detector = ServiceLocator::Get<CollisionDetector>())
	{
		p_detector->RegisterCollider(m_DamageCollider);
		p_detector->RegisterCollider(m_AttackCollider);
	}
}

Character::~Character()
{
	if (CollisionDetector* p_detector = ServiceLocator::Get<CollisionDetector>())
	{
		p_detector->UnregisterCollider(&m_DamageCollider);
		p_detector->UnregisterCollider(&m_AttackCollider);
	}
}

void Character::Update()
{
	MeshObject::Update();
	ProcessHits();
}

void Character::ProcessHits()
{
	for (const CollisionInfo& info : m_DamageCollider.GetCollisionEvents())
	{
		if (!info.IsHit) { continue; }

		HitEvent hit_event{};
		hit_event.AttackAmount = info.AttackAmount;
		hit_event.ContactPoint = info.ContactPoint;
		hit_event.Normal       = info.Normal;

		ApplyDamage(hit_event);
	}
}
