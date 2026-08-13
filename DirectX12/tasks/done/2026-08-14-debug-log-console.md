# Current Task

## Goal

Add a Unity-Console-like runtime log display: an `ImGui` window that shows scrolling
`Info`/`Warning`/`Error` log entries, color-coded, with a Clear button. For now this
is display-only (no command input) — the user said "いったんログ表示だけImguiへ出力
するように" (for now, just get log display working in ImGui).

## Background

Neither this project nor the older reference project (`Senzan`) has an existing
console/command system. However, `Senzan` has a well-shaped logging singleton worth
porting the *shape* of (not copy-pasting — Senzan uses `Singleton<T>`, this project's
established convention is ServiceLocator, see `DESIGN.md`'s "マネージャーの所有方針").
Reference file (read-only, do not modify):
`C:\Users\green\source\C++\Senzan\Senzan\SourceCode\System\Singleton\Debug\Log\DebugLog.h`
— it has `LogLevel{Info,Warning,Error}`, `LogInfo/LogWarning/LogError(message)`
methods, and writes to `std::cout`/`std::cerr` with a `[INFO]`/`[WARNING]`/`[ERROR]`
prefix. Port that same public API shape, but store entries in-memory too (for the
ImGui display) rather than only writing to stdout.

This project's existing debug-tooling wiring pattern (confirmed in `Main.cpp`) is
**not** `#if _DEBUG`-gated at the call site — `ImGuiManager`/`DebugHud` are
constructed, registered, and drawn unconditionally in both Debug and Release builds
(unlike some other things in this codebase, e.g. the F1 scene-switch key, which *is*
`#if _DEBUG`-gated). Follow the `DebugHud`/`ImGuiManager` pattern exactly for this
task — do **not** wrap this feature in `#if _DEBUG`.

## Scope

- `SourceCode/99_Utility/Debug/Log/DebugLog.h` (new)
- `SourceCode/99_Utility/Debug/Log/DebugLog.cpp` (new)
- `SourceCode/99_Utility/Debug/Imgui/DebugConsole.h` (new)
- `SourceCode/99_Utility/Debug/Imgui/DebugConsole.cpp` (new)
- `SourceCode/00_Game/00_GameLoop/Main.h`
- `SourceCode/00_Game/00_GameLoop/Main.cpp`
- `DirectX12.vcxproj` / `DirectX12.vcxproj.filters` (register the 4 new files)

## Out of Scope

- No command input / parsing / dispatch table — display only, this iteration.
- Don't modify anything under `Senzan/` — read-only reference.
- Don't add logging calls from other systems yet (e.g. don't sprinkle
  `DebugLog::LogInfo` calls into Collision/Combat/etc.) — this task is just building
  the log storage + display mechanism itself. Once it exists, wiring it into other
  systems is separate follow-up work.
- Don't add a command history, autocomplete, or any Console-specific input widget —
  explicitly deferred per the user's "いったんログ表示だけ" instruction.

## Implementation Requirements

### 1. `DebugLog` (new class, ServiceLocator-registered — not `Singleton<T>`)

`SourceCode/99_Utility/Debug/Log/DebugLog.h`:
```cpp
#pragma once

#include <deque>
#include <string>

enum class LogLevel
{
	Info,
	Warning,
	Error
};

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

	void LogInfo(const std::string& Message);
	void LogWarning(const std::string& Message);
	void LogError(const std::string& Message);

	// 表示用に蓄積済みの全エントリを取得する.
	const std::deque<LogEntry>& GetEntries() const noexcept { return m_Entries; }

	// 表示をクリアする(蓄積内容を破棄するだけ. 標準出力への既出力には影響しない).
	void Clear() noexcept { m_Entries.clear(); }

private:
	void AddEntry(LogLevel Level, const std::string& Message);

private:
	std::deque<LogEntry> m_Entries;

	static constexpr size_t MAX_ENTRIES = 500; // 際限なく増え続けないよう上限を設ける.
};
```

`DebugLog.cpp`: implement `LogInfo/LogWarning/LogError` as thin calls to
`AddEntry(LogLevel::X, Message)`. `AddEntry` should:
1. Push `{Level, Message}` to `m_Entries` (`push_back`).
2. If `m_Entries.size() > MAX_ENTRIES`, `pop_front()` until back under the cap
   (or just once, since one entry is added per call).
3. Also mirror to `std::cout`/`std::cerr` matching Senzan's `Log::OutputLog` exactly:
   prefix `[INFO] `/`[WARNING] `/`[ERROR] ` + message + newline, errors to
   `std::cerr`, everything else to `std::cout`.

### 2. `DebugConsole` (new ImGui panel, mirrors `DebugHud`'s exact shape)

`SourceCode/99_Utility/Debug/Imgui/DebugConsole.h`:
```cpp
#pragma once

class DebugConsole
{
public:
	static void Draw();
};
```

`DebugConsole.cpp` — read `DebugHud.cpp` first for the exact style/pattern
(`ImGui::SetNextWindowPos(..., ImGuiCond_FirstUseEver)` + `ImGui::Begin(...,
ImGuiWindowFlags_AlwaysAutoResize)` there is NOT quite right for a scrolling log —
use a fixed initial size instead of auto-resize for this one, e.g.
`ImGui::SetNextWindowSize(ImVec2(500, 300), ImGuiCond_FirstUseEver)`). Content:

```cpp
void DebugConsole::Draw()
{
	DebugLog* p_debug_log = ServiceLocator::Get<DebugLog>();
	if (!p_debug_log) { return; }

	// 他のデバッグウィンドウ(Debug HUD等)と重ならない初期位置(初回起動時のみ).
	ImGui::SetNextWindowPos(ImVec2(20.0f, 260.0f), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(500.0f, 300.0f), ImGuiCond_FirstUseEver);
	ImGui::Begin("Console");

	if (ImGui::Button("Clear")) {
		p_debug_log->Clear();
	}

	ImGui::Separator();

	ImGui::BeginChild("ConsoleScroll", ImVec2(0.0f, 0.0f), true);
	for (const LogEntry& entry : p_debug_log->GetEntries())
	{
		ImVec4 color;
		switch (entry.Level)
		{
			case LogLevel::Warning: color = ImVec4(1.0f, 0.85f, 0.2f, 1.0f); break;
			case LogLevel::Error:   color = ImVec4(1.0f, 0.3f,  0.3f, 1.0f); break;
			default:                color = ImVec4(1.0f, 1.0f,  1.0f, 1.0f); break;
		}
		ImGui::TextColored(color, "%s", entry.Message.c_str());
	}

	// 最下部までスクロールしていたら、追加分にも自動で追従する.
	if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
		ImGui::SetScrollHereY(1.0f);
	}
	ImGui::EndChild();

	ImGui::End();
}
```
Check the actual `SetNextWindowPos` coordinates already used by `DebugHud`/`Scene`/
other windows first (grep `SetNextWindowPos` across `SourceCode/99_Utility/Debug/
Imgui/` and `SourceCode/00_Game/00_Scene/`) and adjust the position above if `(20,
260)` would overlap an existing window — the goal is just "doesn't overlap on first
launch," exact coordinates aren't important.

Note: `entry.Message` may itself contain `%` characters (arbitrary log text) — using
`"%s"` with the message as an argument (not as the format string itself) avoids any
format-string injection issue. Keep it that way, don't pass `entry.Message.c_str()`
directly as the format argument.

### 3. Wire into `Main.h`/`Main.cpp`

`Main.h`: add `class DebugLog;` to the forward-declaration block (alphabetical-ish
placement near the others is fine, match existing order style), and add
`std::unique_ptr<DebugLog> m_upDebugLog;` to the private members section (add a
one-line comment matching this file's existing comment style for each member).

`Main.cpp`:
- Add `#include "99_Utility/Debug/Log/DebugLog.h"` and
  `#include "99_Utility/Debug/Imgui/DebugConsole.h"` near the other similar includes.
- In `Main::Create()`, construct and register `DebugLog` — place it right after the
  `ImGuiManager` construction block (before `CameraManager`), since it's simple and
  order-independent:
  ```cpp
	// ログ蓄積(DebugConsoleが表示に使う)を構築・登録.
	m_upDebugLog = std::make_unique<DebugLog>();
	ServiceLocator::Provide<DebugLog>(m_upDebugLog.get());
  ```
- In `Main::Draw()`, add `DebugConsole::Draw();` right after the existing
  `DebugHud::Draw();` line.
- In `Main::Release()`, add teardown matching the existing pattern used for the
  other simple managers (e.g. `m_upCollisionDetector`'s block) — place it wherever
  fits the existing teardown order (no strict dependency on anything else, so
  anywhere in the sequence is safe):
  ```cpp
	if (m_upDebugLog) {
		ServiceLocator::Provide<DebugLog>(nullptr);
		m_upDebugLog.reset();
	}
  ```

### 4. Register new files

Add all 4 new files (`DebugLog.h/.cpp`, `DebugConsole.h/.cpp`) to
`DirectX12.vcxproj` and `DirectX12.vcxproj.filters`, matching the existing filter
groupings for `SourceCode/99_Utility/Debug/Log/` (new filter folder, mirror how
`SourceCode/99_Utility/Debug/Imgui/` is already grouped) and
`SourceCode/99_Utility/Debug/Imgui/` (existing filter, just add the 2 new files to
it).

## Relevant Files

Read before starting:
- `C:\Users\green\source\C++\Senzan\Senzan\SourceCode\System\Singleton\Debug\Log\DebugLog.h` (reference only, read-only)
- `SourceCode/99_Utility/Debug/Imgui/DebugHud.h/.cpp` (pattern to mirror for `DebugConsole`)
- `SourceCode/99_Utility/ServiceLocator/ServiceLocator.h`
- `SourceCode/00_Game/00_GameLoop/Main.h/.cpp` (what you're wiring into — read
  `Create()`/`Draw()`/`Release()` fully first)
- `DirectX12.vcxproj.filters` (existing filter structure under `SourceCode\99_Utility\Debug\`)

## Acceptance Criteria

- All listed files compile with 0 errors, 0 warnings in Debug|x64 AND Release|x64
  (this feature is NOT `_DEBUG`-gated, so both configs must build and both must
  actually include it — verify by confirming `DebugConsole::Draw()` is called
  unconditionally in `Main::Draw()`, not inside an `#if _DEBUG` block).
- `DebugLog`/`DebugConsole` follow the ServiceLocator ownership pattern (owned by
  `Main` via `unique_ptr`, registered/unregistered via `ServiceLocator::Provide`) —
  not `Singleton<T>`.
- No other file is modified beyond what's listed in Scope.

## Build

```powershell
powershell -File scripts\build.ps1
powershell -File scripts\build.ps1 -Configuration Release
```

Expected result: 0 errors, 0 warnings for both (aside from the known pre-existing,
unrelated `Vertex.hlsl` shader warning in Release builds — that one is not part of
this task, don't try to fix it, just don't introduce any new ones).

## Test

No automated test suite. Manual verification (launching the game, confirming a
"Console" ImGui window appears — even with zero log entries yet, since nothing calls
`LogInfo`/etc. yet in this task) will be done by Claude/the user after this lands.

## Notes

- Keep this minimal — no command input, no persistence, no unrelated cleanup.
- `std::deque` is used (not `std::vector`) specifically so `pop_front()` for the
  entry cap is O(1) — don't switch to `std::vector`.
