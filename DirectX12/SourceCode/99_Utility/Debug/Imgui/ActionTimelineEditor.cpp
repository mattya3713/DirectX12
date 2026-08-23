#include "ActionTimelineEditor.h"

#include <algorithm>
#include <memory>

#include "ImGuiManager.h"
#include "10_Ggraphic/30_Asset/RuntimeModel/MMdl/MMdlActor.h"
#include "99_Utility/FileManager/FileManager.h"

namespace {
	// ファイル名→アニメーションクリップ名の対応表(Player側StateのApplyNamedClip呼び出しと一致させる).
	struct ClipMapping
	{
		const char* FileName;
		const char* ClipName;
	};
	constexpr ClipMapping kClipMappings[] = {
		{ "AttackCombo_0.json", "player_attack1" },
		{ "AttackCombo_1.json", "player_attack2" },
		{ "AttackCombo_2.json", "player_attack3" },
		{ "Parry.json",         "player_parry"   },
	};

	constexpr float kTimelinePadRate = 1.05f; // 最長時刻に対する表示余白の割合.

	// SettingsData同士の同値比較(Undoコマンドの要否判定用).
	bool SettingsEquals(const ActionTimelineEditor::SettingsData& A, const ActionTimelineEditor::SettingsData& B)
	{
		return A.ComboStartTime == B.ComboStartTime &&
		       A.MinComboTransTime == B.MinComboTransTime &&
		       A.ComboEndTime == B.ComboEndTime &&
		       A.Windows == B.Windows;
	}

	// ドラッグ編集1回分を「編集前後のSettingsDataスナップショット」として記録するコマンド.
	// (ベクタ再配置に強い全状態スナップショット方式. データは数フレーム分程度と小さい).
	class TimelineStateCommand final : public IEditorCommand
	{
	public:
		TimelineStateCommand(ActionTimelineEditor* pOwner, ActionTimelineEditor::SettingsData OldState,
			ActionTimelineEditor::SettingsData NewState, const char* pLabel)
			: m_pOwner(pOwner), m_OldState(OldState), m_NewState(NewState), m_Label(pLabel)
		{
		}

		void Execute() override { m_pOwner->ApplySettings(m_NewState); }
		void Undo() override { m_pOwner->ApplySettings(m_OldState); }
		const char* GetLabel() const override { return m_Label; }

	private:
		ActionTimelineEditor* const            m_pOwner;
		const ActionTimelineEditor::SettingsData m_OldState;
		const ActionTimelineEditor::SettingsData m_NewState;
		const char* const                      m_Label;
	};
}

ActionTimelineEditor::ActionTimelineEditor()
{
	ScanFiles();

	// 初回は先頭のファイルを選択して読み込んでおく(アクターは未確定なのでクリップ再生はしない).
	if (!m_FileNames.empty())
	{
		m_SelectedFile = m_FileNames.front();
		LoadSelected(nullptr);
	}
}

void ActionTimelineEditor::ScanFiles()
{
	m_FileNames.clear();

	const std::filesystem::path dir = kJsonDir;
	if (!std::filesystem::exists(dir)) { return; }

	for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(dir))
	{
		if (!entry.is_regular_file() || entry.path().extension() != ".json") { continue; }
		m_FileNames.push_back(entry.path().filename().string());
	}

	std::sort(m_FileNames.begin(), m_FileNames.end());
}

void ActionTimelineEditor::LoadSelected(MmdlActor* pActor)
{
	if (m_SelectedFile.empty()) { return; }

	m_SelectedPath  = std::filesystem::path(kJsonDir) / m_SelectedFile;
	m_Data          = SettingsData{};
	m_SelectedWindow = -1;
	m_ScrubSeconds   = 0.0f;
	m_DragMode       = DragMode::None;
	m_DragWindow     = -1;
	m_ClipName       = FindClipName();
	m_Commands.Clear(); // ファイルが変わったためUndo/Redo履歴は無効化する.

	const nlohmann::json data = FileManager::JsonLoad(m_SelectedPath);
	if (!data.empty())
	{
		m_Data.ComboStartTime    = data.value("ComboStartTime", 0.0f);
		m_Data.MinComboTransTime = data.value("MinComboTransTime", 0.0f);
		m_Data.ComboEndTime      = data.value("ComboEndTime", 1.0f);

		if (data.contains("ColliderWindows"))
		{
			for (const nlohmann::json& window : data["ColliderWindows"])
			{
				m_Data.Windows.push_back({ window.value("start", 0.0f), window.value("duration", 0.1f) });
			}
		}
	}

	// アクション切り替えに合わせて該当クリップを先頭から再生し直す
	// (SetCurrentFrameによる外部駆動状態からの復帰も兼ねる).
	if (pActor && !m_ClipName.empty())
	{
		pActor->PlayAnimation(m_ClipName);
	}
}

bool ActionTimelineEditor::SaveSelected()
{
	// 保存前に全項目を再検証し、範囲外の値をドラッグ編集と同じ制約へクランプする.
	ClampSettings();

	nlohmann::json windows = nlohmann::json::array();
	for (const WindowParam& window : m_Data.Windows)
	{
		windows.push_back({ { "start", window.Start }, { "duration", window.Duration } });
	}

	nlohmann::json out;
	out["ComboStartTime"]    = m_Data.ComboStartTime;
	out["MinComboTransTime"] = m_Data.MinComboTransTime;
	out["ComboEndTime"]      = m_Data.ComboEndTime;
	out["ColliderWindows"]   = windows;

	return FileManager::JsonSave(m_SelectedPath, out);
}

const char* ActionTimelineEditor::FindClipName() const
{
	for (const ClipMapping& mapping : kClipMappings)
	{
		if (m_SelectedFile == mapping.FileName) { return mapping.ClipName; }
	}
	return "";
}

void ActionTimelineEditor::ApplyScrub(MmdlActor& Actor)
{
	if (!m_ClipName.empty() && Actor.GetCurrentClipIndex() < 0)
	{
		// 未再生状態ではSetCurrentFrameが無視されるため、まずクリップを起動する.
		Actor.PlayAnimation(m_ClipName);
	}
	Actor.SetCurrentFrame(m_ScrubSeconds * 30.0f);
}

float ActionTimelineEditor::TimeMax() const
{
	float latest = m_Data.ComboEndTime;
	for (const WindowParam& window : m_Data.Windows)
	{
		latest = std::max(latest, window.Start + window.Duration);
	}
	return std::max(latest * kTimelinePadRate, 0.1f);
}

// 編集データを一括差し替える(Undo/Redoコマンドから履歴復元に使う).
void ActionTimelineEditor::ApplySettings(const SettingsData& Data)
{
	m_Data = Data;
}

// 数値直接入力でもドラッグ編集と同じ範囲制約を保証するためのクランプ.
// 制約: Start>=0 / Duration>0 / Start+Duration<=ComboEndTime /
//       0<=ComboStartTime<=MinComboTransTime<=ComboEndTime.
bool ActionTimelineEditor::ClampSettings()
{
	bool was_clamped = false;

	const auto clamp_check = [&was_clamped](float Value, float Min, float Max) {
		const float clamped = std::clamp(Value, Min, Max);
		if (clamped != Value) { was_clamped = true; }
		return clamped;
	};

	// 終了時刻が最上位の上限(下限0.1秒).
	m_Data.ComboEndTime = clamp_check(m_Data.ComboEndTime, 0.1f, 3600.0f);

	// 判定区間: 開始>=0、長さ>0、開始+長さ<=終了時刻.
	for (WindowParam& window : m_Data.Windows)
	{
		window.Start    = clamp_check(window.Start, 0.0f, m_Data.ComboEndTime);
		window.Duration = clamp_check(window.Duration, 0.01f,
			std::max(0.01f, m_Data.ComboEndTime - window.Start));
	}

	// コンボ時間: 受付開始 <= 最低遷移 <= 終了(順序制約のためこの順でクランプする).
	m_Data.ComboStartTime    = clamp_check(m_Data.ComboStartTime, 0.0f, m_Data.ComboEndTime);
	m_Data.MinComboTransTime = clamp_check(m_Data.MinComboTransTime, m_Data.ComboStartTime, m_Data.ComboEndTime);

	return was_clamped;
}

void ActionTimelineEditor::Draw(MmdlActor& Actor)
{
	ImGui::SetNextWindowPos(ImVec2(500.0f, 430.0f), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin(IMGUI_JP("Action Timeline Editor"))) { ImGui::End(); return; }

	// Ctrl+Z / Ctrl+Y(Ctrl+Shift+Z)での取り消し・やり直し(共通Editorフレームワーク).
	m_Commands.HandleShortcuts();

	// 数値直接入力等で範囲外になった値はドラッグ編集と同じ制約へ常時クランプする.
	if (ClampSettings()) {
		ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f),
			IMGUI_JP("範囲外の入力を補正しました(詳細はタイムライン/一覧を参照)"));
	}

	if (!m_SelectedPath.empty() && ImGui::Button(IMGUI_JP("保存")))
	{
		SaveSelected();
	}
	ImGui::SameLine();
	if (ImGui::Button(IMGUI_JP("再読込")))
	{
		ScanFiles();
		LoadSelected(&Actor);
	}

	// 対象JSONの選択(コンボ). 選択が変わったら読み直す.
	const std::string previous = m_SelectedFile;
	ImGuiManager::Combo("アクション", m_SelectedFile, m_FileNames);
	if (!m_FileNames.empty() && m_SelectedFile != previous)
	{
		LoadSelected(&Actor);
	}

	ImGui::Separator();

	// コンボタイミングの数値編集(タイムライン上でも調整可能).
	// NOTE: ラッパー(Tweak等)には素の日本語リテラルを渡す(内部でUTF-8へ自動変換される).
	ImGuiManager::Tweak("コンボ受付開始", m_Data.ComboStartTime, 0.0f, 10.0f);
	ImGuiManager::Tweak("最低遷移時刻", m_Data.MinComboTransTime, 0.0f, 10.0f);
	ImGuiManager::Tweak("終了時刻", m_Data.ComboEndTime, 0.1f, 10.0f);

	ImGui::Separator();

	DrawTimeline(Actor);

	ImGui::Separator();

	// ColliderWindowsの一覧編集(タイムラインと同じメンバを操作する).
	if (ImGui::Button(IMGUI_JP("判定区間を追加")))
	{
		m_Data.Windows.push_back({ m_ScrubSeconds, 0.1f });
		m_SelectedWindow = static_cast<int>(m_Data.Windows.size()) - 1;
	}

	for (size_t i = 0; i < m_Data.Windows.size(); ++i)
	{
		WindowParam& window = m_Data.Windows[static_cast<size_t>(i)];
		ImGui::PushID(static_cast<int>(i));

		char label[32] = {};
		sprintf_s(label, "Window %d##select", static_cast<int>(i));
		if (ImGui::SmallButton(label))
		{
			m_SelectedWindow = (m_SelectedWindow == static_cast<int>(i)) ? -1 : static_cast<int>(i);
		}
		ImGui::SameLine();

		ImGuiManager::Tweak("開始", window.Start, 0.0f, m_Data.ComboEndTime);
		ImGuiManager::Tweak("長さ", window.Duration, 0.01f, m_Data.ComboEndTime);

		if (ImGui::SmallButton(IMGUI_JP("削除")))
		{
			m_Data.Windows.erase(m_Data.Windows.begin() + static_cast<ptrdiff_t>(i));
			m_SelectedWindow = -1;
			ImGui::PopID();
			break;
		}

		ImGui::PopID();
	}

	ImGui::End();
}

void ActionTimelineEditor::DrawTimeline(MmdlActor& Actor)
{
	const float time_max = TimeMax();
	const float width    = ImGui::GetContentRegionAvail().x;
	const ImVec2 origin  = ImGui::GetCursorScreenPos();
	ImDrawList* draw     = ImGui::GetWindowDrawList();

	const auto time_to_x = [&](float time) { return origin.x + (time / time_max) * width; };
	const auto x_to_time = [&](float x)     { return ((x - origin.x) / width) * time_max; };

	ImGui::InvisibleButton("##timeline", ImVec2(width, kBarHeight));
	const bool is_hovered = ImGui::IsItemHovered();
	ImGuiIO& io = ImGui::GetIO();

	// バー背景.
	draw->AddRectFilled(origin, ImVec2(origin.x + width, origin.y + kBarHeight), IM_COL32(38, 38, 38, 255), 3.0f);

	// 0.1秒刻みのグリッドと秒ラベル.
	for (float tick = 0.0f; tick <= time_max; tick += 0.1f)
	{
		const float x = time_to_x(tick);
		draw->AddLine(ImVec2(x, origin.y + kBarHeight - 6.0f), ImVec2(x, origin.y + kBarHeight), IM_COL32(90, 90, 90, 255));
		if (static_cast<int>(tick * 10.0f + 0.5f) % 5 == 0)
	{
		char text[16] = {};
		snprintf(text, sizeof(text), "%.1fs", tick);
		draw->AddText(ImVec2(x + 2.0f, origin.y + kBarHeight - 14.0f), IM_COL32(150, 150, 150, 255), text);
	}
	}

	// ComboStartTime/MinComboTransTimeのマーカー(上から下への縦線).
	const auto draw_marker = [&](float time, ImU32 color) {
		const float x = time_to_x(time);
		draw->AddLine(ImVec2(x, origin.y), ImVec2(x, origin.y + kBarHeight), color, 2.0f);
	};
	draw_marker(m_Data.ComboStartTime,    IM_COL32(235, 210, 70, 255));
	draw_marker(m_Data.MinComboTransTime, IM_COL32(80, 200, 240, 255));

	// 攻撃判定区間(半透明の緑. 選択中は明るい縁取り).
	for (size_t i = 0; i < m_Data.Windows.size(); ++i)
	{
		const WindowParam& window = m_Data.Windows[i];
		const float x0 = time_to_x(window.Start);
		const float x1 = time_to_x(window.Start + window.Duration);
		const bool is_selected = (static_cast<int>(i) == m_SelectedWindow);

		draw->AddRectFilled(ImVec2(x0, origin.y), ImVec2(x1, origin.y + kBarHeight),
			is_selected ? IM_COL32(110, 230, 140, 130) : IM_COL32(80, 190, 120, 80));
		draw->AddRect(ImVec2(x0, origin.y), ImVec2(x1, origin.y + kBarHeight),
			is_selected ? IM_COL32(240, 240, 160, 255) : IM_COL32(80, 190, 120, 255));

		// 右端ハンドル(Duration変更のつまみ).
		draw->AddRectFilled(ImVec2(x1 - 3.0f, origin.y), ImVec2(x1 + 3.0f, origin.y + kBarHeight), IM_COL32(220, 220, 220, 200));
	}

	// 再生位置カーソル(MmdlActorの実際の再生位置に追従する).
	{
		const float current = Actor.GetCurrentAnimationSeconds();
		const float x = time_to_x(current);
		draw->AddLine(ImVec2(x, origin.y), ImVec2(x, origin.y + kBarHeight), IM_COL32(255, 110, 70, 255), 2.0f);
	}
	draw->AddRect(ImVec2(origin.x, origin.y), ImVec2(origin.x + width, origin.y + kBarHeight), IM_COL32(120, 120, 120, 255));

	{
		char scrub_text[32] = {};
		snprintf(scrub_text, sizeof(scrub_text), "scrub: %.2fs", m_ScrubSeconds);
		ImGuiManager::Text(scrub_text);
	}

	// ドラッグ開始(押した場所で操作種別を決める. ハンドル>マーカー>区間本体>スクラブの優先順).
	if (is_hovered && ImGui::IsMouseClicked(0))
	{
		const float t = x_to_time(io.MousePos.x);
		m_DragMode   = DragMode::Scrub;
		m_DragWindow = -1;

		for (size_t i = 0; i < m_Data.Windows.size(); ++i)
		{
			const WindowParam& window = m_Data.Windows[i];
			if (std::abs(io.MousePos.x - time_to_x(window.Start + window.Duration)) <= kHandleGrabPx)
			{
				m_DragMode   = DragMode::ResizeWindow;
				m_DragWindow = static_cast<int>(i);
				m_SelectedWindow = static_cast<int>(i);
				break;
			}
			if (io.MousePos.x >= time_to_x(window.Start) && io.MousePos.x <= time_to_x(window.Start + window.Duration))
			{
				m_DragMode       = DragMode::MoveWindow;
				m_DragWindow     = static_cast<int>(i);
				m_DragGrabOffset = t - window.Start;
				m_SelectedWindow = static_cast<int>(i);
				break;
			}
		}

		if (m_DragMode == DragMode::Scrub)
		{
			if (std::abs(io.MousePos.x - time_to_x(m_Data.ComboStartTime)) <= kHandleGrabPx)
			{
				m_DragMode = DragMode::MoveComboStart;
			}
			else if (std::abs(io.MousePos.x - time_to_x(m_Data.MinComboTransTime)) <= kHandleGrabPx)
			{
				m_DragMode = DragMode::MoveMinTrans;
			}
		}

		// 編集を伴うドラッグなら開始時の状態を記録しておく(確定時にUndoコマンドへ積む).
		if (m_DragMode != DragMode::Scrub && m_DragMode != DragMode::None)
		{
			m_DragStartSnapshot = m_Data;
		}
	}

	// ドラッグ中の反映(毎フレーム現在のマウスXへ追従させる).
	if (ImGui::IsMouseDown(0) && m_DragMode != DragMode::None && is_hovered)
	{
		const float t = std::clamp(x_to_time(io.MousePos.x), 0.0f, time_max);

		switch (m_DragMode)
		{
		case DragMode::Scrub:
			m_ScrubSeconds = t;
			ApplyScrub(Actor);
			break;
		case DragMode::ResizeWindow:
			if (m_DragWindow >= 0 && m_DragWindow < static_cast<int>(m_Data.Windows.size()))
			{
				WindowParam& window = m_Data.Windows[static_cast<size_t>(m_DragWindow)];
				window.Duration = std::clamp(t - window.Start, 0.01f, time_max);
			}
			break;
		case DragMode::MoveWindow:
			if (m_DragWindow >= 0 && m_DragWindow < static_cast<int>(m_Data.Windows.size()))
			{
				WindowParam& window = m_Data.Windows[static_cast<size_t>(m_DragWindow)];
				const float duration = window.Duration;
				window.Start = std::clamp(t - m_DragGrabOffset, 0.0f, time_max - duration);
			}
			break;
		case DragMode::MoveComboStart:
			m_Data.ComboStartTime = std::min(t, m_Data.ComboEndTime);
			break;
		case DragMode::MoveMinTrans:
			m_Data.MinComboTransTime = std::min(t, m_Data.ComboEndTime);
			break;
		default:
			break;
		}
	}
	else
	{
		// ドラッグ確定: 開始時と値が変わっていればUndo履歴へ積む(スクラブは履歴対象外).
		if (m_DragMode != DragMode::None && m_DragMode != DragMode::Scrub &&
		    !SettingsEquals(m_DragStartSnapshot, m_Data))
		{
			const char* p_label = "edit";
			switch (m_DragMode)
			{
			case DragMode::MoveWindow:    p_label = "区間移動"; break;
			case DragMode::ResizeWindow:  p_label = "区間長変更"; break;
			case DragMode::MoveComboStart: p_label = "コンボ受付開始移動"; break;
			case DragMode::MoveMinTrans:   p_label = "最低遷移時刻移動"; break;
			default: break;
			}

			auto p_command = std::make_unique<TimelineStateCommand>(this, m_DragStartSnapshot, m_Data, p_label);
			m_Commands.Execute(std::move(p_command));
		}

		m_DragMode = DragMode::None;
	}
}
