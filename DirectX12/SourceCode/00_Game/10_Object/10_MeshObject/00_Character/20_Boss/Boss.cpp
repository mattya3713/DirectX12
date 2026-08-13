#include "Boss.h"

#include "00_Game/10_Object/10_MeshObject/00_Character/20_Boss/State/00_Idle/Idle.h"
#include "00_Game/10_Object/10_MeshObject/00_Character/20_Boss/State/10_Move/Move.h"
#include "00_Game/10_Object/10_MeshObject/00_Character/20_Boss/State/20_Attack/Attack.h"
#include "00_Game/10_Object/10_MeshObject/00_Character/20_Boss/State/30_Dead/Dead.h"

namespace {
	// Enemyの既定値(4.0/10.0/2.5/20.0)より大柄・広範囲(仮値. 専用攻撃パターン実装時に見直す).
	constexpr float BOSS_MOVE_SPEED   = 3.0f;
	constexpr float BOSS_AGGRO_RANGE  = 15.0f;
	constexpr float BOSS_ATTACK_RANGE = 3.5f;
	constexpr float BOSS_LOSE_RANGE   = 30.0f;
}

Boss::Boss()
	: Enemy          { BOSS_MOVE_SPEED, BOSS_AGGRO_RANGE, BOSS_ATTACK_RANGE, BOSS_LOSE_RANGE }
	, m_StateMachine { this }
{
	// Enemy()が登録したOnDeathはEnemy自身の(Bossからは使わない)StateMachine<Enemy>を
	// 動かすものなので、Boss専用StateMachineを動かすものに差し替える.
	SetOnDeath([this]() { ChangeState(BossState::eID::Dead); });

	ChangeState(BossState::eID::Idle);
}

Boss::~Boss() = default;

void Boss::Update()
{
	m_StateMachine.Update();
	m_StateMachine.LateUpdate();

	Character::Update(); // Enemy::Update()は使わない(Enemy側の未使用StateMachineを動かさないため).
}

void Boss::ChangeState(BossState::eID Id)
{
	switch (Id)
	{
	case BossState::eID::Idle:
		m_StateMachine.ChangeState(std::make_shared<BossState::Idle>(this));
		break;

	case BossState::eID::Move:
		m_StateMachine.ChangeState(std::make_shared<BossState::Move>(this));
		break;

	case BossState::eID::Attack:
		m_StateMachine.ChangeState(std::make_shared<BossState::Attack>(this));
		break;

	case BossState::eID::Dead:
		m_StateMachine.ChangeState(std::make_shared<BossState::Dead>(this));
		break;

	default:
		return;
	}

	m_CurrentStateID = Id;
}
