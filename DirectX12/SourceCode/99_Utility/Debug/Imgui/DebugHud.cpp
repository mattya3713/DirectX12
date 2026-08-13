#include "DebugHud.h"

#include "99_Utility/Debug/Imgui/ImGuiManager.h"
#include "99_Utility/ServiceLocator/ServiceLocator.h"
#include "00_Game/00_GameLoop/Time/Time.h"
#include "00_Game/30_Camera/99_Manager/CameraManager.h"
#include "00_Game/30_Camera/00_Base/CameraBase.h"

void DebugHud::Draw()
{
	// 常に同じ初期位置に置き、他のデバッグウィンドウと重ならないようにする(初回起動時のみ).
	ImGui::SetNextWindowPos(ImVec2(20.0f, 20.0f), ImGuiCond_FirstUseEver);
	ImGui::Begin("Debug HUD", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

	// FPS・デルタタイム.
	const float delta_time = GameTime::GetDeltaTime();
	const float fps = (delta_time > 0.0f) ? (1.0f / delta_time) : 0.0f;

	ImGui::Text("FPS: %.1f", fps);
	ImGui::Text("Delta Time: %.3f ms", delta_time * 1000.0f);

	ImGui::Separator();

	// アクティブカメラの情報.
	CameraManager* p_camera_manager = ServiceLocator::Get<CameraManager>();
	CameraBase* p_active_camera = p_camera_manager ? p_camera_manager->GetActive() : nullptr;

	if (p_active_camera)
	{
		const DirectX::XMFLOAT3& position = p_active_camera->GetPosition();
		ImGui::Text("Position: (%.2f, %.2f, %.2f)", position.x, position.y, position.z);

		const DirectX::XMFLOAT3& look = p_active_camera->GetLook();
		ImGui::Text("Look: (%.2f, %.2f, %.2f)", look.x, look.y, look.z);

		float yaw = p_active_camera->GetYaw();
		ImGuiManager::Input("Yaw (rad)", yaw);
		p_active_camera->SetYaw(yaw);

		float pitch = p_active_camera->GetPitch();
		ImGuiManager::Input("Pitch (rad)", pitch);
		p_active_camera->SetPitch(pitch);

		float fov_y = p_active_camera->GetFovY();
		ImGuiManager::Tweak("FovY (rad)", fov_y, 0.1f, 3.0f);
		p_active_camera->SetFovY(fov_y);
	}
	else
	{
		ImGui::Text("No active camera.");
	}

	ImGui::End();
}
