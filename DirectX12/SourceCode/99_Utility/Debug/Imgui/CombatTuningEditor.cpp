#include "CombatTuningEditor.h"

#include <cstring>
#include <filesystem>

#include "00_Game/60_Combat/CombatTuning.h"
#include "99_Utility/Debug/Imgui/ImGuiManager.h"
#include "99_Utility/Localization/LocalizationTable.h"

namespace {
	constexpr const char* kPresetDir = "Data\\Json\\Combat";

	// 項目テーブルの内容をスライダーとして並べて編集する.
	void DrawFieldGroup(const std::vector<CombatTuningFieldRule>& Rules)
	{
		CombatTuningData& tuning = CombatTuning::Get();
		for (const CombatTuningFieldRule& rule : Rules)
		{
			ImGuiManager::Tweak(rule.Label, tuning.*rule.Field, rule.Min, rule.Max);
		}
	}

	// Boss攻撃1種分のタイミング+威力をまとめて編集するUI.
	void DrawBossAttack(const char* Label, const std::vector<CombatTuningFieldRule>& Rules)
	{
		if (!ImGui::CollapsingHeader(Label)) { return; }
		DrawFieldGroup(Rules);
	}
}

void CombatTuningEditor::Draw()
{
	ImGui::SetNextWindowPos(ImVec2(520.0f, 20.0f), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(420.0f, 640.0f), ImGuiCond_FirstUseEver);
	// ウィンドウタイトルはローカライゼーション経由(lang ja/enで切替デモ).
	if (!ImGui::Begin(LocalizationTable::Instance().GetText("combat_tuning.title").c_str())) { ImGui::End(); return; }

	// スライダーの直接入力(Ctrl+クリック等)で混入した負値/NaN/範囲外の値を毎フレーム検査する.
	// 弾いた項目は表示に残しつつ実値はその場で丸めるため、ゲーム参照中・保存先とも不正値は載らない.
	const CombatTuningValidationReport input_check = CombatTuningRules::Validate(CombatTuning::Get());
	if (!input_check.IsClean())
	{
		m_InvalidInputLog.clear();
		for (const CombatTuningIssue& issue : input_check.Issues)
		{
			m_InvalidInputLog += issue.Path;
			m_InvalidInputLog += ": ";
			m_InvalidInputLog += issue.Message;
			m_InvalidInputLog += '\n';
		}
		CombatTuningRules::Sanitize(CombatTuning::Get());
	}

	if (!m_InvalidInputLog.empty())
	{
		ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.3f, 1.0f), IMGUI_JP("直近の不正入力(自動補正済み)"));
		ImGuiManager::Text(m_InvalidInputLog.c_str());
	}

	// ----- プリセット操作 -----
	if (ImGui::Button(IMGUI_JP("既定値へリセット"))) {
		CombatTuning::ResetToDefaults();
		m_WasLoadedFromMissingFile = false;
		m_InvalidInputLog.clear();
		m_WasSaveBlocked = false;
	}

	ImGui::SameLine();
	if (ImGui::Button(IMGUI_JP("保存")) && m_PresetName[0] != '\0') {
		std::string name = m_PresetName;
		if (name.find(".json") == std::string::npos) { name += ".json"; }
		const std::filesystem::path path = std::filesystem::path(kPresetDir) / name;

		// 書き出す直前にもう一度全項目を検証する. 不正値が残っているなら保存しない.
		if (CombatTuningRules::Validate(CombatTuning::Get()).IsClean()) {
			std::filesystem::create_directories(path.parent_path());
			CombatTuning::Save(path);
			m_InvalidInputLog.clear();
			m_WasSaveBlocked = false;
		}
		else {
			m_WasSaveBlocked = true;
		}
	}

	ImGui::SameLine();
	if (ImGui::Button(IMGUI_JP("読込")) && m_PresetName[0] != '\0') {
		std::string name = m_PresetName;
		if (name.find(".json") == std::string::npos) { name += ".json"; }
		m_WasLoadedFromMissingFile = !CombatTuning::Load(std::filesystem::path(kPresetDir) / name);
		if (!m_WasLoadedFromMissingFile) {
			// 読み込んだ値の検査は次フレームの先頭で行われる.
			m_InvalidInputLog.clear();
			m_WasSaveBlocked = false;
		}
	}
	if (m_WasLoadedFromMissingFile) {
		ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), IMGUI_JP("プリセットが見つかりません"));
	}
	if (m_WasSaveBlocked) {
		ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.3f, 1.0f), IMGUI_JP("値が不正なため保存を中止しました"));
	}

	ImGui::InputText(IMGUI_JP("プリセット名"), m_PresetName, sizeof(m_PresetName));

	ImGui::Separator();

	// ----- Player攻撃(コンボ) -----
	if (ImGui::CollapsingHeader(IMGUI_JP("Player: コンボ"), ImGuiTreeNodeFlags_DefaultOpen))
	{
		DrawFieldGroup(CombatTuningRules::PlayerCombo());
	}

	// ----- Player防御 -----
	if (ImGui::CollapsingHeader(IMGUI_JP("Player: パリィ/回避")))
	{
		DrawFieldGroup(CombatTuningRules::ParryDodge());
	}

	// ----- 演出 -----
	if (ImGui::CollapsingHeader(IMGUI_JP("ヒットストップ/スロー")))
	{
		DrawFieldGroup(CombatTuningRules::HitStopSlow());
	}

	// ----- Boss攻撃 -----
	DrawBossAttack(IMGUI_JP("Boss: 攻撃1(boss_attack1)"), CombatTuningRules::BossAttack1());

	DrawBossAttack(IMGUI_JP("Boss: 攻撃2(boss_attack2)"), CombatTuningRules::BossAttack2());

	DrawBossAttack(IMGUI_JP("Boss: ビーム"), CombatTuningRules::BossBeam());

	if (ImGui::CollapsingHeader(IMGUI_JP("Boss: 跳躍攻撃")))
	{
		DrawFieldGroup(CombatTuningRules::BossJump());
	}

	DrawBossAttack(IMGUI_JP("Boss: 回転攻撃"), CombatTuningRules::BossSpin());

	ImGui::End();
}
