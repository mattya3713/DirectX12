#include "AnimationEditor.h"

#include "99_Utility/Debug/Imgui/ImGuiManager.h"
#include "10_Ggraphic/PMX/PMXActor.h"

AnimationEditor::AnimationEditor()
{
	// 初回起動時は存在しなくてよい(失敗を無視). imgui.rulと同様、ビルド成果物の一部として
	// 直接読み書きする実行時生成データのため、ProjectDir側のData\とは別物としてOutDir側にのみ存在する.
	m_ClipTable.Load(AnimationClipTable::DEFAULT_FILE_PATH);
}

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

	// 名前付きクリップとしての保存・読込.
	ImGuiManager::Input("Clip Name", m_ClipName);

	if (ImGui::Button("Save Clip"))
	{
		m_ClipTable.Set(m_ClipName, AnimationClipData{ start_frame, end_frame, speed });
		m_ClipTable.Save(AnimationClipTable::DEFAULT_FILE_PATH);
	}

	ImGui::SameLine();

	if (ImGui::Button("Load Clip"))
	{
		if (const AnimationClipData* p_clip = m_ClipTable.Find(m_ClipName))
		{
			Actor.SetPlaybackRange(p_clip->StartFrame, p_clip->EndFrame);
			Actor.SetAnimationSpeed(p_clip->Speed);
		}
	}

	ImGui::Separator();

	if (ImGui::Button("Step 1 Frame"))
	{
		step_requested = true;
	}

	ImGui::End();

	return step_requested;
}
