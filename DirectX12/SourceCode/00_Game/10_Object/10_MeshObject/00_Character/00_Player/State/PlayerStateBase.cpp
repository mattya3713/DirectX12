#include "PlayerStateBase.h"

#include <cmath>

#include "00_Game/10_Object/10_MeshObject/00_Character/00_Player/Player.h"
#include "00_Game/50_Input/VirtualPad.h"
#include "99_Utility/ServiceLocator/ServiceLocator.h"

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
	m_pOwner->PlayNamedClip(ClipName);
}

bool PlayerStateBase::TryStartCombatAction() const
{
	VirtualPad* p_pad = ServiceLocator::Get<VirtualPad>();
	if (!p_pad) { return false; }

	// 【仮実装】必殺技トリガー: 必殺ゲージMAX中にSpecialAttackで発動する.
	// 正式な必殺技システム(入力・発動条件)は別Featureで上書きされる前提.
	if (p_pad->IsActionPress(VirtualPad::eGameAction::SpecialAttack) &&
		GetPlayer()->GetCurrentUltValue() >= GetPlayer()->GetMaxUltValue())
	{
		GetPlayer()->ChangeState(PlayerState::eID::SpecialMove);
		return true;
	}

	if (p_pad->IsActionPress(VirtualPad::eGameAction::Attack))
	{
		GetPlayer()->ChangeState(PlayerState::eID::AttackCombo_0);
		return true;
	}

	if (p_pad->IsActionDown(VirtualPad::eGameAction::Dodge))
	{
		GetPlayer()->ChangeState(PlayerState::eID::DodgeExecute);
		return true;
	}

	if (p_pad->IsActionDown(VirtualPad::eGameAction::Parry))
	{
		GetPlayer()->ChangeState(PlayerState::eID::Parry);
		return true;
	}

	return false;
}
