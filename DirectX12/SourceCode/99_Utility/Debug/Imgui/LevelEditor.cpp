#include "LevelEditor.h"

#include <algorithm>
#include <cstdio>
#include <cstring>

#include "99_Utility/Debug/Imgui/ImGuiManager.h"
#include "99_Utility/Debug/Log/DebugLog.h"
#include "99_Utility/ServiceLocator/ServiceLocator.h"

namespace {

	constexpr const char* kEnemyDefinitionJson = "Data\\Json\\Enemy\\Definitions.json";

	// カタログに存在しない敵IDの数を数える(保存時の警告用).
	int CountUnknownEnemyIds(const std::vector<LevelEnemySpawnDesc>& Spawns, const EnemyDefinitionCatalog& Catalog)
	{
		int unknown = 0;
		for (const LevelEnemySpawnDesc& spawn : Spawns)
		{
			if (!Catalog.Contains(spawn.DefinitionId)) { ++unknown; }
		}
		return unknown;
	}

}

LevelEditor::LevelEditor()
{
	m_Catalog.Load(kEnemyDefinitionJson); // 失敗しても空カタログとして動作する.

	ScanFiles();
	ScanMstcFiles();
	ScanEnemyDefinitions();

	if (!m_MstcFileNames.empty())
	{
		m_Objects.push_back({ m_MstcFileNames.front(), { 0.0f, 0.0f, 3.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f } });
	}

	m_PlayerSpawn = { true, { 0.0f, 0.0f, 0.0f }, 0.0f };
	m_BossSpawn   = { true, { 0.0f, 0.0f, 8.0f }, 180.0f };

	LoadSelected(); // 既存JSONがあればその内容で上書きする.
}

void LevelEditor::SetOnLevelChanged(std::function<void(const std::filesystem::path&)> Callback)
{
	m_OnLevelChanged = std::move(Callback);
}

void LevelEditor::Draw()
{
	ImGui::SetNextWindowPos(ImVec2(20.0f, 620.0f), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(460.0f, 320.0f), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin(IMGUI_JP("Level Editor"))) { ImGui::End(); return; }

	// ----- ファイル選択・保存 -----
	if (!m_FileNames.empty()) {
		std::string previous = m_SelectedFile;
		ImGuiManager::Combo("Level", m_SelectedFile, m_FileNames);
		if (m_SelectedFile != previous) {
			LoadSelected();
			if (m_OnLevelChanged) { m_OnLevelChanged(m_SelectedPath); }
		}
	}
	else {
		ImGuiManager::Text("Data\\Json\\LevelにJSONが見つかりません.");
	}

	ImGui::SameLine();
	if (ImGui::Button(IMGUI_JP("再読込"))) {
		ScanFiles();
		ScanMstcFiles();
		ScanEnemyDefinitions();
	}

	ImGui::SameLine();
	if (m_SelectedFile.empty()) { ImGui::BeginDisabled(); }
	if (ImGui::Button(IMGUI_JP("上書き保存")) && SaveSelected() && m_OnLevelChanged) {
		m_OnLevelChanged(m_SelectedPath);
	}
	if (m_SelectedFile.empty()) { ImGui::EndDisabled(); }

	// 新規保存(名前を入力して保存).
	ImGui::InputText(IMGUI_JP("New File"), m_NewFileName, sizeof(m_NewFileName));
	if (m_NewFileName[0] != '\0' && ImGui::Button(IMGUI_JP("新規保存"))) {
		std::string new_name = m_NewFileName;
		if (new_name.find(".json") == std::string::npos) { new_name += ".json"; }
		const bool name_is_invalid = new_name.find("..") != std::string::npos ||
			new_name.find('/') != std::string::npos || new_name.find('\\') != std::string::npos;
		if (!name_is_invalid) {
			m_SelectedFile = new_name;
			if (SaveSelected()) {
				ScanFiles();
				std::memset(m_NewFileName, 0, sizeof(m_NewFileName));
				if (m_OnLevelChanged) { m_OnLevelChanged(m_SelectedPath); }
			}
		}
	}

	ImGui::Separator();

	// ----- 配置オブジェクト一覧 -----
	int removed_index = -1;
	for (size_t i = 0; i < m_Objects.size(); ++i) {
		LevelObjectDesc& object = m_Objects[i];

		char header[64];
		std::snprintf(header, sizeof(header), IMGUI_JP("Object %zu: %s"), i, object.MstcFile.c_str());

		ImGui::PushID(static_cast<int>(i));
		if (ImGui::CollapsingHeader(header)) {
			ImGuiManager::Combo("Mstc", object.MstcFile, m_MstcFileNames);
			ImGuiManager::Input("Pos X", object.Position.x);
			ImGuiManager::Input("Pos Y", object.Position.y);
			ImGuiManager::Input("Pos Z", object.Position.z);
			ImGuiManager::Input("Rot X(deg)", object.RotationDeg.x);
			ImGuiManager::Input("Rot Y(deg)", object.RotationDeg.y);
			ImGuiManager::Input("Rot Z(deg)", object.RotationDeg.z);
			ImGuiManager::Tweak("Scale X", object.Scale.x, 0.01f, 50.0f);
			ImGuiManager::Tweak("Scale Y", object.Scale.y, 0.01f, 50.0f);
			ImGuiManager::Tweak("Scale Z", object.Scale.z, 0.01f, 50.0f);

			if (ImGui::SmallButton(IMGUI_JP("複製"))) {
				m_Objects.insert(m_Objects.begin() + static_cast<std::ptrdiff_t>(i) + 1, object);
			}
			ImGui::SameLine();
			if (ImGui::SmallButton(IMGUI_JP("削除")) && m_Objects.size() > 1) {
				removed_index = static_cast<int>(i);
			}
		}
		ImGui::PopID();
	}

	if (removed_index >= 0) {
		m_Objects.erase(m_Objects.begin() + removed_index);
	}

	if (ImGui::Button(IMGUI_JP("オブジェクト追加")) && !m_MstcFileNames.empty()) {
		LevelObjectDesc added{};
		added.MstcFile = m_MstcFileNames.front();
		m_Objects.push_back(added);
	}

	ImGui::Separator();

	// ----- 敵スポーン一覧 -----
	ImGui::TextUnformatted(IMGUI_JP("Enemy Spawns"));
	int removed_enemy_index = -1;
	for (size_t i = 0; i < m_EnemySpawns.size(); ++i) {
		LevelEnemySpawnDesc& spawn = m_EnemySpawns[i];

		char header[96];
		std::snprintf(header, sizeof(header), IMGUI_JP("Enemy %zu: %s"), i, spawn.DefinitionId.c_str());

		ImGui::PushID(static_cast<int>(1000 + i));
		if (ImGui::CollapsingHeader(header)) {
			ImGuiManager::Combo("定義ID", spawn.DefinitionId, m_CatalogIds);
			ImGuiManager::Input("InstanceName", spawn.InstanceName);
			ImGuiManager::Input("Pos X", spawn.Position.x);
			ImGuiManager::Input("Pos Y", spawn.Position.y);
			ImGuiManager::Input("Pos Z", spawn.Position.z);
			ImGuiManager::Input("Rot Y(deg)", spawn.RotationDeg.y);
			ImGuiManager::Tweak("Scale X", spawn.Scale.x, 0.01f, 10.0f);
			ImGuiManager::Tweak("Scale Y", spawn.Scale.y, 0.01f, 10.0f);
			ImGuiManager::Tweak("Scale Z", spawn.Scale.z, 0.01f, 10.0f);

			if (ImGui::SmallButton(IMGUI_JP("複製"))) {
				m_EnemySpawns.insert(m_EnemySpawns.begin() + static_cast<std::ptrdiff_t>(i) + 1, spawn);
			}
			ImGui::SameLine();
			if (ImGui::SmallButton(IMGUI_JP("削除"))) {
				removed_enemy_index = static_cast<int>(i);
			}
		}
		ImGui::PopID();
	}

	if (removed_enemy_index >= 0) {
		m_EnemySpawns.erase(m_EnemySpawns.begin() + removed_enemy_index);
	}

	if (ImGui::Button(IMGUI_JP("敵スポーン追加")) && !m_CatalogIds.empty()) {
		LevelEnemySpawnDesc added{};
		added.DefinitionId = m_CatalogIds.front();
		m_EnemySpawns.push_back(added);
	}
	if (m_CatalogIds.empty()) {
		ImGui::SameLine();
		ImGuiManager::Text("Definitions.jsonが見つかりません.");
	}

	ImGui::Separator();

	// ----- スポーン地点 -----
	ImGui::TextUnformatted(IMGUI_JP("Player Spawn"));
	ImGuiManager::CheckBox("Player Spawn有効", m_PlayerSpawn.HasValue);
	ImGuiManager::Input("P Spawn X", m_PlayerSpawn.Position.x);
	ImGuiManager::Input("P Spawn Y", m_PlayerSpawn.Position.y);
	ImGuiManager::Input("P Spawn Z", m_PlayerSpawn.Position.z);
	ImGuiManager::Input("P Spawn Yaw(deg)", m_PlayerSpawn.YawDeg);

	ImGui::TextUnformatted(IMGUI_JP("Boss Spawn"));
	ImGuiManager::CheckBox("Boss Spawn有効", m_BossSpawn.HasValue);
	ImGuiManager::Input("B Spawn X", m_BossSpawn.Position.x);
	ImGuiManager::Input("B Spawn Y", m_BossSpawn.Position.y);
	ImGuiManager::Input("B Spawn Z", m_BossSpawn.Position.z);
	ImGuiManager::Input("B Spawn Yaw(deg)", m_BossSpawn.YawDeg);

	ImGui::End();
}

void LevelEditor::ScanFiles()
{
	namespace fs = std::filesystem;

	m_FileNames.clear();

	const fs::path directory = kJsonDir;

	std::error_code error;
	fs::create_directories(directory, error); // 無ければ作る(保存先の確保).

	for (const fs::directory_entry& entry : fs::directory_iterator(directory, error)) {
		if (entry.is_regular_file() && entry.path().extension() == ".json") {
			m_FileNames.push_back(entry.path().filename().string());
		}
	}

	std::sort(m_FileNames.begin(), m_FileNames.end());

	if (m_FileNames.empty()) {
		m_SelectedFile.clear();
		m_SelectedPath.clear();
		return;
	}

	if (std::find(m_FileNames.begin(), m_FileNames.end(), m_SelectedFile) == m_FileNames.end()) {
		m_SelectedFile = m_FileNames.front();
	}
	m_SelectedPath = directory / m_SelectedFile;
}

void LevelEditor::ScanMstcFiles()
{
	namespace fs = std::filesystem;

	m_MstcFileNames.clear();

	std::error_code error;
	const fs::path directory = kMstcDir;
	if (!fs::is_directory(directory, error)) { return; }

	for (const fs::directory_entry& entry : fs::directory_iterator(directory, error)) {
		if (entry.is_regular_file() && entry.path().extension() == ".mstc") {
			m_MstcFileNames.push_back(entry.path().filename().string());
		}
	}

	std::sort(m_MstcFileNames.begin(), m_MstcFileNames.end());
}

void LevelEditor::ScanEnemyDefinitions()
{
	// カタログをJSONから再読込し、コンボ候補のID一覧を更新する.
	m_Catalog.Load(kEnemyDefinitionJson);
	m_CatalogIds = m_Catalog.GetIds();
	std::sort(m_CatalogIds.begin(), m_CatalogIds.end());
}

void LevelEditor::LoadSelected()
{
	if (m_SelectedFile.empty()) { return; }

	m_SelectedPath = std::filesystem::path(kJsonDir) / m_SelectedFile;

	const LevelDesc desc = LoadLevelJson(m_SelectedPath);

	m_Objects     = desc.Objects;
	m_EnemySpawns = desc.EnemySpawns;
	m_PlayerSpawn = desc.PlayerSpawn;
	m_BossSpawn   = desc.BossSpawn;

	if (m_Objects.empty() && !m_MstcFileNames.empty())
	{
		m_Objects.push_back({ m_MstcFileNames.front(), { 0.0f, 0.0f, 3.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f } });
	}
}

bool LevelEditor::SaveSelected()
{
	if (m_SelectedFile.empty()) { return false; }

	m_SelectedPath = std::filesystem::path(kJsonDir) / m_SelectedFile;

	// 未知の敵IDは警告の上そのまま保存する(Editor単体では判定を潰さないため.
	// 読込側はカタログと照合して安全に扱う).
	if (const int unknown = CountUnknownEnemyIds(m_EnemySpawns, m_Catalog); unknown > 0)
	{
		if (DebugLog* p_debug_log = ServiceLocator::Get<DebugLog>()) {
			p_debug_log->LogWarning("LevelEditor: " + std::to_string(unknown) + "個の未登録敵IDを含んだまま保存しました");
		}
	}

	LevelDesc desc{};
	desc.Objects     = m_Objects;
	desc.EnemySpawns = m_EnemySpawns;
	desc.PlayerSpawn = m_PlayerSpawn;
	desc.BossSpawn   = m_BossSpawn;

	return LevelData::WriteToFile(m_SelectedPath, desc);
}

LevelDesc LevelEditor::LoadLevelJson(const std::filesystem::path& Path)
{
	return LevelData::LoadFromFile(Path);
}
