// DebugBridge契約テスト(C++側. samples/配下の共有サンプルJSONを検証する).
// ビルド例(DirectX12ディレクトリから):
//   cl /nologo /EHsc /std:c++20 /W4 /I Data\Library ^
//     tools\DebugBridge\DebugBridgeContractTest.cpp
// 実行: tools\DebugBridge\DebugBridgeContractTest.exe [samplesディレクトリ]
//
// プロトコル原本: docs/debug_bridge_protocol.md (v1)

#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "json/json.hpp"

namespace {

	using nlohmann::json;

	struct ValidationResult
	{
		bool Ok = true;
		std::string Field;      // 失敗フィールド(空=全体).
		std::string Message;    // 失敗理由.
	};

	void Fail(ValidationResult& Result, const std::string& Field, const std::string& Message)
	{
		Result.Ok = false;
		if (Result.Field.empty()) { Result.Field = Field; }
		Result.Message = Message;
	}

	// エンベロープ共通+種別ごとの必須フィールドを検証する(docs §3, §5, §6).
	ValidationResult ValidateEnvelope(const json& Message)
	{
		ValidationResult result{};

		if (!Message.contains("protocolVersion")) { Fail(result, "protocolVersion", "missing"); return result; }
		if (!Message["protocolVersion"].is_number_integer()) { Fail(result, "protocolVersion", "must be integer"); return result; }
		if (Message["protocolVersion"].get<int>() != 1) {
			Fail(result, "protocolVersion",
				"major mismatch: expected 1, got " + std::to_string(Message["protocolVersion"].get<int>()));
			return result;
		}

		if (!Message.contains("type")) { Fail(result, "type", "missing"); return result; }
		const std::string type = Message.value("type", "");
		if (type != "request" && type != "response" && type != "event") {
			Fail(result, "type", "must be request/response/event, got: " + type);
			return result;
		}

		// idはrequest/responseでのみ必須(eventでは省略可: docs §3.1).
		if (type != "event")
		{
			if (!Message.contains("id") || !Message["id"].is_string() || Message["id"].get<std::string>().empty()) {
				Fail(result, "id", "request/response requires non-empty string id");
				return result;
			}
		}

		if (type == "request")
		{
			if (!Message.contains("command")) { Fail(result, "command", "request requires command"); return result; }
			const std::string command = Message["command"].get<std::string>();
			if (command.find('.') == std::string::npos && command != "ping" && command != "handshake") {
				Fail(result, "command", "expected '<domain>.<action>' or reserved name: " + command);
			}
		}
		else if (type == "response")
		{
			if (!Message.contains("ok") || !Message["ok"].is_boolean()) { Fail(result, "ok", "response requires boolean ok"); return result; }
			const bool ok = Message["ok"].get<bool>();
			if (!ok) {
				if (!Message.contains("error")) { Fail(result, "error", "required when ok=false"); return result; }
				if (!Message["error"].contains("code") || Message["error"]["code"].get<std::string>().empty()) {
					Fail(result, "error.code", "required when ok=false");
				}
				if (!Message["error"].contains("message")) { Fail(result, "error.message", "required when ok=false"); }
			}
		}
		else // event
		{
			if (!Message.contains("event") || Message["event"].get<std::string>().empty()) {
				Fail(result, "event", "event message requires event name");
			}
		}

		return result;
	}

} // namespace

int main(int ArgumentCount, char** p_Arguments)
{
	std::string samples_dir = "tools/DebugBridge/samples";
	if (ArgumentCount > 1) { samples_dir = p_Arguments[1]; }

	int failures = 0, passed = 0;

	for (const auto& entry : std::filesystem::directory_iterator(samples_dir))
	{
		if (entry.path().extension() != ".json") { continue; }

		std::ifstream file(entry.path());
		if (!file.is_open()) { continue; }

		json message = json::parse(file, nullptr, /*allow_exceptions=*/false);
		if (message.is_discarded())
		{
			std::cout << "[FAIL] " << entry.path().filename().string()
				<< ": field '(parse)' - invalid JSON" << std::endl;
			++failures;
			continue;
		}

		const ValidationResult result = ValidateEnvelope(message);
		if (result.Ok)
		{
			++passed;
			std::cout << "[PASS] " << entry.path().filename().string() << std::endl;
		}
		else
		{
			++failures;
			std::cout << "[FAIL] " << entry.path().filename().string()
				<< ": field '" << result.Field << "' - " << result.Message << std::endl;
		}
	}

	std::cout << "\nC++ contract test: " << passed << " passed, " << failures << " failed" << std::endl;
	return failures == 0 ? 0 : 1;
}
