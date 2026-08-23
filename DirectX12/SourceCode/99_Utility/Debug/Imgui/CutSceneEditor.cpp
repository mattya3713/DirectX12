#include "stdafx.h"
#include "CutSceneEditor.h"

#include <algorithm>
#include <filesystem>

#include "ImGuiManager.h"
#include "00_Game/80_CutScene/CutScenePlayer.h"
#include "99_Utility/FileManager/FileManager.h"
#include "99_Utility/Math/Easing/Easing.h"
#include "99_Utility/ServiceLocator/ServiceLocator.h"

namespace {
	constexpr const char* kJsonDir = "Data/Json/CutScene";
	constexpr int       kEasingTypeCount = 31; // MyEasing::Typeの要素数(Liner〜InOutBounce).

	const char* TrackTypeName(eCutSceneTrackType Type) noexcept
	{
		switch (Type) {
		case eCutSceneTrackType::Camera:   return "Camera";
		case eCutSceneTrackType::SkinMesh: return "SkinMesh";
		case eCutSceneTrackType::Sound:    return "Sound";
		default: return "?";
		}
	}
}

void CutSceneEditor::Draw()
{
	if (!ImGui::Begin("Cut Scene Editor")) {
		ImGui::End();
		return;
	}

	// イベント名.
	char name_buffer[128] = {};
	m_Event.Name.copy(name_buffer, sizeof(name_buffer) - 1);
	if (ImGui::InputText(IMGUI_JP("イベント名"), name_buffer, sizeof(name_buffer))) {
		m_Event.Name = name_buffer;
	}

	ImGui::Text(IMGUI_JP("再生時間: %.2f秒"), m_Event.TotalDuration);

	// トラック一覧.
	ImGui::Separator();
	if (ImGui::BeginListBox("##Tracks", ImVec2(-1.0f, 100.0f))) {
		for (size_t i = 0; i < m_Event.Tracks.size(); ++i) {
			const CutSceneTrack& track = m_Event.Tracks[i];
			const bool is_selected = (static_cast<int>(i) == m_SelectedTrack);
			if (ImGui::Selectable((std::string(TrackTypeName(track.Type)) + ": " + track.Name).c_str(), is_selected)) {
				m_SelectedTrack = static_cast<int>(i);
				m_SelectedFrame = -1;
			}
		}
		ImGui::EndListBox();
	}

	// トラック追加.
	static int new_track_type = 0;
	ImGui::SetNextItemWidth(120.0f);
	ImGui::Combo("##NewTrackType", &new_track_type, "Camera\0SkinMesh\0Sound\0");
	ImGui::SameLine();
	if (ImGui::Button(IMGUI_JP("トラック追加"))) {
		CutSceneTrack track{};
		track.Type = static_cast<eCutSceneTrackType>(new_track_type);
		track.Name = TrackTypeName(track.Type);
		CutSceneKeyframe first{};
		first.Time     = 0.0f;
		first.Scale    = 1.0f;
		first.Position = { 0.0f, 1.0f, 0.0f };
		track.Keyframes.push_back(first);

		m_Event.Tracks.push_back(std::move(track));
		m_SelectedTrack = static_cast<int>(m_Event.Tracks.size()) - 1;
		m_SelectedFrame = -1;
	}
	ImGui::SameLine();
	if (m_SelectedTrack >= 0 && ImGui::Button(IMGUI_JP("トラック削除"))) {
		m_Event.Tracks.erase(m_Event.Tracks.begin() + m_SelectedTrack);
		m_SelectedTrack = -1;
		m_SelectedFrame = -1;
	}

	// 選択トラックのプロパティとキーフレーム.
	if (m_SelectedTrack >= 0 && m_SelectedTrack < static_cast<int>(m_Event.Tracks.size())) {
		DrawTrackProperties();
		DrawKeyframeList();
	}

	// 保存/読込/プレビュー.
	ImGui::Separator();
	if (ImGui::Button(IMGUI_JP("保存"))) {
		SaveToFile();
	}
	ImGui::SameLine();

	char load_buffer[128] = {};
	m_LoadName.copy(load_buffer, sizeof(load_buffer) - 1);
	ImGui::SetNextItemWidth(150.0f);
	if (ImGui::InputText(IMGUI_JP("読込名"), load_buffer, sizeof(load_buffer))) {
		m_LoadName = load_buffer;
	}
	ImGui::SameLine();
	if (ImGui::Button(IMGUI_JP("読込")) && !m_LoadName.empty()) {
		LoadFromFile(m_LoadName);
	}
	ImGui::SameLine();
	if (ImGui::Button(IMGUI_JP("プレビュー再生")) && !m_Event.Tracks.empty()) {
		if (CutScenePlayer* p_player = ServiceLocator::Get<CutScenePlayer>()) {
			p_player->PlayEvent(m_Event, nullptr);
		}
	}
	ImGui::SameLine();
	if (ImGui::Button(IMGUI_JP("停止"))) {
		if (CutScenePlayer* p_player = ServiceLocator::Get<CutScenePlayer>()) {
			p_player->Stop();
		}
	}

	ImGui::End();
}

void CutSceneEditor::DrawTrackProperties()
{
	CutSceneTrack& track = m_Event.Tracks[m_SelectedTrack];

	ImGui::Separator();
	ImGui::Text(IMGUI_JP("種別: %s"), TrackTypeName(track.Type));

	int target_mode = static_cast<int>(track.TargetMode);
	if (ImGui::Combo(IMGUI_JP("ターゲット"), &target_mode, "SpawnedInstance\0ExistingInstance\0")) {
		track.TargetMode = static_cast<eCutSceneTargetMode>(target_mode);
	}

	if (track.Type == eCutSceneTrackType::SkinMesh || track.Type == eCutSceneTrackType::Camera) {
		char key_buffer[64] = {};
		track.TargetKey.copy(key_buffer, sizeof(key_buffer) - 1);
		if (ImGui::InputText(IMGUI_JP("TargetKey(Player/Boss)"), key_buffer, sizeof(key_buffer))) {
			track.TargetKey = key_buffer;
		}
	}

	if (track.Type == eCutSceneTrackType::SkinMesh) {
		char clip_buffer[64] = {};
		track.AnimClipName.copy(clip_buffer, sizeof(clip_buffer) - 1);
		if (ImGui::InputText(IMGUI_JP("アニメクリップ"), clip_buffer, sizeof(clip_buffer))) {
			track.AnimClipName = clip_buffer;
		}
	}

	if (track.Type == eCutSceneTrackType::Sound) {
		char sound_buffer[64] = {};
		track.SoundName.copy(sound_buffer, sizeof(sound_buffer) - 1);
		if (ImGui::InputText(IMGUI_JP("SE名"), sound_buffer, sizeof(sound_buffer))) {
			track.SoundName = sound_buffer;
		}
		ImGui::Checkbox(IMGUI_JP("ループ"), &track.IsLoopSound);
	}
}

void CutSceneEditor::DrawKeyframeList()
{
	CutSceneTrack& track = m_Event.Tracks[m_SelectedTrack];

	ImGui::Separator();
	if (ImGui::Button(IMGUI_JP("キーフレーム追加"))) {
		CutSceneKeyframe frame{};
		frame.Time     = track.EndTime() + 1.0f;
		frame.Scale    = 1.0f;
		frame.Position = { 0.0f, 1.0f, 0.0f };
		track.Keyframes.push_back(frame);
		m_SelectedFrame = static_cast<int>(track.Keyframes.size()) - 1;
	}
	ImGui::SameLine();
	if (m_SelectedFrame >= 0 && ImGui::Button(IMGUI_JP("キーフレーム削除"))) {
		track.Keyframes.erase(track.Keyframes.begin() + m_SelectedFrame);
		m_SelectedFrame = -1;
	}

	for (size_t i = 0; i < track.Keyframes.size(); ++i) {
		CutSceneKeyframe& frame = track.Keyframes[i];

		char header[32] = {};
		std::snprintf(header, sizeof(header), "KF %d (%.2fs)", static_cast<int>(i), frame.Time);

		const bool is_open = ImGui::TreeNode((void*)(intptr_t)i, "%s%s",
			header,
			(static_cast<int>(i) == m_SelectedFrame) ? IMGUI_JP(" [選択中]") : "");
		if (ImGui::IsItemClicked()) {
			m_SelectedFrame = static_cast<int>(i);
		}
		if (!is_open) { continue; }

		ImGui::DragFloat(IMGUI_JP("時刻"), &frame.Time, 0.01f, 0.0f, 60.0f, "%.2fs");
		ImGui::DragFloat3(IMGUI_JP("位置"), &frame.Position.x, 0.05f);
		ImGui::DragFloat3(IMGUI_JP("回転(度)"), &frame.RotationDeg.x, 0.5f);
		ImGui::DragFloat(IMGUI_JP("スケール"), &frame.Scale, 0.01f, 0.05f, 10.0f, "%.2f");

		if (track.Type == eCutSceneTrackType::Camera) {
			ImGui::DragFloat3(IMGUI_JP("注視点"), &frame.LookAt.x, 0.05f);
		}

		int easing = static_cast<int>(frame.Easing);
		if (ImGui::SliderInt(IMGUI_JP("イージング"), &easing, 0, kEasingTypeCount - 1, GetEasingTypeName(static_cast<MyEasing::Type>(easing)))) {
			frame.Easing = static_cast<MyEasing::Type>(easing);
		}

		ImGui::TreePop();
	}
}

bool CutSceneEditor::SaveToFile() const
{
	if (m_Event.Name.empty()) { return false; }

	std::error_code ec{};
	std::filesystem::create_directories(kJsonDir, ec);

	const std::filesystem::path path = std::filesystem::path{ kJsonDir } / (m_Event.Name + ".json");
	return FileManager::JsonSave(path, CutSceneEventToJson(m_Event));
}

bool CutSceneEditor::LoadFromFile(const std::string& Name)
{
	const nlohmann::json data = FileManager::JsonLoad(std::filesystem::path{ kJsonDir } / (Name + ".json"));
	CutSceneEvent event{};
	if (!CutSceneEventFromJson(data, event)) { return false; }

	event.RecalculateDuration();
	m_Event         = std::move(event);
	if (m_Event.Name.empty()) { m_Event.Name = Name; }
	m_SelectedTrack = -1;
	m_SelectedFrame = -1;

	return true;
}
