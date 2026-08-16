# タスク: 独自ランタイムモデルVisual Studio Viewer調査・初期実装

## 目的

Visual Studio内のVSIXツールウィンドウで、独自形式`.mskn/.mmat`の固定ポーズモデル、テクスチャ、マテリアルを確認できるViewerを作る。既存のVSIX骨格を利用し、まずは変換後データの検証を優先する。

## 今回必須の理解

- VSIXはVisual Studioにツールウィンドウを追加する拡張で、ゲーム実行時Viewerとは別プロセス・別UIになる。
- `.mskn`は頂点、インデックス、ボーン、スキンスロット、サブメッシュを持ち、各サブメッシュが`.mmat`を参照する。
- C++の`RuntimeFormatIO`とC#のバイナリ読込レイアウトを一致させる必要がある。
- 初期版は固定ポーズを表示し、アニメーションは後段に分ける。

## 学習分類

### 今回必須

- バイナリのヘッダー、件数、オフセット、範囲検証。
- WPF/HelixToolkitのViewport、カメラ、MeshGeometry、Material。
- VSIXのPackage、ToolWindow、コマンド登録の関係。
- C++のDirectX座標・行列規約をC#表示座標へ変換する境界。

### 理解推奨

- CPUスキニングの`OffsetMatrix * BoneWorld`の意味。
- WPFのUIスレッドと非同期ファイル読込。
- `.mmat`の旧version互換。

### 今回は後回し

- `.mclp`アニメーション再生。
- GPUスキニング。
- RenderDoc相当のGPUデバッグ。
- Visual Studioデザイナー対応、NuGet依存の完全固定。

## 対象範囲

- 添付VSIX骨格を`Tools/ModelViewerExtension`へ配置する案を調査。
- `.mskn` version 4、`.mmat` version 2/3の安全な読込仕様を作る。
- 固定ポーズのメッシュ表示、UVテクスチャ表示、Diffuse/Specular/Ambient/トゥーン/スフィア情報表示。
- 対応ファイル選択とエラー表示。

## スコープ外

- `.mclp`再生。
- DirectX12ランタイム描画コードの変更。
- 既存ゲームのModelPreviewPanel置換。

## 設計前提

- Visual Studio内のVSIXツールウィンドウとして実装する。
- 初期版は固定ポーズを優先し、ボーン表示とアニメーションは次タスクに分割する。
- HelixToolkit.Wpfを利用する。ただしVSIX SDKとNuGet復元環境が無い場合は、まずC#ローダーと簡易表示までを実装範囲とする。

## 受け入れ基準

- `.mskn`を開いて頂点・インデックス・サブメッシュ件数を表示できる。
- 参照`.mmat`のマテリアル値とテクスチャパスを表示できる。
- 固定ポーズのモデルをViewportへ表示できる。
- 不正な件数、切れたファイル、未知versionで安全にエラー表示する。
- 既存DirectX12 Debug/Releaseビルドへ影響を与えない。
