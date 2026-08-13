#include "CombatCoordinator.h"

#include <cmath>

#include "00_Game/10_Object/10_MeshObject/00_Character/00_Player/Player.h"
#include "00_Game/10_Object/10_MeshObject/00_Character/00_Player/PlayerAccessKeys.h"
#include "00_Game/10_Object/10_MeshObject/00_Character/20_Boss/Boss.h"

namespace {
	constexpr float PARRY_REACTION_DISTANCE = 2.5f;  // パリィ成立後、PlayerがBossの正面に収まる距離.
	constexpr float PARRY_REACTION_DURATION = 0.35f; // 位置合わせにかける時間(秒).
}

void CombatCoordinator::Initialize(Player* pPlayer, Boss* pBoss) noexcept
{
	m_pPlayer = pPlayer;
	m_pBoss   = pBoss;
}

void CombatCoordinator::Clear() noexcept
{
	m_pPlayer = nullptr;
	m_pBoss   = nullptr;
}

void CombatCoordinator::OnParrySuccess() noexcept
{
	if (!m_pPlayer || !m_pBoss) { return; }

	const DirectX::XMFLOAT3 player_pos = m_pPlayer->GetPosition();
	const DirectX::XMFLOAT3 boss_pos   = m_pBoss->GetPosition();

	const float dx = player_pos.x - boss_pos.x;
	const float dz = player_pos.z - boss_pos.z;
	const float distance = std::sqrtf(dx * dx + dz * dz);

	// Bossから見たPlayer方向の単位ベクトル(水平距離がほぼ0の異常値だけ前方固定で回避する).
	const float nx = (distance > 0.0001f) ? dx / distance : 0.0f;
	const float nz = (distance > 0.0001f) ? dz / distance : 1.0f;

	// Playerの新しい位置 = Bossから見たPlayer方向へPARRY_REACTION_DISTANCEだけ進んだ位置
	// (Bossは自分の位置に留まり、向きだけPlayer方向へ合わせる).
	const DirectX::XMFLOAT3 player_target_pos = {
		boss_pos.x + nx * PARRY_REACTION_DISTANCE,
		player_pos.y,
		boss_pos.z + nz * PARRY_REACTION_DISTANCE
	};

	// atan2f(x, z): 0度 = +Z軸方向(前方)を正面とする(AngleToTargetDeg系と同じ規約).
	const float boss_to_player_yaw_deg = std::atan2f(nx, nz) * (180.0f / DirectX::XM_PI);
	const float player_to_boss_yaw_deg = std::atan2f(-nx, -nz) * (180.0f / DirectX::XM_PI);

	m_pBoss->EnterParryReaction(boss_pos, boss_to_player_yaw_deg, PARRY_REACTION_DURATION);
	m_pPlayer->SetParryReactionTarget(player_target_pos, player_to_boss_yaw_deg, PARRY_REACTION_DURATION, PlayerAccess::CombatCoordinatorKey{});
}
