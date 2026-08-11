#include "Dodge.h"

#include <cmath>

#include "00_Game/10_Object/10_MeshObject/00_Character/00_Player/Player.h"
#include "00_Game/30_Camera/00_Base/CameraBase.h"
#include "00_Game/30_Camera/99_Manager/CameraManager.h"
#include "00_Game/50_Input/VirtualPad.h"
#include "99_Utility/ServiceLocator/ServiceLocator.h"

namespace {
	constexpr float INPUT_EPSILON_SQ     = 1e-4f;    // 入力を「無し」とみなす閾値(2乗).
	constexpr float INSTANT_ROTATE_SPEED = 36000.0f; // 回避方向へ実質1フレームで向き直す.
}

namespace PlayerState {

Dodge::Dodge(Player* pOwner) noexcept
	: PlayerStateBase(pOwner)
{
}

void Dodge::Enter()
{
	m_CurrentTime = 0.0f;
	GetPlayer()->SetDamageColliderActive(false); // 回避中は無敵.

	VirtualPad* p_pad = ServiceLocator::Get<VirtualPad>();
	const DirectX::XMFLOAT2 input_vec = p_pad
		? p_pad->GetAxisInput(VirtualPad::eGameAxisAction::Move)
		: DirectX::XMFLOAT2{ 0.0f, 0.0f };

	DirectX::XMVECTOR v_move_dir;

	if (input_vec.x * input_vec.x + input_vec.y * input_vec.y <= INPUT_EPSILON_SQ)
	{
		// 移動入力が無ければ、現在の正面方向へ回避する.
		const float yaw = GetPlayer()->GetTransform().Rotation.y;
		v_move_dir = DirectX::XMVectorSet(std::sinf(yaw), 0.0f, std::cosf(yaw), 0.0f);
	}
	else
	{
		CameraManager* p_camera_manager = ServiceLocator::Get<CameraManager>();
		CameraBase* p_active_camera = p_camera_manager ? p_camera_manager->GetActive() : nullptr;

		if (p_active_camera)
		{
			// カメラの前・右方向をXZ平面へ投影して入力と合成する(Runの移動計算と同じ考え方).
			const DirectX::XMFLOAT3 camera_forward = p_active_camera->GetForward();
			const DirectX::XMFLOAT3 camera_right   = p_active_camera->GetRight();
			DirectX::XMVECTOR v_forward = DirectX::XMLoadFloat3(&camera_forward);
			DirectX::XMVECTOR v_right   = DirectX::XMLoadFloat3(&camera_right);
			v_forward = DirectX::XMVector3Normalize(DirectX::XMVectorSetY(v_forward, 0.0f));
			v_right   = DirectX::XMVector3Normalize(DirectX::XMVectorSetY(v_right, 0.0f));

			v_move_dir = DirectX::XMVectorAdd(
				DirectX::XMVectorScale(v_forward, input_vec.y),
				DirectX::XMVectorScale(v_right, input_vec.x));
		}
		else
		{
			v_move_dir = DirectX::XMVectorSet(input_vec.x, 0.0f, input_vec.y, 0.0f);
		}

		v_move_dir = DirectX::XMVector3Normalize(v_move_dir);
	}

	DirectX::XMFLOAT3 move_dir3 = {};
	DirectX::XMStoreFloat3(&move_dir3, v_move_dir);
	m_InputVec = { move_dir3.x, move_dir3.z };

	// 回避方向へ向きを合わせる(atan2f(x, z): 0度 = +Z軸方向を正面とする).
	const float target_angle_rad = std::atan2f(move_dir3.x, move_dir3.z);
	const float target_angle_deg = target_angle_rad * (180.0f / DirectX::XM_PI);
	GetPlayer()->RotateToTarget(target_angle_deg, INSTANT_ROTATE_SPEED);
}

void Dodge::Exit()
{
	GetPlayer()->SetDamageColliderActive(true);
}

} // namespace PlayerState
