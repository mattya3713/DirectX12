#include "SceneView.h"

#include "10_Ggraphic/DirectX/DirectX12.h"
#include "99_Utility/Debug/Imgui/ImGuiManager.h"
#include "99_Utility/ServiceLocator/ServiceLocator.h"

namespace
{
	ImVec2 s_ContentSize = { 0.0f, 0.0f };
}

void SceneView::Draw()
{
	ImGuiManager* p_imgui_manager = ServiceLocator::Get<ImGuiManager>();
	if (!p_imgui_manager) { return; }

	ImGui::SetNextWindowSize(ImVec2(640.0f, 360.0f), ImGuiCond_FirstUseEver);
	ImGui::Begin("Scene View");

	const D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle = p_imgui_manager->GetSceneTextureGpuHandle();
	const ImTextureID texture_id = reinterpret_cast<ImTextureID>(gpu_handle.ptr);

	constexpr float TARGET_ASPECT = 16.0f / 9.0f;
	const ImVec2 available = ImGui::GetContentRegionAvail();
	if (available.x > 0.0f && available.y > 0.0f)
	{
		const float panel_aspect = available.x / available.y;
		ImVec2 fit_size = available;
		if (panel_aspect > TARGET_ASPECT)
		{
			fit_size.y = available.y;
			fit_size.x = available.y * TARGET_ASPECT;
		}
		else
		{
			fit_size.x = available.x;
			fit_size.y = available.x / TARGET_ASPECT;
		}

		const ImVec2 offset(
			(available.x - fit_size.x) * 0.5f,
			(available.y - fit_size.y) * 0.5f);

		DirectX12* p_dx12 = ServiceLocator::Get<DirectX12>();
		if (p_dx12)
		{
			p_dx12->RequestSceneColorResize(
				static_cast<UINT>(fit_size.x), static_cast<UINT>(fit_size.y));
		}

		s_ContentSize = fit_size;

		constexpr ImU32 LETTERBOX_COLOR = IM_COL32(60, 60, 60, 255);
		const ImVec2 window_pos = ImGui::GetCursorScreenPos();
		ImGui::GetWindowDrawList()->AddRectFilled(
			window_pos,
			ImVec2(window_pos.x + available.x, window_pos.y + available.y),
			LETTERBOX_COLOR);

		const ImVec2 cursor_start = ImGui::GetCursorPos();
		ImGui::SetCursorPos(ImVec2(cursor_start.x + offset.x, cursor_start.y + offset.y));
		ImGui::Image(texture_id, fit_size);
	}

	ImGui::End();
}

ImVec2 SceneView::GetContentSize() noexcept
{
	return s_ContentSize;
}
