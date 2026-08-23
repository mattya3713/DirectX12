using System.Text.Json;
using DebugBridge.Contract;

// samplesディレクトリ(既定: exeから ../../../../samples). 引数で上書き可.
var samples_dir = args.Length > 0 ? args[0]
    : Path.Combine(AppContext.BaseDirectory, "..", "..", "..", "..", "..", "samples");
samples_dir = Path.GetFullPath(samples_dir);

int passed = 0, failures = 0;
foreach (var file in Directory.EnumerateFiles(samples_dir, "*.json").OrderBy(f => f))
{
    try
    {
        using var doc = JsonDocument.Parse(File.ReadAllText(file));
        var result = EnvelopeValidator.Validate(doc.RootElement);
        if (result.Ok) { ++passed; Console.WriteLine($"[PASS] {Path.GetFileName(file)}"); }
        else
        {
            ++failures;
            Console.WriteLine($"[FAIL] {Path.GetFileName(file)}: field '{result.Field}' — {result.Message}");
        }
    }
    catch (JsonException ex)
    {
        ++failures;
        Console.WriteLine($"[FAIL] {Path.GetFileName(file)}: field '(parse)' — {ex.Message}");
    }
}

Console.WriteLine($"\nC# contract test: {passed} passed, {failures} failed");
return failures == 0 ? 0 : 1;
