#include "PlayerStateBase.h"

#include <cmath>

#include "00_Game/10_Object/10_MeshObject/00_Character/00_Player/Player.h"
#include "10_Ggraphic/PMX/AnimationClipTable.h"

namespace {
	constexpr float FACING_ROTATE_SPEED = 720.0f; // 度/秒.
	constexpr float MOVE_VEC_EPSILON    = 1e-4f;  // 移動ベクトルの微小ノイズを無視する閾値.
}

PlayerStateBase::PlayerStateBase(Player* pOwner) noexcept
	: StateBase<Player>(pOwner)
{
}

void PlayerStateBase::LateUpdate()
{
	const DirectX::XMFLOAT3& move_vec = m_pOwner->GetMoveVec();

	// 移動していない(≒向きを持たない)場合は回転させない.
	if (std::fabs(move_vec.x) < MOVE_VEC_EPSILON && std::fabs(move_vec.z) < MOVE_VEC_EPSILON)
	{
		return;
	}

	// atan2f(x, z): 0度 = +Z軸方向(前方)を正面とする.
	const float target_angle_rad = std::atan2f(move_vec.x, move_vec.z);
	const float target_angle_deg = target_angle_rad * (180.0f / DirectX::XM_PI);

	m_pOwner->RotateToTarget(target_angle_deg, FACING_ROTATE_SPEED);
}

void PlayerStateBase::ApplyNamedClip(const char* ClipName) const
{
	AnimationClipTable clip_table;
	clip_table.Load(AnimationClipTable::DEFAULT_FILE_PATH);

	if (const AnimationClipData* p_clip = clip_table.Find(ClipName))
	{
		m_pOwner->ApplyAnimationClip(p_clip->StartFrame, p_clip->EndFrame, p_clip->Speed);
	}
}
