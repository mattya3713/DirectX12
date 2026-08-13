# Coding Rules

> Authoritative source: `C++,DirectXコーディング規約.docx` (external, not in this repo).
> `README.md` (repo root) is the maintained in-repo summary of that document, written
> for humans re-checking the rules while implementing. **This file is a shorter digest
> of `README.md` for AI agents (Claude Code / Codex), plus a few points that are
> stricter than what's written down anywhere else** (confirmed directly by the project
> owner, not inferred). When in doubt, `README.md` has the fuller version with
> examples — read it, don't guess.

## Non-negotiable / build-breaking if violated

- **Every source file must be saved as UTF-8 with BOM.** No `/utf-8` compiler flag is
  set, so the compiler relies on the BOM to detect UTF-8; without it, Japanese
  comments get misread as Shift-JIS/CP932 (mojibake, or `error C2001`). Exception:
  `.hlsl`/`.hlsli` files under Shader folders are BOM-less UTF-8 (see
  `.editorconfig`).
  - **This is not just a C++ rule.** `scripts/*.ps1` hit the exact same failure mode:
    a BOM-less `.ps1` containing non-ASCII text (even just an em-dash in a comment, or
    Japanese text in a string) can fail to parse under Windows PowerShell 5.1 with a
    confusing, distant-looking error (e.g. `TerminatorExpectedAtEndOfString` on a
    line that's nowhere near the actual problem) — the interpreter misreads the file
    under the system codepage instead of UTF-8. Any `.ps1` file with non-ASCII
    content needs a BOM for the same reason `.cpp`/`.h` do.
  - Tools that write files (this includes Codex, and Claude's Write/Edit tools) do
    **not** add a BOM by default. After creating or editing a `.cpp`/`.h` file, check
    for a BOM and add one if missing:
    ```powershell
    powershell -File scripts\check-bom.ps1 -Fix              # all tracked .cpp/.h
    powershell -File scripts\check-bom.ps1 -StagedOnly -Fix   # only git-staged files
    ```
    (Equivalent manual one-off if you'd rather not use the script:
    ```powershell
    $bytes = [System.IO.File]::ReadAllBytes($path)
    $hasBom = $bytes.Length -ge 3 -and $bytes[0] -eq 0xEF -and $bytes[1] -eq 0xBB -and $bytes[2] -eq 0xBF
    if (-not $hasBom) {
        $content = [System.IO.File]::ReadAllText($path)
        [System.IO.File]::WriteAllText($path, $content, (New-Object System.Text.UTF8Encoding $true))
    }
    ```
    )
  - This applies after *every* edit to an existing file too, not just new files — an
    Edit-style partial write on a BOM-less file can also cause misdetection.
  - A `pre-commit` git hook (`.git/hooks/pre-commit`, repo root — one level above this
    project folder) runs `check-bom.ps1 -StagedOnly -Fix` automatically as a safety
    net. Don't rely on it instead of checking yourself — it catches what slips
    through, it isn't a substitute for doing it right the first time.
- **No `using` declarations at all** — this includes `using namespace` (already
  banned by `README.md` §9) **and also plain type-alias `using Foo = Bar;`**. This is
  stricter than the written coding standard; it's a hard project-specific rule.
  Prefer the full type name, a `typedef` only where truly unavoidable, or just don't
  alias.

## Summary of `README.md`'s rules (see there for full tables/examples)

- **Naming**: classes/functions/files PascalCase; local variables snake_case; local
  raw pointers `p_`/`cp_` + snake_case; member variables `m_` + PascalCase, with a
  type-specific infix (`m_p` raw ptr, `m_up` unique_ptr, `m_sp` shared_ptr, `m_wp`
  weak_ptr, `m_cp` MyComPtr, `s_` static). Struct members (as opposed to class
  members) are prefix-less PascalCase. Bools: member `m_IsX`, accessor
  `bool IsX() const noexcept` — never `Get`/`Check`/`Should` for a bool getter.
- **Classes**: every class is either designed for inheritance (virtual destructor) or
  marked `final` — never ambiguous. Abstract bases end in `Base`; interfaces are
  prefixed `I`, pure-virtual only, no data members.
- **Copy/move**: owning classes delete copy; move (if present) is always `noexcept`.
- **`explicit`** on every single-argument constructor unless implicit conversion is
  intentional (and commented as to why).
- **`const`/`noexcept`**: read-only member functions and bool getters are `const`;
  simple getters/move ops are `noexcept`. Parameters ≤16 bytes pass by value, larger
  by `const T&`.
- **`enum class`** everywhere, with an explicit underlying type where it matters; bit
  flags use `1 << n` with `DEFINE_ENUM_FLAG_OPERATORS`.
- **Namespaces**: not used at global scope except for internal grouping (e.g. a
  `PlayerState` namespace holding that owner's concrete states). Always
  `DirectX::XMVECTOR` etc. in full, never `using namespace`.
- **File layout**: one class per file, `#pragma once`, include order is own header →
  project headers → third-party (DirectX etc.) → standard library. A `ModelData.h`
  style "plain-data header" (several related PODs, or a binary-format struct group +
  its parser) is the one allowed exception to one-class-per-file.
- **Resource management**: RAII, no raw-owning members, prefer `MyComPtr` for D3D
  resources. Single ownership → `unique_ptr`; shared → `shared_ptr`; non-owning
  reference → raw pointer. `mutable` and `friend` are both "avoid by default, allow
  with a comment when there's a real reason."
- **Member init lists use `{}`** (catches narrowing conversions at compile time), not
  `()` — except for types like `std::vector` where `{}` vs `()` actually changes
  behavior (check intent there).
- **Hot path**: no `new`/`malloc`, no `push_back` that can trigger a reallocation, in
  anything that runs every frame.
- **Comments**: Japanese, ending each line with a half-width period. File header
  doc-comment block (`@author`/`@date`/`@brief`, `@pattern` if a named design pattern
  is used) once per file, right above the first type. Everything else — member
  functions, member variables, enum values — gets a short one-line comment, not a
  block. See `README.md` §1 for the exact templates.
  - **Comment density**: match `README.md`'s templates in *form*, but keep content
    tight — state the non-obvious "why", not a restatement of what the code already
    says. The project owner has pushed back before on AI-authored code having
    padded/redundant comments; when unsure how much is enough, look at a
    similarly-scoped file the owner wrote or already trimmed themselves (e.g. recent
    `Player`/`Boss`-adjacent files) as the calibration reference.
- **Commit messages**: this project uses a `目的:` / `変更点:` / `影響範囲:` template
  (purpose, what changed, blast radius), one line for 目的. Match the style of recent
  `git log` entries.

## Verification (there is no automated test suite)

- "Build" means: MSBuild, Debug|x64 at minimum (Release|x64 too for anything touching
  shared headers, build config, or shader code), with **zero warnings** — this
  project has previously done a full pass to eliminate all compiler warnings and
  intends to stay at zero, not just "no errors."
- "Test" means: no unit/integration test project exists. Verification is manual —
  running the app and exercising the changed behavior. If a change can't reasonably
  be manually verified in the time available, say so explicitly rather than claiming
  it works.
