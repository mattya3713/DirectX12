#include "AnimationEditor.h"

#include "99_Utility/Debug/Imgui/ImGuiManager.h"
#include "10_Ggraphic/PMX/PMXActor.h"

bool AnimationEditor::Draw(PMXActor& Actor)
{
	if (!m_IsActive) { return false; }

	bool step_requested = false;

	ImGui::Begin("Animation Editor", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

	ImGuiManager::Text("起動中: ゲームの更新を停止しています(F1で終了)");
	ImGui::Text("Frame: %.1f / %u", Actor.GetCurrentAnimationTime(), Actor.GetMaxFrame());

	ImGui::Separator();

	// 再生範囲・速度の調整.
	float start_frame = Actor.GetStartFrame();
	float end_frame   = Actor.GetEndFrame();
	float speed       = Actor.GetAnimationSpeed();

	ImGuiManager::Input("Start Frame", start_frame);
	ImGuiManager::Input("End Frame", end_frame);
	ImGuiManager::Tweak("Speed", speed, 0.0f, 60.0f);

	Actor.SetPlaybackRange(start_frame, end_frame);
	Actor.SetAnimationSpeed(speed);

	ImGui::Separator();

	if (ImGui::Button("Step 1 Frame"))
	{
		step_requested = true;
	}

	ImGui::End();

	return step_requested;
}
