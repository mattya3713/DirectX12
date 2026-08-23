// DebugBridgeサーバーのスタンドアロン動作確認ハーネス(ゲーム起動なしで検証する).
// PowerShellクライアント(PipeClientSmokeTest.ps1)を子プロセスで起動し、
// Pump()を回しながら応答が正しく返るかを確認する.
// ビルド例(DirectX12ディレクトリから):
//   cl /nologo /EHsc /std:c++20 /W4 /I SourceCode /I Data\Library ^
//     tools\DebugBridge\ServerSmokeHarness.cpp ^
//     SourceCode\99_Utility\DebugBridge\DebugBridgeServer.cpp

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <thread>

#include "../../SourceCode/99_Utility/DebugBridge/DebugBridgeServer.h"

int main()
{
	// 前回の成果物を掃除.
	std::remove("smoke_out.txt");
	std::remove("smoke_done.txt");

	DebugBridgeServer server;

	server.SetRuntimeInfoResolver([]() -> nlohmann::json {
		return {
			{ "player", { { "hp", 80.0 }, { "maxHp", 100.0 }, { "stateId", 0 }, { "combo", 3 } } },
			{ "boss",   { { "hp", 55.0 }, { "maxHp", 100.0 }, { "stateId", 2 } } },
			{ "timeScale", 1.0 },
			{ "paused", false },
		};
	});

	if (!server.Start(L"\\\\.\\pipe\\senzan.debugbridge.control.v1"))
	{
		std::cout << "[FAIL] server start" << std::endl;
		return 1;
	}

	// PowerShellクライアントを非同期で起動(検証結果はsmoke_out.txtとsmoke_done.txt経由で受け取る).
	std::thread client_thread([] {
		std::system("powershell -NoProfile -ExecutionPolicy Bypass -File tools\\DebugBridge\\PipeClientSmokeTest.ps1 > smoke_out.txt 2>&1");
		std::ofstream done("smoke_done.txt");
		done << "done";
	});

	// Client処理が完了するまでPumpを回し続ける(最大15秒).
	const auto start = std::chrono::steady_clock::now();
	bool done = false;
	while (std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count() < 15.0)
	{
		server.Pump();
		if (std::filesystem::exists("smoke_done.txt")) { done = true; break; }
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}

	server.Stop();
	client_thread.join();

	if (!done)
	{
		std::cout << "[FAIL] client did not finish in time" << std::endl;
		return 1;
	}

	std::ifstream out("smoke_out.txt");
	std::string content((std::istreambuf_iterator<char>(out)), std::istreambuf_iterator<char>());
	std::cout << content << std::endl;

	const bool pass = content.find("SMOKE PASS") != std::string::npos;
	std::cout << (pass ? "HARNESS PASS" : "HARNESS FAIL") << std::endl;

	std::remove("smoke_out.txt");
	std::remove("smoke_done.txt");
	return pass ? 0 : 1;
}
