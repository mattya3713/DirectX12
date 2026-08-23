#include "Combat.h"

#include <algorithm>
#include <cmath>

#include "00_Game/10_Object/10_MeshObject/00_Character/00_Player/Player.h"
#include "00_Game/10_Object/10_MeshObject/00_Character/20_Boss/Boss.h"
#include "00_Game/30_Camera/00_Base/CameraBase.h"
#include "00_Game/30_Camera/99_Manager/CameraManager.h"
#include "00_Game/50_Input/VirtualPad.h"
#include "00_Game/00_GameLoop/Time/Time.h"
#include "99_Utility/FileManager/FileManager.h"
#include "99_Utility/ServiceLocator/ServiceLocator.h"

namespace {
	// 突進の合計距離(攻撃時間全体で詰める量. 仮値. 演出バランスは後で調整する).
	constexpr float kRushDistance = 2.5f;
	// コンボ数1あたりの再生速度上昇量(倍率).
	constexpr float kSpeedPerCombo = 0.01f;
	// 再生速度上昇の上限(+20%. 青天井にしないための制限).
	constexpr float kMaxSpeedBonus = 0.2f;
	// 入力を「有り」とみなす閾値(2乗).
	constexpr float kInputEpsilonSq = 1e-4f;
}

namespace PlayerState {

Combat::Combat(Player* pOwner) noexcept
	: PlayerStateBase(pOwner)
{
}

void Combat::Enter()
{
	m_CurrentTime     = 0.0f;
	m_IsComboAccepted = false;
	m_ColliderWindows.clear();
	m_IsRushEnabled   = false;

	GetPlayer()->SetAttackColliderActive(false);

	LoadSettings();
}

void Combat::LateUpdate()
{
	// 移動方向へ向きをラープ回転させてから、突進移動を行う.
	PlayerStateBase::LateUpdate();
	ProcessRushMovement();
}

void Combat::Update()
{
	m_CurrentTime = GetPlayer()->GetCurrentAnimationSeconds();
	ProcessColliderWindows();
}

void Combat::Exit()
{
	for (ColliderWindow& window : m_ColliderWindows)
	{
		window.IsAct = false;
		window.IsEnd = false;
	}

	GetPlayer()->SetAttackColliderActive(false);

	GetPlayer()->SetAnimPlaybackSpeed(1.0f); // 再生速度を等速へ戻す(次Stateへの持ち越し防止).
}

void Combat::LoadSettings()
{
	const std::string file_name = GetSettingsFileName();
	if (file_name.empty()) { return; }

	const nlohmann::json data = FileManager::JsonLoad(file_name);
	if (data.empty()) { return; }

	m_ComboStartTime    = data.value("ComboStartTime", m_ComboStartTime);
	m_MinComboTransTime = data.value("MinComboTransTime", m_MinComboTransTime);
	m_ComboEndTime      = data.value("ComboEndTime", m_ComboEndTime);

	if (data.contains("ColliderWindows"))
	{
		for (const nlohmann::json& window : data["ColliderWindows"])
		{
			AddColliderWindow(window.value("start", 0.0f), window.value("duration", 0.1f));
		}
	}
}

void Combat::AddColliderWindow(float Start, float Duration)
{
	m_ColliderWindows.push_back(ColliderWindow{ Start, Duration, false, false });
}

void Combat::ProcessColliderWindows()
{
	for (ColliderWindow& window : m_ColliderWindows)
	{
		if (window.IsEnd) { continue; }

		if (window.IsAct)
		{
			if (m_CurrentTime >= window.Start + window.Duration)
			{
				GetPlayer()->SetAttackColliderActive(false);
				window.IsAct = false;
				window.IsEnd = true;
			}
		}
		else if (m_CurrentTime >= window.Start)
		{
			GetPlayer()->SetAttackColliderActive(true);
			window.IsAct = true;
		}
	}
}

bool Combat::UpdateComboInput()
{
	VirtualPad* p_pad = ServiceLocator::Get<VirtualPad>();
	if (p_pad && !m_IsComboAccepted
		&& m_CurrentTime >= m_ComboStartTime && m_CurrentTime <= m_ComboEndTime
		&& p_pad->IsActionPress(VirtualPad::eGameAction::Attack))
	{
		m_IsComboAccepted = true;
	}

	return m_IsComboAccepted && m_CurrentTime >= m_MinComboTransTime;
}

void Combat::ApplyComboSpeedToAnimation()
{
	// 勢い制: コンボ数が乗るほど攻撃モーションが速くなる(上限付き).
	const float speed = 1.0f + std::min(GetPlayer()->GetCombo() * kSpeedPerCombo, kMaxSpeedBonus);
	GetPlayer()->SetAnimPlaybackSpeed(speed);
}

void Combat::DecideRushDirection(bool AllowInputRedirect)
{
	m_IsRushEnabled = false;

	DirectX::XMVECTOR v_dir = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	bool is_decided = false;

	// 移動入力をカメラ基準のワールド方向へ変換する(3段目の方向転換).
	if (AllowInputRedirect)
	{
		if (VirtualPad* p_pad = ServiceLocator::Get<VirtualPad>())
		{
			const DirectX::XMFLOAT2 input_vec = p_pad->GetAxisInput(VirtualPad::eGameAxisAction::Move);
			if (input_vec.x * input_vec.x + input_vec.y * input_vec.y > kInputEpsilonSq)
			{
				if (CameraManager* p_camera_manager = ServiceLocator::Get<CameraManager>())
				{
					if (CameraBase* p_camera = p_camera_manager->GetActive())
					{
						const DirectX::XMFLOAT3 camera_forward = p_camera->GetForward();
						const DirectX::XMFLOAT3 camera_right   = p_camera->GetRight();
						DirectX::XMVECTOR v_forward = DirectX::XMLoadFloat3(&camera_forward);
						DirectX::XMVECTOR v_right   = DirectX::XMLoadFloat3(&camera_right);
						v_forward = DirectX::XMVector3Normalize(DirectX::XMVectorSetY(v_forward, 0.0f));
						v_right   = DirectX::XMVector3Normalize(DirectX::XMVectorSetY(v_right, 0.0f));

						v_dir = DirectX::XMVectorAdd(
							DirectX::XMVectorScale(v_forward, input_vec.y),
							DirectX::XMVectorScale(v_right, input_vec.x));
						is_decided = true;
					}
				}
			}
		}
	}

	// 入力が無ければBoss方向へ向ける.
	if (!is_decided)
	{
		if (Boss* p_boss = ServiceLocator::Get<Boss>())
		{
			const DirectX::XMFLOAT3 my_pos     = GetPlayer()->GetPosition();
			const DirectX::XMFLOAT3 target_pos = p_boss->GetPosition();
			const DirectX::XMFLOAT3 to_target  = { target_pos.x - my_pos.x, 0.0f, target_pos.z - my_pos.z };
			const float length_sq = to_target.x * to_target.x + to_target.z * to_target.z;

			if (length_sq > kInputEpsilonSq)
			{
				const float length = std::sqrt(length_sq);
				v_dir = DirectX::XMVectorSet(to_target.x / length, 0.0f, to_target.z / length, 0.0f);
				is_decided = true;
			}
		}
	}

	if (is_decided)
	{
		DirectX::XMFLOAT3 dir{};
		DirectX::XMStoreFloat3(&dir, DirectX::XMVector3Normalize(v_dir));
		m_RushDirection = dir;
		m_IsRushEnabled = true;

		// 攻撃の向きも突進方向へ合わせる(MoveVec基準のラープ回転を流用. 移動はしない).
		GetPlayer()->SetMoveVec({ dir.x * 0.01f, 0.0f, dir.z * 0.01f }, PlayerAccess::MovementKey{});
	}
}

void Combat::ProcessRushMovement()
{
	if (m_IsRushEnabled == false) { return; }
	if (m_CurrentTime >= m_ComboEndTime) { return; } // 攻撃時間中のみ詰める.

	const float duration  = (m_ComboEndTime > 0.01f) ? m_ComboEndTime : 0.01f;
	const float rush_speed = kRushDistance / duration;
	const float delta_time = GameTime::GetDeltaTime();

	GetPlayer()->AddPosition({ m_RushDirection.x * rush_speed * delta_time,
		0.0f,
		m_RushDirection.z * rush_speed * delta_time });
}

} // namespace PlayerState
