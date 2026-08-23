#pragma once

/**********************************************************************************
* @author    : mattya3713 / Coder 青龍(せいりゅう).
* @date      : 2026/08/14 / 2026-08-23 コマンド入力機能追加.
* @brief     : 実行時ログ表示+コマンド入力(_DEBUG限定)のImGuiコンソールパネル.
*            : F4でトグル表示。RegisterCommand()で任意のコマンドを登録できる.
*            : インスタンスは初回Draw時に生成されServiceLocatorへ登録される
*            : (既存の呼び出し箇所(DebugConsole::Draw())を変更しないため).
**********************************************************************************/

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

class DebugConsole final
{
public:
	using CommandArgs = std::vector<std::string>;
	using CommandHandler = std::function<void(const CommandArgs&)>;

	DebugConsole() = default;

	// 保持中のログ+コマンド入力欄を毎フレーム表示する(F4でトグル. 入力実行は_DEBUG限定).
	static void Draw();

	// コマンドを登録する(Nameは小文字推奨. 実行時の引数はArgs[0]=コマンド名).
	void RegisterCommand(const std::string& Name, CommandHandler Handler);

	// コンソールの表示/非表示を切り替える.
	void ToggleVisible() noexcept { m_IsVisible = !m_IsVisible; }

private:
	// ビルトインコマンド(help/god/kill_boss等)を初回のみ登録する.
	void EnsureBuiltinCommands();

	// 入力文字列を解析して対応するコマンドを実行する.
	void Execute(const std::string& Line);

	std::unordered_map<std::string, CommandHandler> m_Commands;      // 登録済みコマンド(キー=小文字名).
	std::string                                     m_InputBuffer;   // 入力中の文字列.
	bool                                            m_IsVisible = false;          // 入力欄の表示状態.
	bool                                            m_IsBuiltinsRegistered = false; // ビルトイン登録済み.
};
