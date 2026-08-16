# タスク: MmdlRendererのActor所有を除去

## 目的

`MmdlRenderer`から旧`PMXActor`の所有・更新・描画責務を取り除き、ゲームオブジェクト側が`MmdlActor`を保持して更新・描画する構造へ移行する。

## 今回必須の理解

- RendererはGPUパイプライン、ルートシグネチャ、共通テクスチャを所有・提供する。
- Actorは自分の頂点、マテリアル、ボーン行列、ランタイム状態を所有する。
- `MeshObject`は`GameObject`を継承し、`MmdlActor`を保持してゲームループから`Update`/`Draw`を呼ぶ。
- `MmdlRenderer`から`MmdlActor`を参照する必要はなく、`MmdlActor`がRendererを非所有参照で利用する。
- 所有権変更とGPUリソース状態遷移は別問題であり、今回GPU状態遷移の仕様は変更しない。

## 対象範囲

- `MmdlRenderer`の`PMXActor`一覧メンバーを削除する。
- `MmdlRenderer::AddActor`、Actor一覧を走査する`Update`/`Draw`処理を削除またはRendererの責務に合う形へ整理する。
- `MmdlActor`に必要な更新・描画処理を移し、既存の描画結果を維持する。
- `MeshObject`または既存の適切なGameObject層から`MmdlActor`の`Update`/`Draw`を呼ぶ。
- MainScene、AnimationTuningScene、ModelPreviewPanelの呼び出し順を新しい所有関係に合わせる。
- `MmdlRenderer`の公開APIは、パイプライン、ルートシグネチャ、共通テクスチャ提供に限定する。

## スコープ外

- PMX/X/PMD/VMDのパーサーやオフライン変換処理の変更
- シェーダー、ルートシグネチャのレイアウト、DescriptorHeap、リソース状態遷移の変更
- MMdlのファイルフォーマット変更
- 実機の見た目を変えるための新機能
- 旧`PMXRenderer`ファイルの削除
- コミット

## 実装要件

- `MmdlRenderer`は`PMXActor`またはActor一覧を参照・所有しない。
- `MmdlActor`が必要とするRendererは参照またはポインタで非所有利用する。
- `MeshObject`が所有するActorの寿命がRendererより長くならないよう、既存のScene所有関係を確認する。
- `using`宣言を追加しない。
- `.cpp`/`.h`はUTF-8 BOM、`.hlsl`/`.hlsli`はBOMなしを維持する。
- 変更前後で描画順序とUpdate順序を可能な限り維持する。

## 受け入れ基準

- `MmdlRenderer`に`PMXActor`一覧、`AddActor`、Actor更新・描画の所有処理が残っていない。
- MainSceneとAnimationTuningSceneが、Sceneまたは`MeshObject`経由でActorを更新・描画できる。
- Debug/Releaseのx64ビルドが0エラー・0警告。
- `rg`でMMdl実行経路から`PMXActor`所有依存が消えている。
- 既存のPMX/X/VMD変換・Legacyコードは引き続きビルドまたは変換用途を満たす。

## 確認方法

- `tasks/.codex-last-report.json`を確認する。
- `git diff`で所有権変更と呼び出し順を確認する。
- `powershell -File scripts/build.ps1 -Configuration Debug -Platform x64`
- `powershell -File scripts/build.ps1 -Configuration Release -Platform x64`

## 実施結果

- `MmdlRenderer`からActor一覧、`AddActor`、Actorの更新・描画処理を削除した。
- `MainScene`と`AnimationTuningScene`は`MmdlRenderer::BeforDraw()`でGPU状態を設定し、Actorの描画は既存の`MeshObject`/`ModelPreviewPanel`経由で行う。
- 旧`PMXActor`を同一プロジェクトでコンパイルする互換性のため、`friend PMXActor`だけは残した。所有関係はない。
- Debug/Releaseとも0エラー・0警告でビルド成功。
- BOM確認済み。手動起動確認は未実施。
