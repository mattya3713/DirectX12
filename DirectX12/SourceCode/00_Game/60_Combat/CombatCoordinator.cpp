#include "CombatCoordinator.h"

#include <cmath>
#include <vector>

#include "00_Game/00_GameLoop/Time/Time.h"
#include "00_Game/30_Camera/50_Keyframe/KeyframeCamera.h"
#include "00_Game/30_Camera/99_Manager/CameraManager.h"
#include "00_Game/60_Combat/CombatTuning.h"
#include "99_Utility/ServiceLocator/ServiceLocator.h"

namespace {
	constexpr float PARRY_REACTION_DISTANCE = 2.5f;  // パリィ成立後、PlayerがBossの正面に収まる距離.
	constexpr float PARRY_REACTION_DURATION = 0.35f; // 位置合わせにかける時間(秒).

	// パリィ演出カメラ(Boss-Player中点を側面から見る構図で、寄りながら再生する).
	constexpr float PARRY_CAMERA_WIDE_DISTANCE = 3.5f;
	constexpr float PARRY_CAMERA_PUSH_DISTANCE = 2.0f;
	constexpr float PARRY_CAMERA_HEIGHT        = 1.6f;
	constexpr float PARRY_CAMERA_FOV_RAD       = DirectX::XMConvertToRadians(35.0f);
	constexpr float PARRY_CAMERA_DURATION      = 0.6f;
}

void CombatCoordinator::Initialize(const PlayerCombatView& PlayerView, const BossCombatView& BossView) noexcept
{
	m_PlayerView = PlayerView;
	m_BossView   = BossView;
}

void CombatCoordinator::Clear() noexcept
{
	m_PlayerView.reset();
	m_BossView.reset();
}

void CombatCoordinator::OnParrySuccess() noexcept
{
	if (!m_PlayerView || !m_BossView) { return; }

	// パリィ成立スローモーション(グローバル時間スケール. 専用カメラ演出も自動的にスローで進む).
	GameTime::SetTimeScale(CombatTuning::Get().ParrySlowScale, CombatTuning::Get().ParrySlowDuration);

	const DirectX::XMFLOAT3 player_pos = m_PlayerView->GetPosition();
	const DirectX::XMFLOAT3 boss_pos   = m_BossView->GetPosition();

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

	m_BossView->EnterParryReaction(boss_pos, boss_to_player_yaw_deg, PARRY_REACTION_DURATION);
	m_PlayerView->EnterParryReaction(player_target_pos, player_to_boss_yaw_deg, PARRY_REACTION_DURATION);

	if (CameraManager* p_camera_manager = ServiceLocator::Get<CameraManager>()) {
		// Boss→Player方向に対して垂直な向き(真後ろからではなく側面からの構図にするため).
		const float side_x = -nz;
		const float side_z = nx;

		const DirectX::XMFLOAT3 look_at = {
			(boss_pos.x + player_target_pos.x) * 0.5f,
			boss_pos.y + PARRY_CAMERA_HEIGHT * 0.5f,
			(boss_pos.z + player_target_pos.z) * 0.5f
		};

		const DirectX::XMFLOAT3 wide_shot_pos = {
			look_at.x + side_x * PARRY_CAMERA_WIDE_DISTANCE,
			look_at.y + PARRY_CAMERA_HEIGHT,
			look_at.z + side_z * PARRY_CAMERA_WIDE_DISTANCE
		};

		const DirectX::XMFLOAT3 push_in_pos = {
			look_at.x + side_x * PARRY_CAMERA_PUSH_DISTANCE,
			look_at.y + PARRY_CAMERA_HEIGHT * 0.5f,
			look_at.z + side_z * PARRY_CAMERA_PUSH_DISTANCE
		};

		std::vector<CameraKeyframe> keyframes = {
			{ wide_shot_pos, look_at, PARRY_CAMERA_FOV_RAD, 0.0f,                  MyEasing::Type::Liner    },
			{ push_in_pos,   look_at, PARRY_CAMERA_FOV_RAD, PARRY_CAMERA_DURATION, MyEasing::Type::OutCubic },
		};

		p_camera_manager->PlayOneShot("ParryReaction", std::move(keyframes));
	}
}
