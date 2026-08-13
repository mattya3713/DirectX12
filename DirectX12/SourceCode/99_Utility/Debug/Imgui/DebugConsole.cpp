#include "DebugConsole.h"

#include "99_Utility/Debug/Log/DebugLog.h"
#include "99_Utility/Debug/Imgui/ImGuiManager.h"
#include "99_Utility/ServiceLocator/ServiceLocator.h"

void DebugConsole::Draw()
{
	DebugLog* p_debug_log = ServiceLocator::Get<DebugLog>();
	if (!p_debug_log)
	{
		return;
	}

	// Debug HUDとSceneManagerの初期位置を避ける.
	ImGui::SetNextWindowPos(ImVec2(20.0f, 330.0f), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(500.0f, 300.0f), ImGuiCond_FirstUseEver);
	ImGui::Begin("Console");

	if (ImGui::Button("Clear"))
	{
		p_debug_log->Clear();
	}

	ImGui::Separator();

	ImGui::BeginChild("ConsoleScroll", ImVec2(0.0f, 0.0f), true);
	for (const LogEntry& entry : p_debug_log->GetEntries())
	{
		ImVec4 color;
		switch (entry.Level)
		{
			case LogLevel::Warning: color = ImVec4(1.0f, 0.85f, 0.2f, 1.0f); break;
			case LogLevel::Error:   color = ImVec4(1.0f, 0.3f, 0.3f, 1.0f); break;
			default:                color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); break;
		}
		ImGui::TextColored(color, "%s", entry.Message.c_str());
	}

	if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
	{
		ImGui::SetScrollHereY(1.0f);
	}
	ImGui::EndChild();

	ImGui::End();
}
