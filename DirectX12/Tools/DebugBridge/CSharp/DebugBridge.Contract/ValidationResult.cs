namespace DebugBridge.Contract;

/// <summary>検証結果. 失敗時にどのフィールドが不一致かを保持する.</summary>
public sealed class ValidationResult
{
    public bool Ok { get; init; } = true;
    public string Field { get; init; } = "";
    public string Message { get; init; } = "";

    public static ValidationResult Pass() => new();
    public static ValidationResult Fail(string field, string message) =>
        new() { Ok = false, Field = field, Message = message };
}
