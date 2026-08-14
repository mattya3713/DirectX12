# 調査報告

調査日時: 2026-08-14。対象は調査時点の作業ツリー。実装・削除は行っていない。

## A. Animation/time systems

### 1. `GameTime`

対象: `SourceCode/00_Game/00_GameLoop/Time/Time.h:8-30`, `Time.cpp:5-55`。

`GameTime::GetDeltaTime()` は `ServiceLocator::Get<GameTime>()->m_DeltaTime` を返す静的 getter (`Time.cpp:52-55`)。`Update()` は `high_resolution_clock::now()` と `m_PreviousTime` の差を `std::chrono::duration<float>` として秒で計算し、その値を `m_DeltaTime` に保存する (`Time.cpp:20-37`)。

固定 timestep の設定はない。`MaintainFPS()` は `m_DeltaTime < m_TargetFrameTime` のとき `m_DeltaTime = m_TargetFrameTime` とするだけで、sleep はコメントアウトされている (`Time.cpp:39-50`)。目標 FPS は 60、`m_TargetFrameTime = 1.0f / 60.0f` (`Time.cpp:5,12`)。メンバーは `m_PreviousTime`, `m_TargetFrameTime`, `m_DeltaTime` (`Time.h:27-30`)。

### 2. PMX/VMD animation

対象: `SourceCode/10_Ggraphic/PMX/PMXActor.h:42-180`, `PMXActor.cpp:11-29,99-128,189-211,291-329,331-376`。

公開 API は `PlayAnimation()` と `StopAnimation()` (`PMXActor.h:62-64`) だが、両方とも空実装 (`PMXActor.cpp:202-211`)。実際の更新は `Update()` 内の壁時計依存処理で、コード上の式は次のとおり。

```cpp
std::chrono::duration<float> deltaTimeChrono = currentTime - m_AnimationStartTime;
float deltaTime = deltaTimeChrono.count();
float range = m_EndFrame - m_StartFrame;
if (range <= 0.0f) { range = static_cast<float>(m_MaxFrame + 1); }
m_CurrentAnimationTime = m_StartFrame + fmod(deltaTime * m_AnimationSpeed, range);
```

出典: `PMXActor.cpp:99-116`。`Update()` の外側は `if (true)` であり、`m_IsPlayingAnimation` は参照されない。`m_AnimationStartTime` はコンストラクタで現在時刻に初期化される (`PMXActor.cpp:23-29`)。したがって通常更新は `GameTime::GetDeltaTime()` ではなく、生成時からの壁時計経過時間を使う。

`StepFrame()` は範囲を同じ方法で求め、`localTime = m_CurrentAnimationTime - m_StartFrame + 1.0f`、`m_CurrentAnimationTime = m_StartFrame + fmod(localTime, range)` として 1 フレーム進め、`UpdateAnimation()` を呼ぶ (`PMXActor.cpp:119-128`)。

公開 setter/getter は次のとおり (`PMXActor.h:77-86`)。

```cpp
void SetPlaybackRange(float StartFrame, float EndFrame) noexcept
    { m_StartFrame = StartFrame; m_EndFrame = EndFrame; }
void SetAnimationSpeed(float Speed) noexcept { m_AnimationSpeed = Speed; }
float GetStartFrame() const noexcept;
float GetEndFrame() const noexcept;
float GetAnimationSpeed() const noexcept;
float GetCurrentAnimationTime() const noexcept;
uint32_t GetMaxFrame() const noexcept;
```

`GetCurrentAnimationTime()` により、現在フレームは外部から float として読める (`PMXActor.h:85`)。メンバーは `m_IsPlayingAnimation`, `m_CurrentAnimationTime`, `m_AnimationSpeed`, `m_MaxFrame`, `m_StartFrame`, `m_EndFrame`, `m_AnimationStartTime` (`PMXActor.h:173-180`)。VMDロード後、`m_MaxFrame` は全ボーンキーフレームの最大 `FrameNo`、範囲は `0.0f..m_MaxFrame` に設定される (`PMXActor.cpp:307-319`)。

### 3. X animation

対象: `SourceCode/10_Ggraphic/X/XActor.h:38-50,101-102`, `XActor.cpp:98-123`。`PlayAnimation(const std::string& ClipName)` は名前一致するクリップの index を `m_CurrentClipIndex` に設定し、`m_CurrentTime = 0.0f` にする。見つからない場合は何もしない (`XActor.cpp:98-109`)。`StopAnimation()` は `m_CurrentClipIndex = -1` (`XActor.h:41`)。

更新式は次のとおり (`XActor.cpp:111-123`)。

```cpp
const float max_time = static_cast<float>(m_Skeleton.Clips[m_CurrentClipIndex].MaxTime);
m_CurrentTime += GameTime::GetDeltaTime()
               * static_cast<float>(m_Skeleton.TicksPerSecond);
if (max_time > 0.0f) { m_CurrentTime = std::fmod(m_CurrentTime, max_time); }
```

`m_CurrentClipIndex` は `-1` が未再生、`m_CurrentTime` は「ファイル依存の時間軸で秒ではない」private member (`XActor.h:101-102`)。外部から読めるのは clip 一覧と index (`GetClips()`, `GetCurrentClipIndex()`, `XActor.h:48-50`) で、現在 Tick 時刻の getter はない。

`XSkeleton::AnimationClip::MaxTime` は全キー中の最大 `time` (`XSkeletonData.h:60-66`)。`SkeletalData::TicksPerSecond` は `AnimTicksPerSecond`、ファイルに無い場合の既定値 4800 (`XSkeletonData.h:68-75`)。X の実時間から Tick への変換は `deltaTime * TicksPerSecond` として存在するが、PMX/VMD の Frame 軸との変換・共有・参照はコード内に存在しない。PMX は `PMXActor.cpp:102-112` の壁時計秒×速度、X は `XActor.cpp:117-119` の `GameTime` 秒×TicksPerSecond で、相互変換はない。

### 4. `AnimationClipTable`

対象: `SourceCode/10_Ggraphic/PMX/AnimationClipTable.h:13-43`, `.cpp:6-50`。

データ構造は `AnimationClipData { float StartFrame; float EndFrame; float Speed; }`。既定値は `0.0f`, `0.0f`, `30.0f` (`AnimationClipTable.h:13-18`)。保存先定数は `Data\\Config\\AnimationClips.txt` (`AnimationClipTable.h:23-24`)。

```cpp
void Set(const std::string& ClipName, const AnimationClipData& Data);
const AnimationClipData* Find(const std::string& ClipName) const;
bool Load(const std::string& FilePath);
bool Save(const std::string& FilePath) const;
```

出典: `AnimationClipTable.h:30-40`。`Load()` は `ifstream` を開けなければ false、開ければ map を clear し、`名前 開始 終了 速度` の空白区切り4項目を1行ずつ読む (`AnimationClipTable.cpp:17-32`)。`Save()` は親ディレクトリを作成し、unordered_map の各要素を同じ形式で出力する (`AnimationClipTable.cpp:34-50`)。

コードが読む実行時パスは `Data\\Config\\AnimationClips.txt`。調査時点で `Data/Config/AnimationClips.txt` は存在せず、従って示せる実データ内容は「ファイルなし」である。`AnimationEditor` コンストラクタはこの Load の失敗を無視する (`AnimationEditor.cpp:7-12`)。

### 5. `AnimationEditor`

対象: `SourceCode/99_Utility/Debug/Imgui/AnimationEditor.h:23-44`, `.cpp:7-102`。

PMX 用 UI は active 時だけ描画 (`AnimationEditor.cpp:14-16`)。現在時刻を `Frame: %.1f / %u` で表示し、`GetCurrentAnimationTime()` と `GetMaxFrame()` を使う (`AnimationEditor.cpp:24`)。`Start Frame`、`End Frame` は `ImGuiManager::Input`、`Speed` は `0.0f..60.0f` の `Tweak` (`AnimationEditor.cpp:28-38`)。毎 UI フレーム `SetPlaybackRange` と `SetAnimationSpeed` を呼ぶ。

`Clip Name` 入力、`Save Clip`、`Load Clip` がある (`AnimationEditor.cpp:42-60`)。Save は `m_ClipTable.Set(m_ClipName, AnimationClipData{start,end,speed})` 後に既定パスへ `Save()`。Load は map の `Find()` 成功時だけ Actor に範囲・速度を設定する。`Step 1 Frame` は true を返すフラグを立てるだけ (`AnimationEditor.cpp:62-71`)。

X 用 UI (`AnimationEditor.cpp:74-102`) はクリップ名ボタンだけで、押すと `Actor.PlayAnimation(clips[i].Name)`。開始/終了/速度入力、停止、Step、スクラブはない。PMX の current frame は UI 表示されるが、X の current time は表示されない。

### 6. `ModelPreviewPanel`

対象: `SourceCode/99_Utility/Debug/Imgui/ModelPreviewPanel.h:22-60`, `.cpp:26-164`。`ScanModels()` は `Data\\Model\\PMX` を再帰走査して `.pmx`、`Data\\Model\\X` を再帰走査して小文字化比較した `.x` を登録する (`ModelPreviewPanel.cpp:40-74`)。各 `ModelEntry` は表示名、実パス、`IsXFormat` を保持する (`ModelPreviewPanel.h:35-41`)。

コンストラクタは走査後に先頭を `LoadModel(0)`、`AnimationEditor` を生成して active にする (`ModelPreviewPanel.cpp:26-36`)。保持する Actor は PMX が `std::shared_ptr<PMXActor>`、X が `std::unique_ptr<XActor>` で、同時に一方だけ有効 (`ModelPreviewPanel.h:50-55`)。

`LoadModel()` は切替前に `DirectX12::WaitForGPU()`、両 Actor を破棄し、`IsXFormat` に応じて構築する (`ModelPreviewPanel.cpp:77-105`)。X はスケール15倍、先頭 clip を `PlayAnimation()`。PMX は構築後 `StepFrame()` を一度呼ぶ。毎更新、PMX では Editor の Step 要求時だけ `StepFrame()` (`ModelPreviewPanel.cpp:142-147`)、X では毎フレーム `Update()` (`ModelPreviewPanel.cpp:149-153`)。Draw は有効な Actor の Draw を呼ぶ (`ModelPreviewPanel.cpp:156-164`)。

PMX の current frame は `AnimationEditor.cpp:24` の表示がある。ModelPreviewPanel 自体に frame slider/scrub UI はない。

### 7. `AnimationTuningScene`

対象: `SourceCode/00_Game/00_Scene/Ex_Test/AnimationTuning/AnimationTuningScene.h:20-34`, `.cpp:23-91`。`Initialize()` と `LateUpdate()` は空 (`.cpp:23-25,75-77`)。`Create()` は DebugCamera を登録・有効化し、`PMXRenderer` と `ModelPreviewPanel` を生成する (`.cpp:27-39`)。

`Update()` は Debug 時 F1 で MainScene を予約して return (`.cpp:41-51`)、カメラ更新、DirectX12 更新、ModelPreviewPanel 更新を行う (`.cpp:53-73`)。`Draw()` は PMX renderer の pipeline state/root signature/topology を設定後、panel の Draw を呼ぶ (`.cpp:79-91`)。従って現在のシーン固有のアニメーション編集処理はなく、実体は専用カメラ付き `ModelPreviewPanel` である。

## B. Combat state machine & JSON data

### 8. `Combat`

対象: `SourceCode/00_Game/10_Object/10_MeshObject/00_Character/00_Player/State/20_Combat/Combat.h:10-64`, `.cpp:11-104`。`m_CurrentTime` は `Enter()` で0、`Update()` ごとに `GameTime::GetDeltaTime()` を加算する「Enter からの経過秒」(`Combat.cpp:16-31`, `Combat.h:58-62`)。

`ColliderWindow` のフィールドは `float Start`, `float Duration`, `bool IsAct`, `bool IsEnd` (`Combat.h:10-17`)。JSON は派生 state の `GetSettingsFileName()` から得たパスで `FileManager::JsonLoad()` し、トップレベル `ComboStartTime`, `MinComboTransTime`, `ComboEndTime`、配列 `ColliderWindows` の各 `start`, `duration` を `value()` で読む (`Combat.cpp:44-63`)。

`ProcessColliderWindows()` は終了済みを skip。active 中に `m_CurrentTime >= Start + Duration` なら collider OFF、`IsAct=false`, `IsEnd=true`。非active かつ `m_CurrentTime >= Start` なら ON、`IsAct=true` (`Combat.cpp:70-90`)。複数 window が同時に重なる場合も同じ共有 collider に対して順に処理される。

`UpdateComboInput()` は、未受付かつ current time が `ComboStartTime <= t <= ComboEndTime` の間に Attack press があれば `m_IsComboAccepted=true` にする (`Combat.cpp:93-101`)。入力時点では state 遷移せず、戻り値は `m_IsComboAccepted && m_CurrentTime >= m_MinComboTransTime` (`Combat.cpp:103`)。各派生 state が true を受けて次 state に `ChangeState` し、そうでなく `m_CurrentTime >= m_ComboEndTime` なら Idle へ遷移する。`Exit()` は全 window の flags を reset し collider を OFF (`Combat.cpp:33-42`)。

### 9. 3つの AttackCombo

`AttackCombo_0` は `Data\\Json\\Player\\AttackCombo\\AttackCombo_0.json`、clip `player_attack1`、damage `25.0f` (`AttackCombo_0.h:19-23`, `.cpp:5-21`)。true なら `AttackCombo_1`、終了なら Idle (`.cpp:24-37`)。

`AttackCombo_1` は `AttackCombo_1.json`、clip `player_attack2`、damage `30.0f` (`AttackCombo_1.h:19-23`, `.cpp:5-21`)。true なら `AttackCombo_2`、終了なら Idle (`.cpp:24-37`)。

`AttackCombo_2` は `AttackCombo_2.json`、clip `player_attack3`、damage `40.0f` (`AttackCombo_2.h:19-23`, `.cpp:5-21`)。true なら `AttackCombo_0` にループし、終了なら Idle (`.cpp:24-38`)。3つの差分はこの JSON パス、clip 名、damage 値、遷移先である。

### 10. 実際の attack JSON

実ファイルは `Data/Json/Player/AttackCombo/` に3つ存在する。

`AttackCombo_0.json`:

```json
{
    "ComboStartTime": 0.15,
    "MinComboTransTime": 0.45,
    "ComboEndTime": 0.65,
    "ColliderWindows": [
        { "start": 0.36, "duration": 0.10 }
    ]
}
```

`AttackCombo_1.json`:

```json
{
    "ComboStartTime": 0.20,
    "MinComboTransTime": 0.40,
    "ComboEndTime": 0.70,
    "ColliderWindows": [
        { "start": 0.20, "duration": 0.10 },
        { "start": 0.37, "duration": 0.10 }
    ]
}
```

`AttackCombo_2.json`:

```json
{
    "ComboStartTime": 0.60,
    "MinComboTransTime": 1.00,
    "ComboEndTime": 1.50,
    "ColliderWindows": [
        { "start": 0.30, "duration": 0.05 },
        { "start": 0.35, "duration": 0.05 },
        { "start": 0.40, "duration": 0.05 },
        { "start": 0.45, "duration": 0.05 },
        { "start": 0.51, "duration": 0.05 }
    ]
}
```

### 11. JSON loader/saver

対象: `SourceCode/99_Utility/FileManager/FileManager.h:13-19`, `.cpp:8-51`。

```cpp
nlohmann::json JsonLoad(const std::filesystem::path& FilePath);
bool JsonSave(const std::filesystem::path& FilePath,
              const nlohmann::json& JsonData);
```

`JsonLoad()` はファイル不存在または size 0 なら空 JSON、open 失敗なら `_ASSERT_EXPR(false, ...)` 後に空 JSONを返す (`FileManager.cpp:8-23`)。parse error は次の try/catch で処理する (`FileManager.cpp:25-36`)。

```cpp
try
{
    file >> out;
}
catch (const nlohmann::json::parse_error& Error)
{
    const std::wstring w_message =
        MyString::StringToWString(FilePath.string() + ": " + Error.what());
    _ASSERT_EXPR(false, w_message.c_str());
    return nlohmann::json{};
}
```

`JsonSave()` は対象を開けなければ false、開ければ `std::setw(2) << JsonData << std::endl` で保存して true (`FileManager.cpp:41-51`)。呼び出し側の attack 設定では `value(key, default)` が使われる。

### 12. Character collider

対象: `SourceCode/00_Game/10_Object/10_MeshObject/00_Character/Character.h:44-47,80-85`, `Character.cpp:18-36`。constructor は damage collider を radius `0.5f`, height `2.0f`, offset `{0.0f,1.0f,0.0f}`、attack collider を radius `1.0f`, height `2.0f`, offset `{0.0f,1.0f,1.5f}` に設定し、attack を inactive にする。両方を CollisionDetector に登録する (`Character.cpp:18-36`)。

setter は `SetAttackColliderActive(bool)`→`m_AttackCollider.SetActive`、`SetAttackAmount(float)`→`SetAttackAmount`、`SetAttackColliderOffset(const DirectX::XMFLOAT3&)`→`SetPositionOffset` (`Character.h:44-47`)。Combat の call site は `Enter()`/`Exit()` の active OFF (`Combat.cpp:22,41`)、window 開始時 ON・終了時 OFF (`Combat.cpp:80,87`)。3つの AttackCombo の call site は各 `Enter()` の `SetAttackAmount(ATTACK_AMOUNT)` (`AttackCombo_0.cpp:20-21`, `AttackCombo_1.cpp:20-21`, `AttackCombo_2.cpp:20-21`)。Player combat code に `SetAttackColliderOffset` の呼び出しはない。

### 13. Player passkeys

対象: `SourceCode/00_Game/10_Object/10_MeshObject/00_Character/00_Player/PlayerAccessKeys.h:22-42`。`ComboEconomyKey` は private constructor で、friend は `PlayerState::AttackCombo_0`, `_1`, `_2`, `Parry` (`PlayerAccessKeys.h:32-40`)。このキーを受ける Player メソッドは `AddCombo`, `ResetCombo`, `AddUltValue`, `ResetUltValue` (`SourceCode/.../Player/Player.h:54-58`)。

指定された `CombatCoordinatorKey` クラスは `PlayerAccessKeys.h` に存在せず、`rg` によるリポジトリ全体の `class CombatCoordinatorKey` 検索でも該当なし。従って、そのキーの friend 方法は存在しない。

## C. Project docs & current repo state

### 14. ドキュメント

`docs/architecture.md` 全文を確認した。関連する事実は次のとおり。

- `SourceCode/00_Game/10_Object/` に GameObject→MeshObject→Character→Player/Enemy/Boss の階層 (`architecture.md:64-78`)。
- owner ごとに `StateMachine<T>`、owner 専用 state enum、`StateBase<T>`、番号付き `State/` 配下の concrete state を使う (`architecture.md:41-48`)。
- `ServiceLocator` は manager の非所有ポインタ登録 (`architecture.md:35-40`)。
- Passkey は `*AccessKeys.h` に置き、friend されたクラスだけが setter を呼べる形 (`architecture.md:49-53`)。
- JSON/FileManager の配置についての一般的な新システム規定は architecture.md にない。Debug tooling は `SourceCode/99_Utility/Debug/Imgui/` (`architecture.md:130-135`)、AnimationTuningScene は `SourceCode/00_Game/00_Scene/Ex_Test/AnimationTuning/` (`architecture.md:80-93`)。
- 自動テストスイートは存在せず、build と手動検証が前提 (`architecture.md:137-147`)。

`docs/coding-rules.md` 全文を確認した。関連する事実は、source の UTF-8 BOM、`using` 宣言禁止 (`coding-rules.md:11-50`)、PascalCase の型/関数と snake_case のローカル変数、`m_` member 命名、`enum class`、global namespace を避ける方針 (`coding-rules.md:54-75`)、通常は1 class/1 file・`#pragma once` (`coding-rules.md:76-78`)、RAII/所有権規則 (`coding-rules.md:79-85`)、日本語・短い一行コメント (`coding-rules.md:88-98`)。Build は Debug|x64 以上で warning 0、automated test はなく手動検証 (`coding-rules.md:103-111`)。

### 15. Git 状態

`C:\Users\green\source\C++\DirectX\DirectX12` で `git status --short` と `git diff --stat` を実行した結果:

- ` m Data/Library/DirectXTex`（submodule の変更）
- `?? tasks/investigation-request.md`
- diff stat: `DirectX12/Data/Library/DirectXTex | 0`、1 file changed、1行相当の submodule 状態差分。

親リポジトリ `C:\Users\green\source\C++\DirectX` でも結果は対応しており:

- ` m DirectX12/Data/Library/DirectXTex`
- `?? DirectX12/tasks/investigation-request.md`
- diff stat: `DirectX12/Data/Library/DirectXTex | 0`、1 file changed。

この報告ファイル作成前の状態として、上記以外の未コミットパスは確認されなかった。
