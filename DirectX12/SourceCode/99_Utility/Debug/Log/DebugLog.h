#pragma once

#include <cstddef>
#include <deque>
#include <string>

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/14.
* @brief     : ImGui表示と標準出力に共有する実行時ログを保持する.
* @pattern   : ServiceLocator経由でMainが所有するサービス.
**********************************************************************************/

// ログの重要度.
enum class LogLevel
{
	Info,
	Warning,
	Error
};

// 表示用に保持するログ一件分.
struct LogEntry
{
	LogLevel    Level;
	std::string Message;
};

class DebugLog final
{
public:
	DebugLog() = default;
	~DebugLog() = default;

	DebugLog(const DebugLog&)            = delete;
	DebugLog& operator=(const DebugLog&) = delete;
	DebugLog(DebugLog&&)                 = delete;
	DebugLog& operator=(DebugLog&&)      = delete;

	// 情報ログを追加する.
	void LogInfo(const std::string& Message);
	// 警告ログを追加する.
	void LogWarning(const std::string& Message);
	// エラーログを追加する.
	void LogError(const std::string& Message);

	// 保持中のログを表示用に返す.
	const std::deque<LogEntry>& GetEntries() const noexcept { return m_Entries; }
	// 保持中のログを消去する.
	void Clear() noexcept { m_Entries.clear(); }

private:
	// ログを保持し、標準出力にも転送する.
	void AddEntry(LogLevel Level, const std::string& Message);

private:
	std::deque<LogEntry> m_Entries;

	static constexpr std::size_t MAX_ENTRIES = 500;
};
