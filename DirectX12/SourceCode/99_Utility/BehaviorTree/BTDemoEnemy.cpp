#include "BTDemoEnemy.h"

#include <cmath>
#include <cstdio>

#ifdef _WIN32
#include <windows.h>
#endif

#include "99_Utility/BehaviorTree/ActionNode.h"
#include "99_Utility/BehaviorTree/RootNode.h"
#include "99_Utility/BehaviorTree/SelectorNode.h"
#include "99_Utility/BehaviorTree/SequenceNode.h"

namespace {

	// フェーズの切替ログを出力する(実機ではデバッグ出力/ImGuiログでFSMと比較する用).
	void OutputPhaseLog(const char* pFrom, const char* pTo)
	{
		char text[96];
		std::snprintf(text, sizeof(text), "[BTDemoEnemy] Phase: %s -> %s\n", pFrom, pTo);

#ifdef _WIN32
		OutputDebugStringA(text);
#else
		std::printf("%s", text);
#endif
	}

	// フェーズ名の取得(ログ用).
	const char* PhaseName(BTDemoEnemy::ePhase Phase) noexcept
	{
		switch (Phase)
		{
		case BTDemoEnemy::ePhase::Chase:  return "Chase";
		case BTDemoEnemy::ePhase::Attack: return "Attack";
		default: break;
		}

		return "Idle";
	}

}

BTDemoEnemy::BTDemoEnemy()
{
	BuildTree();
}

BTDemoEnemy::~BTDemoEnemy() = default;

void BTDemoEnemy::Update(float DeltaTime)
{
	m_DeltaTime = DeltaTime;

	m_upRoot->Tick();
}

void BTDemoEnemy::BuildTree()
{
	m_upRoot = std::make_unique<RootNode>();

	// 優先度順: 攻撃 → 追跡 → 待機(FSMの遷移規約と同じ優先度).
	RootNode& Root = *static_cast<RootNode*>(m_upRoot.get());

	SelectorNode& Selector = Root.AddChild<SelectorNode>();

	// Attackブランチ: 攻撃範囲内なら攻撃へ. 攻撃中はActionがRunningを返し
	// Selectorがその子から再開するため、範囲外へ離れても中断されない.
	SequenceNode& SeqAttack = Selector.AddChild<SequenceNode>();
	SeqAttack.AddChild<ActionNode>([this] { return CondInAttackRange(); });
	SeqAttack.AddChild<ActionNode>([this] { return TickAttack(); });

	// Chaseブランチ: 未追跡なら索敵範囲、追跡中はロスト範囲を満たす間追跡する.
	// Chase自体は毎フレームSuccessで完了させ、ツリー全体を再評価させる
	// (FSMのChaseが毎フレーム攻撃範囲/ロスト範囲を再判定する挙動に合わせる).
	SequenceNode& SeqChase = Selector.AddChild<SequenceNode>();
	SeqChase.AddChild<ActionNode>([this] { return CondCanChase(); });
	SeqChase.AddChild<ActionNode>([this] { return TickChase(); });

	// Idleフォールバック: どのブランチも成立しなければ待機.
	Selector.AddChild<ActionNode>([this] { return TickIdle(); });
}

NodeStatus BTDemoEnemy::TickIdle()
{
	ChangePhase(ePhase::Idle);

	return NodeStatus::Success;
}

NodeStatus BTDemoEnemy::TickChase()
{
	ChangePhase(ePhase::Chase);

	RotateToTarget(AngleToTargetDeg(), CHASE_ROTATE_SPEED);

	// 向いている方向(Yaw)へそのまま前進する(Chaseステートと同じ移動).
	const float speed_and_delta = MOVE_SPEED * m_DeltaTime;
	m_Position.x += std::sinf(m_YawRad) * speed_and_delta;
	m_Position.z += std::cosf(m_YawRad) * speed_and_delta;

	return NodeStatus::Success;
}

NodeStatus BTDemoEnemy::TickAttack()
{
	if (m_Phase != ePhase::Attack)
	{
		ChangePhase(ePhase::Attack);

		m_AttackElapsed = 0.0f;
	}

	m_AttackElapsed += m_DeltaTime;

	RotateToTarget(AngleToTargetDeg(), ATTACK_ROTATE_SPEED);

	// 完了したらFSMのAttackと同じく、ロスト範囲内ならChase、範囲外ならIdleへ復帰する.
	if (m_AttackElapsed >= ATTACK_TOTAL_TIME)
	{
		ChangePhase(DistanceToTargetXZ() <= LOSE_RANGE ? ePhase::Chase : ePhase::Idle);

		return NodeStatus::Success;
	}

	return NodeStatus::Running;
}

NodeStatus BTDemoEnemy::CondInAttackRange()
{
	return DistanceToTargetXZ() <= ATTACK_RANGE ? NodeStatus::Success : NodeStatus::Failure;
}

NodeStatus BTDemoEnemy::CondCanChase()
{
	// FSMはIdleからの再索敵が索敵範囲、Chase継続判定がロスト範囲という
	// ヒステリシスを持つため、それを現在フェーズ込みの条件で再現する.
	const float distance = DistanceToTargetXZ();

	if (distance <= AGGRO_RANGE) { return NodeStatus::Success; }
	if (m_Phase == ePhase::Chase && distance <= LOSE_RANGE) { return NodeStatus::Success; }

	return NodeStatus::Failure;
}

float BTDemoEnemy::DistanceToTargetXZ() const noexcept
{
	const float dx = m_TargetPos.x - m_Position.x;
	const float dz = m_TargetPos.z - m_Position.z;

	return std::sqrtf(dx * dx + dz * dz);
}

float BTDemoEnemy::AngleToTargetDeg() const noexcept
{
	const float dx = m_TargetPos.x - m_Position.x;
	const float dz = m_TargetPos.z - m_Position.z;

	// atan2f(x, z): 0度=+Z前方(EnemyStateBaseと同じ規約).
	return std::atan2f(dx, dz) * (180.0f / DirectX::XM_PI);
}

void BTDemoEnemy::RotateToTarget(float TargetAngleDeg, float SpeedDegPerSec) noexcept
{
	const float target_rad = DirectX::XMConvertToRadians(TargetAngleDeg);

	// 角度差を[-π, π]へ正規化し、最短経路で回転する(GameObject::RotateToTargetと同じ).
	float diff_rad = std::fmodf(target_rad - m_YawRad + DirectX::XM_PI, DirectX::XM_2PI);
	if (diff_rad < 0.0f) { diff_rad += DirectX::XM_2PI; }
	diff_rad -= DirectX::XM_PI;

	const float max_step_rad = DirectX::XMConvertToRadians(SpeedDegPerSec) * m_DeltaTime;

	if (std::fabs(diff_rad) <= max_step_rad) {
		m_YawRad = target_rad;
	}
	else {
		m_YawRad += (diff_rad > 0.0f ? max_step_rad : -max_step_rad);
	}
}

void BTDemoEnemy::ChangePhase(ePhase NextPhase)
{
	if (m_Phase == NextPhase) { return; }

	OutputPhaseLog(PhaseName(m_Phase), PhaseName(NextPhase));

	m_Phase = NextPhase;
}
