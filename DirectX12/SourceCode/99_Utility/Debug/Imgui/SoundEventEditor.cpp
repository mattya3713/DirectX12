#include "stdafx.h"
#include "SoundEventEditor.h"

#include <filesystem>

#include "ImGuiManager.h"
#include "99_Utility/Debug/Log/DebugLog.h"
#include "99_Utility/FileManager/FileManager.h"
#include "99_Utility/ServiceLocator/ServiceLocator.h"
#include "99_Utility/Sound/SoundManager.h"

namespace {

	constexpr const char* kJsonDir = "Data/Json/SoundEvent";

	nlohmann::json EventToJson(const SoundEventEditor::SoundEventData& Event)
	{
		return {
			{ "Name",       Event.Name },
			{ "FileName",   Event.FileName },
			{ "Volume",     Event.Volume },
			{ "Pitch",      Event.Pitch },
			{ "MaxVoices",  Event.MaxVoices },
			{ "Cooldown",   Event.Cooldown },
			{ "IsLoop",     Event.IsLoop },
		};
	}

	SoundEventEditor::SoundEventData EventFromJson(const nlohmann::json& JsonData)
	{
		SoundEventEditor::SoundEventData event{};
		event.Name      = JsonData.value("Name", "");
		event.FileName  = JsonData.value("FileName", "");
		event.Volume    = JsonData.value("Volume", 1.0f);
		event.Pitch     = JsonData.value("Pitch", 1.0f);
		event.MaxVoices = JsonData.value("MaxVoices", 1);
		event.Cooldown  = JsonData.value("Cooldown", 0.1f);
		event.IsLoop    = JsonData.value("IsLoop", false);
		return event;
	}

	void LogToConsole(const std::string& Message)
	{
		if (DebugLog* p_debug_log = ServiceLocator::Get<DebugLog>()) {
			p_debug_log->LogInfo(Message);
		}
	}

}

// Combat等からイベント名指定で再生する(Cooldown判定込み. 未定義なら何もしない).
void SoundEventEditor::PlayCombatEvent(const char* Name)
{
	SoundEventEditor* p_editor = ServiceLocator::Get<SoundEventEditor>();
	if (!p_editor) { return; }

	for (SoundEventData& event : p_editor->m_Events)
	{
		if (event.Name == Name)
		{
			p_editor->PlayEvent(event);
			return;
		}
	}
}

void SoundEventEditor::Draw()
{
	if (!ImGui::Begin("Sound Event Editor")) {
		ImGui::End();
		return;
	}

	// イベント一覧.
	if (ImGui::BeginListBox("##Events", ImVec2(-1.0f, 120.0f))) {
		for (size_t i = 0; i < m_Events.size(); ++i) {
			const bool is_selected = (static_cast<int>(i) == m_Selected);
			if (ImGui::Selectable(m_Events[i].Name.c_str(), is_selected)) {
				m_Selected = static_cast<int>(i);
			}
		}
		ImGui::EndListBox();
	}

	if (ImGui::Button(IMGUI_JP("イベント追加"))) {
		SoundEventData event{};
		char buffer[32];
		std::snprintf(buffer, sizeof(buffer), "event_%zu", m_Events.size());
		event.Name = buffer;
		m_Events.push_back(event);
		m_Selected = static_cast<int>(m_Events.size()) - 1;
	}
	ImGui::SameLine();
	if (m_Selected >= 0 && ImGui::Button(IMGUI_JP("イベント削除"))) {
		m_Events.erase(m_Events.begin() + m_Selected);
		m_Selected = -1;
	}

	// 選択中イベントの編集.
	if (m_Selected >= 0 && m_Selected < static_cast<int>(m_Events.size()))
	{
		SoundEventData& event = m_Events[m_Selected];

		ImGui::Separator();

		char name_buffer[64] = {};
		event.Name.copy(name_buffer, sizeof(name_buffer) - 1);
		if (ImGui::InputText(IMGUI_JP("イベント名"), name_buffer, sizeof(name_buffer))) { event.Name = name_buffer; }

		char file_buffer[128] = {};
		event.FileName.copy(file_buffer, sizeof(file_buffer) - 1);
		if (ImGui::InputText(IMGUI_JP("音声ファイル(拡張子無し)"), file_buffer, sizeof(file_buffer))) { event.FileName = file_buffer; }

		ImGui::SliderFloat(IMGUI_JP("音量"), &event.Volume, 0.0f, 1.0f, "%.2f");
		ImGui::SliderFloat(IMGUI_JP("ピッチ(保存のみ)"), &event.Pitch, 0.5f, 2.0f, "%.2f");
		ImGui::InputInt(IMGUI_JP("同時再生数上限(保存のみ)"), &event.MaxVoices);
		ImGui::DragFloat(IMGUI_JP("クールダウン(秒)"), &event.Cooldown, 0.01f, 0.0f, 10.0f, "%.2f");
		ImGui::Checkbox(IMGUI_JP("ループ"), &event.IsLoop);

		if (ImGui::Button(IMGUI_JP("試聴")) && !event.FileName.empty())
		{
			if (SoundManager* p_sound_manager = ServiceLocator::Get<SoundManager>()) {
				p_sound_manager->Play(event.FileName, event.IsLoop, event.Volume);
			}
		}
		ImGui::SameLine();
		if (ImGui::Button(IMGUI_JP("停止")) && !event.FileName.empty())
		{
			if (SoundManager* p_sound_manager = ServiceLocator::Get<SoundManager>()) {
				p_sound_manager->Stop(event.FileName);
			}
		}
	}

	ImGui::Separator();

	// プリセット保存/読込.
	char preset_buffer[128] = {};
	m_PresetName.copy(preset_buffer, sizeof(preset_buffer) - 1);
	if (ImGui::InputText(IMGUI_JP("プリセット名"), preset_buffer, sizeof(preset_buffer))) {
		m_PresetName = preset_buffer;
	}

	if (ImGui::Button(IMGUI_JP("プリセット保存"))) {
		SavePresets();
	}
	ImGui::SameLine();
	if (ImGui::Button(IMGUI_JP("プリセット読込"))) {
		LoadPresets(m_PresetName);
	}

	ImGui::End();
}

// イベント名を指定して再生中の音を停止する.
void SoundEventEditor::StopCombatEvent(const char* Name)
{
	SoundEventEditor* p_editor = ServiceLocator::Get<SoundEventEditor>();
	if (!p_editor) { return; }

	for (const SoundEventData& event : p_editor->m_Events)
	{
		if (event.Name == Name)
		{
			if (SoundManager* p_sound_manager = ServiceLocator::Get<SoundManager>()) {
				p_sound_manager->Stop(event.FileName);
			}
			return;
		}
	}
}

// 試聴/Combatからの再生(Cooldown・同時再生数上限判定込み).
void SoundEventEditor::PlayEvent(SoundEventData& Event)
{
	const auto now = std::chrono::steady_clock::now();
	const std::string cooldown_key = "cooldown_" + Event.Name;

	// クールダウン判定(経過不十分ならスキップ).
	const auto it = m_LastPlayed.find(cooldown_key);
	if (it != m_LastPlayed.end())
	{
		const float elapsed = std::chrono::duration<float>(now - it->second).count();
		if (elapsed < Event.Cooldown)
		{
			LogToConsole("SoundEvent: cooldown skip: " + Event.Name);
			return;
		}
	}

	SoundManager* p_sound_manager = ServiceLocator::Get<SoundManager>();

	if (!Event.FileName.empty() && p_sound_manager)
	{
		// 同時再生数上限(SoundManagerのアクティブボイスカウントで判定).
		if (p_sound_manager->GetActiveVoiceCount(Event.FileName) >= Event.MaxVoices)
		{
			LogToConsole("SoundEvent: max voices reached: " + Event.Name);
			m_LastPlayed[cooldown_key] = now;
			return;
		}

		p_sound_manager->PlayEx(Event.FileName, Event.Volume, Event.Pitch, Event.IsLoop);
	}

	m_LastPlayed[cooldown_key] = now;
}

bool SoundEventEditor::SavePresets() const
{
	std::error_code ec{};
	std::filesystem::create_directories(kJsonDir, ec);

	nlohmann::json events = nlohmann::json::array();
	for (const SoundEventData& event : m_Events) {
		events.push_back(EventToJson(event));
	}

	const nlohmann::json data = { { "Events", events } };
	return FileManager::JsonSave(std::filesystem::path{ kJsonDir } / (m_PresetName + ".json"), data);
}

bool SoundEventEditor::LoadPresets(const std::string& Name)
{
	const nlohmann::json data = FileManager::JsonLoad(std::filesystem::path{ kJsonDir } / (Name + ".json"));
	if (data.is_null() || !data.contains("Events") || !data["Events"].is_array()) { return false; }

	m_Events.clear();
	for (const nlohmann::json& event_json : data["Events"]) {
		m_Events.push_back(EventFromJson(event_json));
	}
	m_PresetName = Name;

	return true;
}
