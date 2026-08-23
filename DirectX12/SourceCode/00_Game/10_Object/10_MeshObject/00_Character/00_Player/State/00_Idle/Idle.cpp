#include "Idle.h"

#include "00_Game/10_Object/10_MeshObject/00_Character/00_Player/Player.h"
#include "00_Game/50_Input/VirtualPad.h"
#include "99_Utility/ServiceLocator/ServiceLocator.h"

namespace {
	constexpr float INPUT_EPSILON_SQ = 1e-4f; // 入力を「無し」とみなす閾値(2乗).
}

namespace PlayerState {

Idle::Idle(Player* pOwner) noexcept
	: PlayerStateBase(pOwner)
{
}

void Idle::Enter()
{
	// 待機状態では移動ベクトルをクリアする(攻撃後などに向きが残らないように).
	GetPlayer()->SetMoveVec({}, PlayerAccess::MovementKey{});

	// アイドル用クリップがモデルに存在しないため、現在フレームでポーズを固定する
	// (SetCurrentFrame経由で外部駆動状態にし、次のState遷移時にクリップ再指定で復帰する).
	GetPlayer()->SetCurrentFrame(GetPlayer()->GetCurrentAnimationSeconds() * 30.0f);
}

void Idle::Update()
{
	if (TryStartCombatAction()) { return; }

	VirtualPad* p_pad = ServiceLocator::Get<VirtualPad>();
	if (!p_pad) { return; }

	const DirectX::XMFLOAT2 input_vec = p_pad->GetAxisInput(VirtualPad::eGameAxisAction::Move);

	if (input_vec.x * input_vec.x + input_vec.y * input_vec.y > INPUT_EPSILON_SQ)
	{
		GetPlayer()->ChangeState(PlayerState::eID::Run);
	}
}

} // namespace PlayerState
