#include "Boss.h"

#include <cmath>

#include "00_Game/10_Object/10_MeshObject/00_Character/20_Boss/State/00_Idle/Idle.h"
#include "00_Game/10_Object/10_MeshObject/00_Character/20_Boss/State/10_Move/Move.h"
#include "00_Game/10_Object/10_MeshObject/00_Character/20_Boss/State/20_Attack/Attack.h"
#include "00_Game/10_Object/10_MeshObject/00_Character/20_Boss/State/21_Attack2/Attack2.h"
#include "00_Game/10_Object/10_MeshObject/00_Character/20_Boss/State/22_BeamAttack/BeamAttack.h"
#include "00_Game/10_Object/10_MeshObject/00_Character/20_Boss/State/23_JumpAttack/JumpAttack.h"
#include "00_Game/10_Object/10_MeshObject/00_Character/20_Boss/State/24_SpinAttack/SpinAttack.h"
#include "00_Game/10_Object/10_MeshObject/00_Character/20_Boss/State/30_Dead/Dead.h"
#include "00_Game/10_Object/10_MeshObject/00_Character/20_Boss/State/40_ParryReaction/ParryReaction.h"
#include "00_Game/60_Combat/CombatTuning.h"
#include "99_Utility/Event/EventBus.h"
#include "99_Utility/Debug/Imgui/SoundEventEditor.h"
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
	m_PendingStaggerKnockBack = { 0.0f, 0.0f, 0.0f }; // 前回硬直の未消費な吹き飛び要求を持ち越さない.

	m_StateMachine.ChangeState(std::make_shared<BossState::ParryReaction>(this, TargetPosition, TargetYawDeg, Duration));
	m_CurrentStateID = BossState::eID::ParryReaction;
}

// 硬直中に攻撃を命中させられた時の「決まった!」演出(通常ヒットより大きく吹き飛び、専用SE. Player::OnDamagedと同じ方向計算).
void Boss::OnDamaged(const HitEvent& Event)
{
	// 硬直中以外はノーリアクション(パリィ成立の利得は硬直中のヒットだけに付く).
	if (m_CurrentStateID != BossState::eID::ParryReaction) { return; }

	// 吹き飛び方向(接触点から離れる水平方向が最も確実. Normalはフォールバック扱い).
	DirectX::XMFLOAT3 direction{ 0.0f, 0.0f, 1.0f };
	{
		const DirectX::XMFLOAT3 my_pos = GetPosition();
		const float dir_x = my_pos.x - Event.ContactPoint.x;
		const float dir_z = my_pos.z - Event.ContactPoint.z;
		const float length_sq = dir_x * dir_x + dir_z * dir_z;

		if (length_sq > 1e-6f) {
			const float inv_length = 1.0f / std::sqrtf(length_sq);
			direction = { dir_x * inv_length, 0.0f, dir_z * inv_length };
		}
		else {
			const float normal_x = -Event.Normal.x;
			const float normal_z = -Event.Normal.z;
			const float length = std::sqrtf(normal_x * normal_x + normal_z * normal_z);
			if (length > 1e-6f) {
				direction = { normal_x / length, 0.0f, normal_z / length };
			}
		}
	}

	const float speed = CombatTuning::Get().ParryStaggerKnockBackSpeed;
	m_PendingStaggerKnockBack = { direction.x * speed, 0.0f, direction.z * speed };

	SoundEventEditor::PlayCombatEvent("attack_hit_stagger"); // Sound Eventで定義されていればSE再生.
}
