#include "DebugConsole.h"

#include <algorithm>
#include <cctype>

#include "ImGuiManager.h"
#include "00_Game/10_Object/10_MeshObject/00_Character/00_Player/Player.h"
#include "00_Game/10_Object/10_MeshObject/00_Character/20_Boss/Boss.h"
#include "00_Game/50_Input/Input.h"
#include "99_Utility/Debug/Log/DebugLog.h"
#include "99_Utility/ServiceLocator/ServiceLocator.h"

namespace {
	// コンソール表示トグルキー(F1〜F3は既存使用のためF4).
	constexpr UINT kToggleKey = VK_F4;

	DebugConsole& GetConsoleInstance()
	{
		static DebugConsole s_Instance;
		return s_Instance;
	}

	void LogToConsole(const std::string& Message)
	{
		if (DebugLog* p_debug_log = ServiceLocator::Get<DebugLog>()) {
			p_debug_log->LogInfo(Message);
		}
	}
}

// ログ表示+コマンド入力欄の描画(_DEBUG限定. 入力欄はF4でトグル).
void DebugConsole::Draw()
{
#if _DEBUG
	DebugConsole& console = GetConsoleInstance();

	// 初回呼び出し時にServiceLocatorへ登録する(他システムがRegisterCommandできるように).
	if (ServiceLocator::Get<DebugConsole>() == nullptr) {
		ServiceLocator::Provide<DebugConsole>(&console);
	}

	console.EnsureBuiltinCommands();

	if (Input::IsKeyDown(kToggleKey)) {
		console.ToggleVisible();
	}

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

	if (console.m_IsVisible)
	{
		ImGui::SameLine();
		ImGui::Text(IMGUI_JP("[F4] 入力欄: 表示中"));
	}
	else
	{
		ImGui::SameLine();
		ImGui::Text(IMGUI_JP("[F4] 入力欄: 非表示"));
	}

	ImGui::Separator();

	if (console.m_IsVisible)
	{
		// コマンド入力欄(Enterで実行).
		char input_buffer[256] = {};
		console.m_InputBuffer.copy(input_buffer, sizeof(input_buffer) - 1);
		ImGui::SetNextItemWidth(-60.0f);
		const bool is_submitted = ImGui::InputText("##CmdInput", input_buffer, sizeof(input_buffer), ImGuiInputTextFlags_EnterReturnsTrue);
		ImGui::SameLine();
		const bool is_run_pressed = ImGui::Button("Run");

		if ((is_submitted || is_run_pressed) && console.m_InputBuffer.empty() == false)
		{
			console.Execute(console.m_InputBuffer);
			console.m_InputBuffer.clear();
		}
		else
		{
			console.m_InputBuffer = input_buffer;
		}

		ImGui::Separator();
	}

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
#endif
}

void DebugConsole::RegisterCommand(const std::string& Name, CommandHandler Handler)
{
	std::string key = Name;
	std::transform(key.begin(), key.end(), key.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
	m_Commands[key] = std::move(Handler);
}

void DebugConsole::EnsureBuiltinCommands()
{
	if (m_IsBuiltinsRegistered) { return; }
	m_IsBuiltinsRegistered = true;

	// 登録コマンド一覧を表示する.
	RegisterCommand("help", [this](const CommandArgs&) {
		std::string names = "commands:";
		for (const auto& pair : m_Commands) { names += " " + pair.first; }
		LogToConsole(names);
	});

	// Playerのダメージ無効化トグル(暫定チート. Parry/Dodgeの被弾制御を上書きし得る点に注意).
	RegisterCommand("god", [](const CommandArgs&) {
		static bool s_IsGodMode = false;
		s_IsGodMode = !s_IsGodMode;
		if (Player* p_player = ServiceLocator::Get<Player>()) {
			p_player->SetDamageColliderActive(!s_IsGodMode);
			LogToConsole(s_IsGodMode ? "god mode ON" : "god mode OFF");
		}
		else {
			LogToConsole("god: Player not found");
		}
	});

	// Boss即死(HealthSystem経由で通常の死亡フローを通す).
	RegisterCommand("kill_boss", [](const CommandArgs&) {
		if (Boss* p_boss = ServiceLocator::Get<Boss>()) {
			p_boss->ApplyDebugKill();
			LogToConsole("kill_boss executed");
		}
		else {
			LogToConsole("kill_boss: Boss not found");
		}
	});
}

void DebugConsole::Execute(const std::string& Line)
{
	LogToConsole("> " + Line);

	// 空白区切りでトークン化.
	CommandArgs args;
	std::string token;
	for (const char c : Line)
	{
		if (c == ' ' || c == '\t')
		{
			if (!token.empty()) { args.push_back(token); token.clear(); }
		}
		else
		{
			token += c;
		}
	}
	if (!token.empty()) { args.push_back(token); }

	if (args.empty()) { return; }

	std::string key = args.front();
	std::transform(key.begin(), key.end(), key.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

	const auto it = m_Commands.find(key);
	if (it != m_Commands.end())
	{
		it->second(args);
	}
	else
	{
		LogToConsole("unknown command: " + key + " (try help)");
	}
}
