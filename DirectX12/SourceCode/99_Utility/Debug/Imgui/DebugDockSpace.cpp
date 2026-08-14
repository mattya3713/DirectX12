#include "DebugDockSpace.h"

#include "99_Utility\Debug\Imgui\ImGuiManager.h"
#include "ImGui/imgui_internal.h"

void DebugDockSpace::Draw()
{
	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);
	ImGui::SetNextWindowViewport(viewport->ID);

	ImGuiWindowFlags host_flags =
		ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
		ImGuiWindowFlags_NoBackground;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin("DockSpaceHost", nullptr, host_flags);
	ImGui::PopStyleVar(3);

	const ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");

	// 保存済みレイアウトがない初回だけ、各デバッグウィンドウの標準配置を構築する.
	if (ImGui::DockBuilderGetNode(dockspace_id) == nullptr)
	{
		ImGui::DockBuilderRemoveNode(dockspace_id);
		ImGui::DockBuilderAddNode(
			dockspace_id,
			ImGuiDockNodeFlags_DockSpace | ImGuiDockNodeFlags_PassthruCentralNode);
		ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->WorkSize);

		// Unityの既定レイアウト(Hierarchy=左、Inspector=右、Project/Console=下段が画面幅いっぱい、
		// 中央がScene/Game view)に合わせ、まず下段を画面幅いっぱいに切り出してから左右を分割する.
		ImGuiID top_area = dockspace_id;
		ImGuiID bottom   = ImGui::DockBuilderSplitNode(top_area, ImGuiDir_Down, 0.25f, nullptr, &top_area);

		ImGuiID center = top_area;
		ImGuiID left   = ImGui::DockBuilderSplitNode(center, ImGuiDir_Left,  0.18f, nullptr, &center);
		ImGuiID right  = ImGui::DockBuilderSplitNode(center, ImGuiDir_Right, 0.22f, nullptr, &center);
		ImGuiID left_top    = ImGui::DockBuilderSplitNode(left, ImGuiDir_Up, 0.35f, nullptr, &left);
		ImGuiID right_top   = ImGui::DockBuilderSplitNode(right, ImGuiDir_Up, 0.50f, nullptr, &right);
		ImGuiID bottom_left = ImGui::DockBuilderSplitNode(bottom, ImGuiDir_Left, 0.50f, nullptr, &bottom);

		ImGui::DockBuilderDockWindow("Debug HUD", left_top);
		ImGui::DockBuilderDockWindow("Scene", left);
		ImGui::DockBuilderDockWindow("Animation Editor", right_top);
		ImGui::DockBuilderDockWindow("Actor Scale (Debug)", right);
		ImGui::DockBuilderDockWindow("Console", bottom_left);
		ImGui::DockBuilderDockWindow("Model Select", bottom);
		ImGui::DockBuilderDockWindow("Scene View", center);

		ImGui::DockBuilderFinish(dockspace_id);
	}

	ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);
	ImGui::End();
}
