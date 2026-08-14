# Investigation Request (NOT an implementation task)

This is a pure research/fact-gathering task. **Do not write, edit, or delete any
code.** Your only output should be a single new file:
`tasks/investigation-report.md`, containing a structured factual report. Do not
modify any other file.

## Context

The user wants to eventually extend the existing `AnimationTuningScene` debug scene
into a full "Action Timeline Editor" for tuning Player attack actions (animation
clip selection, per-section playback speed, frame-based attack-collider windows,
combo input windows, combo transition timing, attack power, and eventually collider
shape/position, invincibility windows, movement, SE/effects). Before any design or
implementation happens, a precise, code-grounded understanding of the current system
is needed. You are gathering that understanding — not designing anything, not
recommending anything, just reporting exactly what exists today with exact
file:line references and code snippets.

## What to investigate — read every file listed FULLY before reporting

### A. Animation/time systems

1. `SourceCode/00_Game/00_GameLoop/Time/Time.h` (and `.cpp` if present) — `GameTime`
   class: `GetDeltaTime()`, whether there's a fixed-timestep option.
2. `SourceCode/10_Ggraphic/PMX/PMXActor.h` and `.cpp` — animation playback:
   `PlayAnimation()`, `StopAnimation()`, `StepFrame()`, `SetPlaybackRange(Start,End)`,
   the animation-speed setter, and every member field involved
   (`m_CurrentAnimationTime`/`m_StartFrame`/`m_EndFrame`/`m_AnimationSpeed`/
   `m_MaxFrame`/`m_AnimationStartTime` or similar — use the actual names in the
   file). Report the EXACT formula used each `Update()` to advance the current
   frame/time, and whether "current frame" is readable as a float from outside the
   class (is there a public getter? if not, say so explicitly).
3. `SourceCode/10_Ggraphic/X/XActor.h` and `.cpp` — the `.X`-format equivalent:
   `m_CurrentClipIndex`, `m_CurrentTime`, `PlayAnimation(ClipName)`,
   `StopAnimation()`, the exact `Update()` frame/time-advance logic, and
   `SourceCode/10_Ggraphic/X/XSkeletonData.h`'s `TicksPerSecond`/`MaxTime` fields.
   Report whether X's Tick-based timing has ANY existing relationship/conversion to
   PMX's Frame-based timing (almost certainly none — confirm explicitly).
4. `SourceCode/10_Ggraphic/PMX/AnimationClipTable.h` and `.cpp` — the
   `{Name -> StartFrame,EndFrame,Speed}` table: exact struct fields, `Load`/`Find`/
   `Save` signatures. Also find and show the FULL content of the actual clip data
   file it reads/writes (likely `Data/Config/AnimationClips.txt` — locate the real
   path from the code, don't guess).
5. `SourceCode/99_Utility/Debug/Imgui/AnimationEditor.h` and `.cpp` FULLY — what
   controls exist today (sliders/inputs for Start/End/Speed?), how preview/scrub
   works, exact Save/Load flow with `AnimationClipTable`.
6. `SourceCode/99_Utility/Debug/Imgui/ModelPreviewPanel.h` and `.cpp` FULLY — how it
   selects PMX vs X, how it owns/constructs the actor, how play/pause/step work
   currently, whether "current frame" is exposed in any UI today.
7. `SourceCode/00_Game/00_Scene/Ex_Test/AnimationTuning/AnimationTuningScene.h` and
   `.cpp` FULLY — exactly what this scene does end to end today (its relationship to
   `ModelPreviewPanel`).

### B. Combat state machine & JSON data

8. `SourceCode/00_Game/10_Object/10_MeshObject/00_Character/00_Player/State/
   20_Combat/Combat.h` and `.cpp` (the shared base for AttackCombo_0/1/2/Parry)
   FULLY. Specifically: how it tracks "current time" (member name, units — seconds
   since `Enter()`?), the `ColliderWindow` struct's exact fields, how it loads
   windows from JSON (which loader function, what path pattern), exactly how/when
   each `Update()` activates/deactivates the attack collider, how `UpdateComboInput()`
   works (combo start/end time window semantics — does pressing attack during the
   window queue a transition, or transition immediately?), and how the state decides
   its own Exit/transition-to-next-combo timing.
9. `SourceCode/00_Game/10_Object/10_MeshObject/00_Character/00_Player/State/
   20_Combat/00_AttackCombo_0/AttackCombo_0.h/.cpp`,
   `10_AttackCombo_1/AttackCombo_1.h/.cpp`,
   `20_Combat/20_AttackCombo_2/AttackCombo_2.h/.cpp` — ALL THREE, FULLY. Exact
   damage values, exact JSON file path each loads, anything that differs between
   them beyond the obvious (clip name, JSON path).
10. Find and show the FULL exact content of the actual JSON file(s) these load
    (search under `Data/Json/Player/AttackCombo/` or wherever `Combat.cpp`'s loader
    path actually points — use the real path from the code).
11. Locate the actual `JsonLoad`/`JsonSave` functions (grep for those exact names if
    the guessed path `SourceCode/99_Utility/File/FileManager.h/.cpp` is wrong) — exact
    signatures, how `nlohmann::json` is used, the parse-error handling behavior
    (show the exact try/catch code).
12. `SourceCode/00_Game/10_Object/10_MeshObject/00_Character/Character.h/.cpp` — the
    `m_AttackCollider`/`m_DamageCollider` setup (Capsule Radius/Height/
    PositionOffset in the constructor — confirm exact current values), and how
    `SetAttackColliderActive`/`SetAttackAmount`/`SetAttackColliderOffset` are called
    from the Combat/AttackCombo files (cite exact call sites).
13. `SourceCode/00_Game/10_Object/10_MeshObject/00_Character/00_Player/
    PlayerAccessKeys.h` — the `ComboEconomyKey`/`CombatCoordinatorKey` Passkey
    classes, exactly which methods each key friends.

### C. Project docs & current repo state

14. Read `docs/architecture.md` and `docs/coding-rules.md` (in this repo's
    `DirectX12/docs/`) FULLY. Summarize points relevant to: file layout
    conventions, JSON conventions, State/FSM conventions, and any stated rules
    about where new systems should live.
15. Run `git status --short` and `git diff --stat` from the repo (both
    `C:\Users\green\source\C++\DirectX` and confirm `DirectX12/` subpaths) and
    report what's currently uncommitted (file paths only — no need for full diffs).

## Output format

Write `tasks/investigation-report.md` structured with headers matching sections A/B/C
above (points 1-15), each with exact file:line citations and code snippets for the
key formulas/signatures/JSON schema. Be exhaustive and precise — this report will be
read by someone who needs the EXACT current mechanics to design against without
re-reading the source themselves, so precision and completeness matter far more than
brevity. Do not add design recommendations or opinions — pure fact reporting only.
