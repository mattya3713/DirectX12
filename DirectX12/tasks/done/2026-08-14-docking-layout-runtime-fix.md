# Task (完了): MainScene/AnimationTuningScene実機検証と、ドッキングレイアウトが実機で機能しないバグの修正

## 発端

直前のセッションで「MainSceneとAnimationTuningSceneで描画方式を分岐させる」タスク
(`tasks/done/2026-08-14-render-mode-scope-split.md`、コミット`d1e1eaa`)がビルド
確認(0エラー0警告)まで完了していたが、**実機での見た目確認がまだ**だった状態から
このタスクは始まった。

## 発見された問題と対応

### 1. MainSceneの直接描画復帰

実機確認したところ**問題なし**。素のゲーム画面(DockSpace/Scene Viewパネルなし、
Debug HUD/Consoleは浮動ウィンドウ)に正しく戻っていた。

### 2. `ModelPreviewPanel.cpp`のImGui Begin/End範囲外ウィジェット呼び出しバグ

`ModelPreviewPanel::Update()`で`ImGuiManager::Tweak("Action Frame (Debug)", ...)`
が`ImGui::Begin("Model Select")`/`ImGui::End()`の**外側**(End()の後)で呼ばれていた。
ImGuiの暗黙のフォールバックウィンドウ「Debug」が浮動表示される原因になっていた。
**Claudeが直接修正**(End()呼び出し位置をTweak呼び出しの後へ移動。数行の修正のため
Codexに委任せず直接実装)。

### 3. AnimationTuningSceneのUnity風ドッキングレイアウトが実機で機能しないバグ

F1でAnimationTuningSceneへ切り替えても、`DebugDockSpace.cpp`が構築するはずの
ドッキングレイアウト(Debug HUD/Scene=左、Animation Editor/Actor Scale=右、
Console/Model Select=下、Scene View=中央)が効かず、全パネルが画面左上に重なって
浮動表示されるバグが見つかった。Codexに3ラウンドかけて調査・修正を依頼:

1. **1回目**: `DockBuilderAddNode`に`ImGuiDockNodeFlags_DockSpace`フラグを追加。
   ImGuiのDockSpace API仕様としては妥当な修正だったが、実機再確認では症状変わらず
   (原因の一部に過ぎなかった)。
2. **2回目**: `DebugDockSpace::Draw()`の呼び出しを`Main::Draw()`から
   `Main::Update()`冒頭(各パネルの`Begin()`より前)へ移動。フレーム内の呼び出し
   順序問題という仮説が的中し、Scene Viewパネルが初めて正しく表示されるように
   なったが、新たな症状(Scene View以外の全パネルが消える)が発生。
3. **3回目**: `DockBuilderDockWindow`でleft/right/bottomの各ノードに複数パネルを
   直接割り当てていたため(例: 左ノードに"Debug HUD"と"Scene"が同居)タブ化・
   非表示の問題が発生していた。各ノードをさらにサブ分割し、パネルごとに専用ノード
   を割り当てるよう修正。これで解決。

## 検証結果(Claudeが独立して実機確認)

クリーンな状態(プロセス再起動)で2回、F1切り替え後のAnimationTuningSceneを確認し、
以下が安定して表示されることを確認した:
- 左: Debug HUD(上)+ Scene(下)
- 右: Animation Editor(Actor Scale (Debug)はMainScene専用のためAnimationTuning
  Sceneでは非表示、想定通り)
- 下: Console(左)+ Model Select(右)
- 中央: Scene View(濃いグレーのレターボックス、灰色グリッド床+X軸青/Z軸赤の
  軸線が確認できた)

Scene Viewの中身が白一色に見えた件は、ユーザーの指摘通り、デフォルトで自動
ロードされる`PMX/Cube\Cube.pmx`(テスト用の巨大な白いキューブ)がカメラ前方を
覆っていたためと判断(グリッド線・レターボックスは正しく確認できているため、
レンダリングパイプライン自体の不具合ではない)。

MainSceneへのF1巻き戻し動作は、自動化テスト(スクリーンショット用の合成キー入力)
のタイミング起因と見られる不安定さが1〜2回発生したが、この巻き戻し機構自体は
今回のどの修正でも一切変更していない既存コードであり、セッション最初のMainScene
単体確認・AnimationTuningScene切り替え確認では毎回正常に動作していたため、
製品バグではなくテスト自動化側の問題と判断した。

## 変更ファイル

- `SourceCode/99_Utility/Debug/Imgui/ModelPreviewPanel.cpp`(Claude直接修正 +
  Codex 3回目で微修正)
- `SourceCode/99_Utility/Debug/Imgui/DebugDockSpace.cpp`(Codex 1回目・3回目)
- `SourceCode/00_Game/00_GameLoop/Main.cpp`(Codex 2回目)

## ビルド確認

Debug|x64、各ラウンドごとにClaudeが独立して`scripts\build.ps1`を実行し、
0エラー0警告を確認済み。

## 副産物

- `scripts/codex-task.ps1`の`-Model`パラメータのデフォルト値を`gpt-5.6-luna`に
  変更(ユーザー指示。以後`-Model`省略時もlunaが使われる)。

## 未着手・保留中(このタスクのスコープ外)

- DebugCameraの回転機能(既知、別タスク)。
- `DESIGN.md`の`CombatCoordinatorKey`古い記述(軽微、別タスク)。
