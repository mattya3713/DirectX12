#include "ActionTimelineEditor.h"

#include <algorithm>

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

bool ActionTimelineEditor::SaveSelected() const
{
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

void ActionTimelineEditor::Draw(MmdlActor& Actor)
{
	ImGui::SetNextWindowPos(ImVec2(500.0f, 430.0f), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin(IMGUI_JP("Action Timeline Editor"))) { ImGui::End(); return; }

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
		m_DragMode = DragMode::None;
	}
}
