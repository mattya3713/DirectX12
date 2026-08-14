# Task (完了): DebugCameraのUnity SceneView風操作追加と、グリッド描画・パネルレイアウトの不具合修正

## 発端

ユーザーから「AnimationEditorでのカメラ、Debugカメラとかにしてほしい。視点の回転
ができないからgridがわからん。UnityのSceneViewと同じ操作がいい」という要望が
あった。

## 実施内容

### 1. DebugCameraへの回転・ズーム追加(Codex実装、Claude修正)

`SourceCode/00_Game/30_Camera/30_Debug/DebugCamera.h/.cpp`に以下を追加:
- 右クリック(RMB)押下中のマウス移動でYaw/Pitch回転(`CameraBase`既存の
  `GetYaw/SetYaw/GetPitch/SetPitch/GetForward/GetRight`を活用).
- Pitchを±89度でクランプ(ジンバルフリップ防止).
- WASD/QEの移動方向を、従来の「LookAtビュー行列の逆行列から算出」方式から
  Yaw/Pitchベースの`GetForward()`/`GetRight()`方式に変更.
- マウスホイールでのズーム(前後移動)を追加。`Main::MsgProc`に
  `WM_MOUSEWHEEL`ケースを新設.

Codexの初回実装ではPitchの符号が逆(マウスを上に動かすと下を向く)だったため、
Claudeが実機確認(Debug HUDのYaw/Pitch値と見た目を照合)して原因を特定し、
初期値とマウス上下ドラッグの反映式の符号を直接修正した(数行の修正のため
Codexに投げ直さず直接実装).

### 2. マウスキャプチャの追加(Claude直接実装)

ユーザーから「今Windowの中でしか動かない。前のプロジェクトはWindow外でも
動いた気がする」との指摘。既存の`Mouse`/`Input`クラスに用意されていたが
未使用だった`CenterMouseCursor()`/`SetShowCursor()`/`SetCenterMouseCursor()`を
使い、右クリック中はカーソルを非表示にして毎フレーム中央へ固定し直すFPS風の
実装に変更(ウィンドウ・画面端でカーソルが止まり回転が続けられなくなる問題を
解消)。回転開始フレームの移動量(固定前の値)は無視するようにした。

### 3. ホイールズームの積算・フレームレート非依存化(Claude直接実装)

`Input::SetWheelDirection()`が上書き方式だったため、1フレーム内に複数の
`WM_MOUSEWHEEL`メッセージが届くと最後の1回分しか反映されない問題があった。
`Main::MsgProc`側で積算方式に変更。また、DebugCamera側の適用量が
`speed * delta_time`(連続入力向けのフレームレート依存スケーリング)を
使っていたため、離散的なホイール入力としては不自然だった点を、
`delta_time`に依存しない固定ステップ(`ZOOM_STEP`)方式に変更した。

### 4. グリッド描画の行列乗算順序バグ修正(Claude直接実装、根本原因)

ユーザーが「回転させてもグリッドがバグったまま」「UIのバグ位置みたいな感じ」と
報告。調査の結果、`Data/Shader/Debug/GridVertex.hlsl`だけが
`mul(vector, matrix)`という順序で行列を乗算しており、このプロジェクトの
他のシェーダー(`PMX/Vertex.hlsl`、`row_major`指定なし)が使う
`mul(matrix, vector)`という規約と逆だったため、実質的に**転置された
ViewProj行列**でグリッドの頂点が変換されていた。これが「回転しても
バグったまま」に見えた根本原因(転置行列による変換は視点変化に対して
正しく追従しないため)。`mul(ViewProj, float4(Input.Position, 1.0f))`に
修正して解決。ユーザーが実機で「グリッドはちゃんと出た」と確認済み。

### 5. ドッキングパネルの比率崩れ・リサイズ時のはみ出し修正(Codex実装)

ユーザーから「パネルの比率がおかしい」「リサイズ時に画面外にちょっとはみ出る」
との報告。`Debug HUD`・`Animation Editor`(2箇所)・`Model Select`が
`ImGuiWindowFlags_AlwaysAutoResize`付きでBeginされており、これがImGuiの
ドッキング機能と相性が悪い(既知の組み合わせ問題)ことが原因と判断。
該当箇所から`ImGuiWindowFlags_AlwaysAutoResize`を除去した
(`MainScene.cpp`の`"Actor Scale (Debug)"`はドッキングされないフローティング
専用ウィンドウのため対象外のまま維持)。

## 変更ファイル

- `SourceCode/00_Game/30_Camera/30_Debug/DebugCamera.h/.cpp`
- `SourceCode/00_Game/00_GameLoop/Main.cpp`
- `Data/Shader/Debug/GridVertex.hlsl`
- `SourceCode/99_Utility/Debug/Imgui/DebugHud.cpp`
- `SourceCode/99_Utility/Debug/Imgui/AnimationEditor.cpp`
- `SourceCode/99_Utility/Debug/Imgui/ModelPreviewPanel.cpp`

## ビルド確認

各ラウンドごとにClaudeが独立して`scripts\build.ps1`を実行し、0エラー0警告を
確認済み。

## 実機確認

- カメラ回転(符号修正後): Debug HUDのYaw/Pitch値と見た目で確認済み(Claude).
- グリッド描画: ユーザーが実機で「グリッドはちゃんと出た」と確認済み.
- マウスキャプチャ・ホイールズーム積算・パネルAutoResize除去: ビルド確認のみ
  (ユーザーがゲームプレイ中だったため、実機での対話的な最終確認は本タスク
  完了時点では未実施。次回セッションで確認する).

## 未着手・保留中

- パネルAutoResize除去後の実機でのリサイズ挙動の最終目視確認.
- Alt+左ドラッグでのオービット操作(Unity SceneView完全互換機能、意図的に
  スコープ外とした。ピボット概念の設計が別途必要).
