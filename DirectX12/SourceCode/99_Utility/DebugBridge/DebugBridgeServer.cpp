#include "DebugBridgeServer.h"

#if _DEBUG

#include <windows.h>

#include <algorithm>

DebugBridgeServer::~DebugBridgeServer()
{
	Stop();
}

bool DebugBridgeServer::Start(const std::wstring& PipeName)
{
	if (m_Running) { return true; }

	m_Shutdown        = false;
	m_ForceDisconnect = false;
	{
		std::lock_guard<std::mutex> lock(m_Mutex);
		m_RequestQueue.clear();
		m_ResponseQueue.clear();
	}

	// パイプ名はスレッドへ値渡しする(lifetimeを共有しないため).
	m_Thread = std::thread(&DebugBridgeServer::ThreadMain, this, PipeName);
	m_Running = true;
	return true;
}

void DebugBridgeServer::Stop()
{
	if (!m_Running) { return; }

	m_Shutdown = true;
	if (m_Thread.joinable()) { m_Thread.join(); }
	m_Running = false;
}

// 受信済み要求を実行して応答を積む(必ずメインスレッドから呼ぶ).
void DebugBridgeServer::Pump()
{
	std::deque<PendingRequest> batch;
	{
		std::lock_guard<std::mutex> lock(m_Mutex);
		batch.swap(m_RequestQueue);
	}

	constexpr size_t kMaxBatchPerFrame = 16; // 溢れた要求は次フレームへ持ち越し.
	size_t processed = 0;

	while (!batch.empty() && processed < kMaxBatchPerFrame)
	{
		const PendingRequest request = batch.front();
		batch.pop_front();
		++processed;

		nlohmann::json response = {
			{ "protocolVersion", 1 },
			{ "type", "response" },
			{ "id", request.Id },
		};

		try
		{
			if (request.Command == "ping")
			{
				response["ok"] = true;
				response["payload"] = { { "pong", true } };
			}
			else if (request.Command == "handshake")
			{
				response["ok"] = true;
				response["payload"] = {
					{ "server", "SenzanGame" },
					{ "commands", { "handshake", "ping", "bridge.get_runtime_info", "debugbridge.get_snapshot" } },
				};
			}
			else if (request.Command == "bridge.get_runtime_info")
			{
				response["ok"] = true;
				response["payload"] = m_RuntimeInfoResolver ? m_RuntimeInfoResolver() : nlohmann::json{};
			}
			else if (request.Command == "debugbridge.get_snapshot")
			{
				nlohmann::json payload = m_RuntimeInfoResolver ? m_RuntimeInfoResolver() : nlohmann::json{};
				payload["snapshot"] = true;
				response["ok"] = true;
				response["payload"] = payload;
			}
			else
			{
				response["ok"] = false;
				response["error"] = { { "code", "E_UNKNOWN_COMMAND" }, { "message", "unknown command: " + request.Command } };
			}
		}
		catch (...)
		{
			response["ok"] = false;
			response["error"] = { { "code", "E_INTERNAL" }, { "message", "unexpected exception in command handler" } };
		}

		std::lock_guard<std::mutex> lock(m_Mutex);
		m_ResponseQueue.push_back(response.dump() + "\n"); // 応答は1メッセージ1行.
	}
}

// 受信した1行JSONを解析して要求キューへ積む(不正入力は握りつぶしゲームを落とさない).
void DebugBridgeServer::EnqueueParsedLine(const std::string& Line)
{
	nlohmann::json parsed = nlohmann::json::parse(Line, nullptr, /*allow_exceptions=*/false);
	if (parsed.is_discarded()) { return; }
	if (!parsed.contains("type") || parsed["type"] != "request") { return; } // ClientはRequestのみ送る.

	const int major_version = parsed.value("protocolVersion", 0);
	if (major_version != 1)
	{
		// メジャー不一致: E_PROTOCOL_VERSION応答後に強制切断する(docs §4.2).
		const std::string id = parsed.contains("id") && parsed["id"].is_string() ? parsed["id"].get<std::string>() : "";
		nlohmann::json response = {
			{ "protocolVersion", 1 },
			{ "type", "response" },
			{ "id", id },
			{ "ok", false },
			{ "error", { { "code", "E_PROTOCOL_VERSION" }, { "message", "unsupported major version" } } },
		};
		std::lock_guard<std::mutex> lock(m_Mutex);
		m_ResponseQueue.push_back(response.dump() + "\n");
		m_ForceDisconnect = true;
		return;
	}

	PendingRequest request{};
	request.Id      = parsed.contains("id") && parsed["id"].is_string() ? parsed["id"].get<std::string>() : "";
	request.Command = parsed.contains("command") && parsed["command"].is_string() ? parsed["command"].get<std::string>() : "";

	std::lock_guard<std::mutex> lock(m_Mutex);
	m_RequestQueue.push_back(std::move(request));
}

// 通信スレ本体: 接続待ち→行読み取り/応答送信→切断→再接続待ちを繰り返す.
// NOTE: ゲーム状態には一切触れず、キューの受け渡しのみを行う(スレッド境界).
void DebugBridgeServer::ThreadMain(std::wstring PipeName)
{
	HANDLE pipe = CreateNamedPipeW(
		PipeName.c_str(),
		PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
		PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
		1,     // 同時接続は1クライアント(v1. 待ち受け中の2番目は接続できない).
		4096, 4096,
		0, nullptr);

	if (pipe == INVALID_HANDLE_VALUE)
	{
		m_Running = false; // 作成失敗(既に起動済み等). ゲームには影響させず終了.
		return;
	}

	OVERLAPPED connect_overlapped{};
	connect_overlapped.hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);

	while (!m_Shutdown)
	{
		BOOL connected = ConnectNamedPipe(pipe, &connect_overlapped);
		if (!connected)
		{
			const DWORD error = GetLastError();
			if (error == ERROR_PIPE_CONNECTED)
			{
				// 接続待ち開始前にClientが繋がっていた場合もそのまま処理へ進む.
			}
			else if (error == ERROR_IO_PENDING)
			{
				bool signalled = false;
				while (!m_Shutdown)
				{
					if (WaitForSingleObject(connect_overlapped.hEvent, 100) == WAIT_OBJECT_0) { signalled = true; break; }
				}
				if (m_Shutdown) { CancelIoEx(pipe, &connect_overlapped); break; }
				DWORD bytes = 0;
				if (!signalled || !GetOverlappedResult(pipe, &connect_overlapped, &bytes, FALSE)) { continue; }
				connected = TRUE;
			}
			else
			{
				Sleep(50);
				continue;
			}
		}

		if (m_Shutdown) { CancelIoEx(pipe, &connect_overlapped); break; }

		// --- 接続済み: 行単位の読み取りと応答送信 ---
		std::string buffer;
		char chunk[1024];
		DWORD read_bytes = 0;
		bool disconnected = false;

		while (!m_Shutdown && !disconnected && !m_ForceDisconnect.load())
		{
			DWORD available = 0;
			if (!PeekNamedPipe(pipe, nullptr, 0, nullptr, &available, nullptr))
			{
				disconnected = true; // Client切断.
				break;
			}

			// 応答の送信(入力の有無に関係なく毎回フラッシュする).
			for (;;)
			{
				std::string line_to_send;
				{
					std::lock_guard<std::mutex> lock(m_Mutex);
					if (m_ResponseQueue.empty()) { break; }
					line_to_send.swap(m_ResponseQueue.front());
					m_ResponseQueue.pop_front();
				}
				DWORD written = 0;
				if (!WriteFile(pipe, line_to_send.data(), static_cast<DWORD>(line_to_send.size()), &written, nullptr))
				{
					disconnected = true;
					break;
				}
			}
			if (disconnected) { break; }

			if (available == 0) { Sleep(5); continue; } // 入力無し. 少し待って再ポーリング.

			if (!ReadFile(pipe, chunk, sizeof(chunk), &read_bytes, nullptr) || read_bytes == 0)
			{
				disconnected = true;
				break;
			}
			buffer.append(chunk, read_bytes);

			size_t newline_pos = 0;
			while ((newline_pos = buffer.find('\n')) != std::string::npos)
			{
				std::string line = buffer.substr(0, newline_pos);
				buffer.erase(0, newline_pos + 1);
				if (!line.empty()) { EnqueueParsedLine(line); }
			}
		}

		FlushFileBuffers(pipe);
		DisconnectNamedPipe(pipe); // 切断→次のClientの再接続待ちへ戻る.
		m_ForceDisconnect = false;
	}

	CloseHandle(connect_overlapped.hEvent);
	CloseHandle(pipe);
}

#endif // _DEBUG
