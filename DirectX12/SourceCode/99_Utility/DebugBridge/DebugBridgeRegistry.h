#pragma once

#if _DEBUG

#include <cassert>
#include <functional>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

#include "json/json.hpp"

/**********************************************************************************
* @author    : Coder 玄武(閃斬 Production Loop).
* @date      : 2026-08-23.
* @brief     : DebugBridgeのコマンド/クエリ明示登録レジストリ.
*            : ゲーム内部機能をC# Editorへ安全に公開するための窓口.
*            : - 未登録キーはE_UNKNOWN_COMMAND、ハンドラ内の型不一致等は
*            :   BridgeError(code/message付き)として応答側へ伝わる.
*            : - 登録はサーバー起動前(メインスレッド)に限定し、実行もPump()=
*            :   メインスレッド上でのみ行うためロック不要.
*            : - Combat Tuning/Particle/UILayout等の後続EditorはこのRegister()
*            :   を呼ぶだけで公開できる(拡張点).
**********************************************************************************/

// ハンドラ内エラー(code/message付きで応答される).
class DebugBridgeError : public std::runtime_error
{
public:
	DebugBridgeError(std::string Code, const std::string& Message)
		: std::runtime_error(Message), m_Code(std::move(Code))
	{
	}

	const std::string& Code() const noexcept { return m_Code; }

private:
	std::string m_Code;
};

class DebugBridgeRegistry final
{
public:
	// Payload(JSONオブジェクト)を受け取り、結果のpayload相当JSONを返す.
	// パラメータ不正はDebugBridgeError("E_INVALID_PARAMS", ...)で報告する.
	using Handler = std::function<nlohmann::json(const nlohmann::json& Payload)>;

	struct Entry
	{
		std::string Name;
		std::string Description;
	};

	// コマンド/クエリを登録する(重複登録はアサート. 登録は起動前に集約すること).
	void Register(const std::string& Name, const std::string& Description, Handler Handler_)
	{
		const bool inserted = m_Handlers.emplace(Name, Item{ Description, std::move(Handler_) }).second;
		if (!inserted) { assert(false && "DebugBridgeRegistry: duplicate command registration"); }
	}

	bool Contains(const std::string& Name) const { return m_Handlers.find(Name) != m_Handlers.end(); }

	// 登録済み一覧を取得する(protocol.list_commands / handshake応答用).
	std::vector<Entry> GetEntries() const
	{
		std::vector<Entry> entries;
		for (const auto& [name, item] : m_Handlers) { entries.push_back({ name, item.Description }); }
		return entries;
	}

	// 登録済みハンドラを実行する(未登録時はDebugBridgeError(E_UNKNOWN_COMMAND)).
	nlohmann::json Execute(const std::string& Name, const nlohmann::json& Payload) const
	{
		const auto it = m_Handlers.find(Name);
		if (it == m_Handlers.end()) { throw DebugBridgeError("E_UNKNOWN_COMMAND", "unknown command: " + Name); }
		return it->second.Handler_(Payload);
	}

private:
	struct Item
	{
		std::string Description;
		Handler     Handler_;
	};

	std::map<std::string, Item> m_Handlers;
};

#endif // _DEBUG
