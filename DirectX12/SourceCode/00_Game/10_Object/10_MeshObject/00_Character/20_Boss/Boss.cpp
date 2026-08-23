#include "Boss.h"

#include "00_Game/10_Object/10_MeshObject/00_Character/20_Boss/State/00_Idle/Idle.h"
#include "00_Game/10_Object/10_MeshObject/00_Character/20_Boss/State/10_Move/Move.h"
#include "00_Game/10_Object/10_MeshObject/00_Character/20_Boss/State/20_Attack/Attack.h"
#include "00_Game/10_Object/10_MeshObject/00_Character/20_Boss/State/21_Attack2/Attack2.h"
#include "00_Game/10_Object/10_MeshObject/00_Character/20_Boss/State/22_BeamAttack/BeamAttack.h"
#include "00_Game/10_Object/10_MeshObject/00_Character/20_Boss/State/23_JumpAttack/JumpAttack.h"
#include "00_Game/10_Object/10_MeshObject/00_Character/20_Boss/State/24_SpinAttack/SpinAttack.h"
#include "00_Game/10_Object/10_MeshObject/00_Character/20_Boss/State/30_Dead/Dead.h"
#include "00_Game/10_Object/10_MeshObject/00_Character/20_Boss/State/40_ParryReaction/ParryReaction.h"
#include "99_Utility/Event/EventBus.h"
#include "99_Utility/ServiceLocator/ServiceLocator.h"

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
	SetOnDeath([this]() {
		ChangeState(BossState::eID::Dead);

		// EventBusデモ: 既存OnDeathコールバックの隣で同じ死亡通知を配信する
		// (既存コールバックの置き換えではなく共存. 本格導入は別タスク).
		if (EventBus* p_event_bus = ServiceLocator::Get<EventBus>()) {
			p_event_bus->Publish(BossDefeatedEvent{ this });
		}
	});

	// 撃破シーケンス用: 通常攻撃ではHP最大値の5%未満へ下がらない(仮値. 最後の一撃は必殺のみ).
	m_Health.SetMinHP(m_Health.GetMaxHP() * 0.05f);

	ChangeState(BossState::eID::Idle);
}

Boss::~Boss() = default;

// 必殺撃破成立用: HP下限を無視してHPを0へ直接設定する.
// NOTE: SetHPはOnDeathコールバックを通らないため、撃破成立イベントと
//       Dead状態への遷移は撃破シーケンス基盤(MainScene)側が責任を持つ.
void Boss::ForceKill()
{
	m_Health.SetHP(0.0f);
}

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

	case BossState::eID::Attack2:
		m_StateMachine.ChangeState(std::make_shared<BossState::Attack2>(this));
		break;

	case BossState::eID::BeamAttack:
		m_StateMachine.ChangeState(std::make_shared<BossState::BeamAttack>(this));
		break;

	case BossState::eID::JumpAttack:
		m_StateMachine.ChangeState(std::make_shared<BossState::JumpAttack>(this));
		break;

	case BossState::eID::SpinAttack:
		m_StateMachine.ChangeState(std::make_shared<BossState::SpinAttack>(this));
		break;

	case BossState::eID::Dead:
		m_StateMachine.ChangeState(std::make_shared<BossState::Dead>(this));
		break;

	default:
		return;
	}

	m_CurrentStateID = Id;
}

void Boss::EnterParryReaction(const DirectX::XMFLOAT3& TargetPosition, float TargetYawDeg, float Duration)
{
	m_StateMachine.ChangeState(std::make_shared<BossState::ParryReaction>(this, TargetPosition, TargetYawDeg, Duration));
	m_CurrentStateID = BossState::eID::ParryReaction;
}
