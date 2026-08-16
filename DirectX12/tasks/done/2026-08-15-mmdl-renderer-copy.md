# タスク: MMdl用レンダラーへの移行

## 目的

ゲーム実行時の描画パイプラインを、入力形式名の`PMXRenderer`から実行時形式名の
`MmdlRenderer`へ移行する。MMdlがPMX専用実装に依存しているように見える状態を解消し、
将来PMX/PMD/Xの旧ランタイムをゲーム本体から外せる境界を作る。

## 今回必須の理解

- Parser/RuntimeConverterはオフライン変換時にだけPMX/X/VMDを読む。
- MmdlRendererはゲーム実行時に`.mmdl`系データを描画するGPUパイプラインである。
- クラス名の変更は実行時の責務を明確にするためであり、描画処理の挙動変更ではない。
- `MmdlActor`が描画に必要とするRendererの公開APIと、Renderer内部が旧PMXActorを管理する部分を区別する。

## 対象範囲

- `PMXRenderer.h/.cpp`を内容の基準として`MmdlRenderer.h/.cpp`へ複製する。旧`PMXRenderer`はLegacyに残し、ゲーム本体のビルドから除外する。
- クラス名、前方宣言、friend宣言、メンバー型、include、呼び出し側を`MmdlRenderer`へ更新する。
- `MainScene`、`AnimationTuningScene`、`ModelPreviewPanel`、`MmdlActor`、`MmdlMesh`など、現行MMdl描画経路の参照を更新する。
- `DirectX12.vcxproj`と`DirectX12.vcxproj.filters`に新旧Rendererを登録し、旧`PMXRenderer.cpp`は`ExcludedFromBuild=true`にする。
- 直前のディレクトリ整理で抜けた`90_Legacy/PMX`、`90_Legacy/PMD`、`90_Legacy/PMX/VMD`の既存ビルド登録を復元し、リンクエラーを解消する。
- 旧PMXRendererが内部で保持する`std::vector<std::shared_ptr<PMXActor>>`については、今回の描画挙動を変えない。PMXActor管理の完全除去は次段階で行う。
- `PMXParser`、`PMDParser`、`XParser`、`VMDLoader`、RuntimeConverterの変換用途は維持する。
- `.cpp`/`.h`のUTF-8 BOMを維持し、`using`宣言を追加しない。

## スコープ外

- PMXActor/PMXMesh/PMDActor/XActor/XMeshの削除
- PMX/PMD/VMDのオフライン変換機能の変更
- シェーダー、ルートシグネチャ、リソース状態遷移、DescriptorHeapの変更
- Renderer内部のActor所有設計の変更
- 実機表示の見た目変更
- コミット

## 受け入れ基準

- 現行ゲームコードとMMdlコードに`PMXRenderer`型の参照が残らず、`MmdlRenderer`を参照する。
- 旧`PMXRenderer.h/.cpp`は`90_Legacy/PMX`に残り、プロジェクトへ登録されるが、旧`.cpp`はビルド除外になる。
- RuntimeConverterのPMX/X/VMD参照は維持される。
- Debug|x64とRelease|x64が0エラー・0警告でビルドできる。
- `scripts/check-bom.ps1 -Fix`後、対象`.cpp`/`.h`がBOM付きである。
- 旧X実装は登録されていてもビルド除外のまま維持される。

## 確認方法

- `git diff`で名前変更と参照更新以外の変更がないことを確認する。
- `rg`でゲーム側の`PMXRenderer`参照と、変換側のPMX/X/VMD参照を分けて確認する。
- `powershell -File scripts/build.ps1 -Configuration Debug -Platform x64`
- `powershell -File scripts/build.ps1 -Configuration Release -Platform x64`

## 実施結果

- `PMXRenderer`を`90_Legacy/PMX`に残し、`PMXRenderer.cpp`はゲームプロジェクトでビルド除外にした。
- 実装を複製した`MmdlRenderer`を`30_Asset/RuntimeModel/MMdl`へ配置し、現行ゲーム/MMdl経路を参照更新した。
- 旧X実装は`90_Legacy/X`に登録し、`.cpp`をビルド除外にした。
- Debug/Releaseとも0エラー・0警告でビルド成功。
