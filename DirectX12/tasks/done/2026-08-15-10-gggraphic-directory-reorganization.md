# タスク: 10_Ggraphicの責務別ディレクトリ整理

## 目的

`SourceCode/10_Ggraphic`直下に増えたグラフィックス関連ファイルを責務別に整理し、
入力フォーマット、変換ツール、実行時モデル、DirectXデバイス、描画処理の境界を
ディレクトリ構成から読み取れるようにする。

## 今回必須の理解

- `Model`はPMX/X/PMDなどの入力を解析するParser群である。
- `RuntimeFormat`は入力フォーマットを実行時用バイナリへ変換・入出力する層である。
- `MMdl`は実行時に`.mmdl`系データを読み、描画・アニメーションする層である。
- フォルダ移動ではファイルの内容・クラスの責務・公開APIを変更せず、プロジェクト参照とincludeだけを更新する。

## 対象範囲

以下の既存ファイル・ディレクトリを移動する。

```text
10_Ggraphic/DirectX       -> 10_Ggraphic/10_Device/DirectX
10_Ggraphic/Debug         -> 10_Ggraphic/20_Render/Debug
10_Ggraphic/Shader        -> 10_Ggraphic/20_Render/Shader
10_Ggraphic/PMX           -> 10_Ggraphic/90_Legacy/PMX
10_Ggraphic/PMD           -> 10_Ggraphic/90_Legacy/PMD
10_Ggraphic/Model         -> 10_Ggraphic/30_Asset/Parser
10_Ggraphic/RuntimeFormat -> 10_Ggraphic/30_Asset/RuntimeFormat
10_Ggraphic/MMdl          -> 10_Ggraphic/30_Asset/RuntimeModel/MMdl
```

`PMXRenderer`は現状PMX/ＭMdl共通の描画パイプラインとして使われているため、
今回は`PMX`ディレクトリを分割せず、既存のまとまりを維持する。

## 実装要件

- `git mv`相当で既存ファイルを移動する。ファイル内容の機能変更は行わない。
- `DirectX12.vcxproj`と`DirectX12.vcxproj.filters`の全ClInclude/ClCompile参照を更新する。
- すべてのソースの相対include・プロジェクトルート基準include・インクルードディレクトリ設定を更新する。
- `scripts/check-bom.ps1 -Fix`で編集・移動対象の`.cpp`/`.h`をUTF-8 BOMにする。
- `using`宣言を追加しない。
- `DESIGN.md`に残る過去の実装履歴は、今回の移動対象ではないため内容を改変しない。ただし現行構成を説明する節が明らかに誤る場合は最小限修正する。

## スコープ外

- クラス名・API・namespaceの変更
- PMX/PMD/VMDの機能削除やランタイム対応方針の変更
- `PMX`ディレクトリ内部のActor/Mesh/Renderer分割
- 描画パイプライン、リソース状態遷移、シェーダーの動作変更
- ソースコード全体の命名規則の統一
- コミット

## 受け入れ基準

- `10_Ggraphic`直下に、移動対象だった`DirectX`、`Debug`、`Shader`、`PMX`、`PMD`、`Model`、`RuntimeFormat`、`MMdl`が残っていない。
- 移動後の全ファイルが`DirectX12.vcxproj`と`.filters`に正しいパスで登録されている。
- `rg`で移動前のパス参照が残っていない。
- Debug|x64とRelease|x64が0エラー・0警告でビルドできる。
- 既存のゲーム実行時モデル読み込み経路が`30_Asset/RuntimeModel/MMdl`の配置で成立する。
- BOMチェックが成功する。

## 確認方法

- `git diff`で移動と参照更新以外の変更がないことを確認する。
- `powershell -File scripts/build.ps1 -Configuration Debug -Platform x64`
- `powershell -File scripts/build.ps1 -Configuration Release -Platform x64`
- 必要ならDebug版を短時間起動し、プロセスが即時終了しないことを確認する。
