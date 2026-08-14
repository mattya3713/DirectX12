#include "SceneView.h"

#include "99_Utility/Debug/Imgui/ImGuiManager.h"
#include "99_Utility/ServiceLocator/ServiceLocator.h"

void SceneView::Draw()
{
	ImGuiManager* p_imgui_manager = ServiceLocator::Get<ImGuiManager>();
	if (!p_imgui_manager) { return; }

	ImGui::SetNextWindowSize(ImVec2(640.0f, 360.0f), ImGuiCond_FirstUseEver);
	ImGui::Begin("Scene View");

	const D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle = p_imgui_manager->GetSceneTextureGpuHandle();
	const ImTextureID texture_id = reinterpret_cast<ImTextureID>(gpu_handle.ptr);

	const ImVec2 available = ImGui::GetContentRegionAvail();
	ImGui::Image(texture_id, available);

	ImGui::End();
}
