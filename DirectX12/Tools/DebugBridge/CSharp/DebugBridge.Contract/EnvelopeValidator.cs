using System.Text.Json;


using System;

namespace DebugBridge.Contract;

/// <summary>
/// DebugBridge Protocol v1 (docs/debug_bridge_protocol.md) のエンベロープ契約を検証する.
/// C++側 DebugBridgeContractTest.cpp と同一ルール.
/// </summary>
public static class EnvelopeValidator
{
    public const int ProtocolMajorVersion = 1;
    private static readonly string[] ValidTypes = { "request", "response", "event" };

    public static ValidationResult Validate(JsonElement message)
    {
        if (!message.TryGetProperty("protocolVersion", out var version))
            return ValidationResult.Fail("protocolVersion", "missing");
        if (version.ValueKind != JsonValueKind.Number || !version.TryGetInt32(out var major))
            return ValidationResult.Fail("protocolVersion", "must be integer");
        if (major != ProtocolMajorVersion)
            return ValidationResult.Fail("protocolVersion",
                $"major mismatch: expected {ProtocolMajorVersion}, got {major}");

        if (!message.TryGetProperty("type", out var typeEl))
            return ValidationResult.Fail("type", "missing");
        var type = typeEl.GetString() ?? "";
        if (Array.IndexOf(ValidTypes, type) < 0)
            return ValidationResult.Fail("type", $"must be request/response/event, got: {type}");

        // idはrequest/responseでのみ必須(eventでは省略可: docs §3.1).
        if (type != "event")
        {
            if (!message.TryGetProperty("id", out var idEl) ||
                idEl.ValueKind != JsonValueKind.String ||
                string.IsNullOrEmpty(idEl.GetString()))
                return ValidationResult.Fail("id", "request/response requires non-empty string id");
        }

        switch (type)
        {
            case "request":
                if (!message.TryGetProperty("command", out var commandEl))
                    return ValidationResult.Fail("command", "request requires command");
                var command = commandEl.GetString() ?? "";
                var isReserved = command is "ping" or "handshake";
                if (!isReserved && !command.Contains('.'))
                    return ValidationResult.Fail("command",
                        $"expected '<domain>.<action>' or reserved name: {command}");
                break;

            case "response":
                if (!message.TryGetProperty("ok", out var okEl) || okEl.ValueKind != JsonValueKind.True && okEl.ValueKind != JsonValueKind.False)
                    return ValidationResult.Fail("ok", "response requires boolean ok");
                var ok = okEl.GetBoolean();
                if (!ok)
                {
                    if (!message.TryGetProperty("error", out var errorEl))
                        return ValidationResult.Fail("error", "required when ok=false");
                    if (!errorEl.TryGetProperty("code", out var codeEl) ||
                        string.IsNullOrEmpty(codeEl.GetString()))
                        return ValidationResult.Fail("error.code", "required when ok=false");
                    if (!errorEl.TryGetProperty("message", out _))
                        return ValidationResult.Fail("error.message", "required when ok=false");
                }
                break;

            case "event":
                if (!message.TryGetProperty("event", out var eventEl) ||
                    string.IsNullOrEmpty(eventEl.GetString()))
                    return ValidationResult.Fail("event", "event message requires event name");
                break;
        }

        return ValidationResult.Pass();
    }
}

