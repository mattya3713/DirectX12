#pragma once

#if _DEBUG

#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "DebugBridgeRegistry.h"
#include "json/json.hpp"

/**********************************************************************************
* @author    : Coder 玄武(閃斬 Production Loop).
* @date      : 2026-08-23.
* @brief     : DebugBridge Protocol v1(docs/debug_bridge_protocol.md)のC++側
*            : Named Pipeサーバー(_DEBUGビルド限定).
*            : スレッド境界: 通信スレッドは受信/送信のみを行い、要求の実行は
*            : Pump()経由でゲームメインスレッドに集約する(ゲーム状態への安全な
*            : アクセスは必ずPump内で行う)。Client切断・再接続・不正入力でも
*            : ゲームは停止しない。
**********************************************************************************/

class DebugBridgeServer final
{
public:
	DebugBridgeServer() = default;
	~DebugBridgeServer();

	DebugBridgeServer(const DebugBridgeServer&)            = delete;
	DebugBridgeServer& operator=(const DebugBridgeServer&) = delete;

	// サーバーを起動する(通信スレッド生成+パイプ待ち受け開始).
	bool Start(const std::wstring& PipeName);
	// サーバーを停止する(スレッド終了+パイプ破棄. 二重呼び出し安全).
	void Stop();
	bool IsRunning() const noexcept { return m_Running; }

	// GetRuntimeInfo/GetSnapshot応答の内容を提供する(MainScene等から設定する.
	// 呼び出しは必ずPump()=メインスレッド上で行われる).
	using RuntimeInfoResolver = std::function<nlohmann::json()>;
	void SetRuntimeInfoResolver(RuntimeInfoResolver Resolver) { m_RuntimeInfoResolver = std::move(Resolver); }

	// 受信済み要求を実行し応答を積む(毎フレーム、メインスレッドから呼ぶ).
	void Pump();

	// コマンド/クエリの明示登録窓口(後続Editorはここへ登録する).
	DebugBridgeRegistry& GetRegistry() noexcept { return m_Registry; }

private:
	// 通信スレ本体(接続待ち→読み書きループ→切断→再接続待ち).
	void ThreadMain(std::wstring PipeName);
	// 1行の受信JSONを解析して要求キューへ積む(不正入力は握りつぶす).
	void EnqueueParsedLine(const std::string& Line);
	// コマンドを実行して応答JSON文字列を返す(メインスレッドのみ).
	std::string ExecuteCommand(const std::string& Id, const std::string& Command);

	std::thread        m_Thread;
	std::atomic<bool>  m_Shutdown{ false };
	std::atomic<bool>  m_Running{ false };
	std::atomic<bool>  m_ForceDisconnect{ false }; // プロトコル違反時の強制切断要求.

	std::mutex m_Mutex;
	struct PendingRequest
	{
		std::string Id;
		std::string Command;
		nlohmann::json Payload;
	};
	std::deque<PendingRequest> m_RequestQueue; // 通信スレッド→メインスレッド.
	std::deque<std::string>    m_ResponseQueue; // メインスレッド→通信スレッド(送信待ち行).
	RuntimeInfoResolver m_RuntimeInfoResolver;
	DebugBridgeRegistry m_Registry; // 登録済みコマンド/クエリ.
	void RegisterDefaultCommands();
};

#endif // _DEBUG
