#include "HealthSystem.h"

#include <algorithm>

HealthSystem::HealthSystem(float MaxHP) noexcept
	: m_MaxHP    { MaxHP }
	, m_HP       { MaxHP }
	, m_OnDamage {}
	, m_OnDeath  {}
{
}

void HealthSystem::ApplyDamage(float DamageAmount)
{
	const bool was_alive = IsAlive();

	m_HP = std::clamp(m_HP - DamageAmount, m_MinHP, m_MaxHP);

	if (m_OnDamage) { m_OnDamage(DamageAmount); }

	// 生存→死亡に変化した瞬間だけ呼ぶ(死亡後の追撃で何度も呼ばれないようにする).
	if (was_alive && !IsAlive() && m_OnDeath)
	{
		m_OnDeath();
	}
}
