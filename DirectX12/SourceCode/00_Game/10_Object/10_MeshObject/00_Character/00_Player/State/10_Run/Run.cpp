#include "Run.h"

#include <cmath>

#include "00_Game/10_Object/10_MeshObject/00_Character/00_Player/Player.h"
#include "00_Game/30_Camera/00_Base/CameraBase.h"
#include "00_Game/30_Camera/99_Manager/CameraManager.h"
#include "00_Game/50_Input/VirtualPad.h"
#include "00_Game/00_GameLoop/Time/Time.h"
#include "99_Utility/Debug/Log/DebugLog.h"
#include "99_Utility/ServiceLocator/ServiceLocator.h"

namespace {
	constexpr float INPUT_EPSILON_SQ = 1e-4f; // 入力を「無し」とみなす閾値(2乗).

#if _DEBUG
	// カメラ相対移動の診断ログ間隔(フレーム. ログ量を抑えるため).
	constexpr int kMoveDebugLogInterval = 30;
#endif
}

namespace PlayerState {

Run::Run(Player* pOwner) noexcept
	: PlayerStateBase(pOwner)
{
}

void Run::Enter()
{
	ApplyNamedClip("player_run");
}

void Run::Update()
{
	if (TryStartCombatAction()) { return; }

	CalculateMoveVec();
}

void Run::LateUpdate()
{
	PlayerStateBase::LateUpdate(); // 移動方向へ向きをラープ回転させる.

	GetPlayer()->AddPosition(GetPlayer()->GetMoveVec());
}

void Run::CalculateMoveVec()
{
	VirtualPad* p_pad = ServiceLocator::Get<VirtualPad>();
	if (!p_pad) { return; }

	const DirectX::XMFLOAT2 input_vec = p_pad->GetAxisInput(VirtualPad::eGameAxisAction::Move);

	// 入力が無くなったら待機へ遷移.
	if (input_vec.x * input_vec.x + input_vec.y * input_vec.y <= INPUT_EPSILON_SQ)
	{
		GetPlayer()->ChangeState(PlayerState::eID::Idle);
		return;
	}

	CameraManager* p_camera_manager = ServiceLocator::Get<CameraManager>();
	CameraBase* p_active_camera = p_camera_manager ? p_camera_manager->GetActive() : nullptr;
	if (!p_active_camera) { return; }

	// カメラの前・右方向をXZ平面へ投影して正規化(Y成分を移動に含めないため).
	DirectX::XMFLOAT3 camera_forward, camera_right;
	p_active_camera->GetBasis(camera_forward, camera_right);
	DirectX::XMVECTOR v_forward = DirectX::XMLoadFloat3(&camera_forward);
	DirectX::XMVECTOR v_right   = DirectX::XMLoadFloat3(&camera_right);
	v_forward = DirectX::XMVector3Normalize(DirectX::XMVectorSetY(v_forward, 0.0f));
	v_right   = DirectX::XMVector3Normalize(DirectX::XMVectorSetY(v_right, 0.0f));

	// XZ投影後にゼロ長・非有限になった場合(真上/真下を向く等)は移動を停止して安全化.
	const float forward_len_sq = DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(v_forward));
	const float right_len_sq   = DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(v_right));
	const bool basis_is_valid =
		forward_len_sq > 1e-8f && right_len_sq > 1e-8f &&
		std::isfinite(forward_len_sq) && std::isfinite(right_len_sq) &&
		std::isfinite(camera_forward.x) && std::isfinite(camera_forward.z) &&
		std::isfinite(camera_right.x) && std::isfinite(camera_right.z);

	if (!basis_is_valid)
	{
		// カメラ基準が使えない場合は移動を止める(ワールド固定へフォールバックしない).
		GetPlayer()->SetMoveVec({}, PlayerAccess::MovementKey{});
		return;
	}

	// 入力とカメラ方向を合成.
	DirectX::XMVECTOR v_move = DirectX::XMVectorAdd(
		DirectX::XMVectorScale(v_forward, input_vec.y),
		DirectX::XMVectorScale(v_right, input_vec.x));

	const float speed_and_delta = GetPlayer()->GetRunMoveSpeed() * GameTime::GetDeltaTime();
	v_move = DirectX::XMVectorScale(v_move, speed_and_delta);

	DirectX::XMFLOAT3 move_vec = {};
	DirectX::XMStoreFloat3(&move_vec, v_move);

#if _DEBUG
	// カメラ相対移動の診断ログ(ActiveカメラのBasis→入力→MoveVecの因果確認用).
	static int s_debug_log_frame = 0;
	if (++s_debug_log_frame % kMoveDebugLogInterval == 0)
	{
		if (DebugLog* p_debug_log = ServiceLocator::Get<DebugLog>()) {
			p_debug_log->LogInfo(
				"MoveDebug camYaw=" + std::to_string(p_active_camera->GetYaw() * 57.2958f) +
				"deg camFwdZ=" + std::to_string(camera_forward.z) +
				" in=(" + std::to_string(input_vec.x) + "," + std::to_string(input_vec.y) + ")" +
				" move=(" + std::to_string(move_vec.x) + "," + std::to_string(move_vec.z) + ")");
		}
	}
#endif

	GetPlayer()->SetMoveVec(move_vec, PlayerAccess::MovementKey{});
}

} // namespace PlayerState
