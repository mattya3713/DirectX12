#include "DebugLog.h"

#include <iostream>

void DebugLog::LogInfo(const std::string& Message)
{
	AddEntry(LogLevel::Info, Message);
}

void DebugLog::LogWarning(const std::string& Message)
{
	AddEntry(LogLevel::Warning, Message);
}

void DebugLog::LogError(const std::string& Message)
{
	AddEntry(LogLevel::Error, Message);
}

void DebugLog::AddEntry(LogLevel Level, const std::string& Message)
{
	m_Entries.push_back({ Level, Message });
	while (m_Entries.size() > MAX_ENTRIES)
	{
		m_Entries.pop_front();
	}

	if (Level == LogLevel::Error)
	{
		std::cerr << "[ERROR] " << Message << '\n';
	}
	else if (Level == LogLevel::Warning)
	{
		std::cout << "[WARNING] " << Message << '\n';
	}
	else
	{
		std::cout << "[INFO] " << Message << '\n';
	}
}
