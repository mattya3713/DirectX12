#include "Enemy.h"

#include "00_Game/10_Object/10_MeshObject/00_Character/10_Enemy/State/00_Idle/Idle.h"
#include "00_Game/10_Object/10_MeshObject/00_Character/10_Enemy/State/10_Chase/Chase.h"
#include "00_Game/10_Object/10_MeshObject/00_Character/10_Enemy/State/20_Attack/Attack.h"
#include "00_Game/10_Object/10_MeshObject/00_Character/10_Enemy/State/30_Dead/Dead.h"

Enemy::Enemy()
	: Enemy(4.0f, 10.0f, 2.5f, 20.0f) // 既定の索敵AI調整値(コメント参照).
{
}

Enemy::Enemy(float MoveSpeed, float AggroRange, float AttackRange, float LoseRange)
	: m_StateMachine { this }
	, m_MoveSpeed    { MoveSpeed }
	, m_AggroRange   { AggroRange }
	, m_AttackRange  { AttackRange }
	, m_LoseRange    { LoseRange }
{
	m_DamageCollider.SetMyMask(eCollisionGroup::EnemyDamage);
	m_DamageCollider.SetTargetMask(eCollisionGroup::PlayerAttack);

	m_AttackCollider.SetMyMask(eCollisionGroup::EnemyAttack);
	// PlayerParryも対象にする(PlayerState::Parry中はPlayerParryだけが有効なため、
	// これが無いとパリィ中の攻撃が誰にも衝突しなくなってしまう).
	m_AttackCollider.SetTargetMask(eCollisionGroup::PlayerDamage | eCollisionGroup::PlayerParry);

	// 実体判定(Player実体との押し出し専用).
	SetBodyCollisionMasks(eCollisionGroup::EnemyBody, eCollisionGroup::PlayerBody);

	// HPが0になった瞬間にDeadへ遷移する(HealthSystem側で生存→死亡の1回だけ発火する).
	SetOnDeath([this]() { ChangeState(EnemyState::eID::Dead); });

	ChangeState(EnemyState::eID::Idle);
}

Enemy::~Enemy() = default;

void Enemy::Update()
{
	m_StateMachine.Update();
	m_StateMachine.LateUpdate();

	Character::Update(); // Transform確定後にメッシュへ反映しつつ、被弾判定も処理する.
}

void Enemy::ChangeState(EnemyState::eID Id)
{
	switch (Id)
	{
	case EnemyState::eID::Idle:
		m_StateMachine.ChangeState(std::make_shared<EnemyState::Idle>(this));
		break;

	case EnemyState::eID::Chase:
		m_StateMachine.ChangeState(std::make_shared<EnemyState::Chase>(this));
		break;

	case EnemyState::eID::Attack:
		m_StateMachine.ChangeState(std::make_shared<EnemyState::Attack>(this));
		break;

	case EnemyState::eID::Dead:
		m_StateMachine.ChangeState(std::make_shared<EnemyState::Dead>(this));
		break;

	default:
		return;
	}

	m_CurrentStateID = Id;
}
