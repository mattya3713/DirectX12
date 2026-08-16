# タスク: 独自モデルViewerのVisual Studio ToolWindow統合

## 目的

既存のWPF独立Viewerを、Visual Studio内で表示できるVSIX ToolWindowへ移植する。
独自形式の読み込みロジックは再利用し、Visual Studioの実験用インスタンスから
`View > Other Windows`またはコマンドでViewerを開ける状態にする。

## 今回必須の理解

- VSIXプロジェクトは拡張機能の配置・マニフェスト・デバッグ起動を担当する。
- `Package`はVisual Studioへ拡張を登録する入口である。
- `ToolWindowPane`はVisual Studio側のウィンドウ枠、WPF `UserControl`はその中身である。
- `ProvideToolWindow`とコマンド登録によってToolWindowを表示できる。
- 独立WPFの`Window`をそのまま埋め込むのではなく、表示部分を`UserControl`へ分離する。

## 学習分類

### 今回必須

- VSIX manifestとPackageの役割。
- ToolWindowPaneとUserControlの所有関係。
- 非同期コマンドとUIスレッドの境界。
- 既存RuntimeFormatReaderをVSIXプロジェクトから参照する方法。
- 実験用Visual Studioインスタンスでのデバッグとアンインストール。

### 理解推奨

- Visual Studio SDKのVSSDK互換拡張とVisualStudio.Extensibilityの違い。
- ToolWindowの永続化・ドッキング位置。
- 大きなMSKNを読む際のUIスレッド停止回避。
- VSIXの依存関係と対象Visual Studioバージョン。

### 今回は後回し

- Solution Explorerの右クリックメニューからのモデル起動。
- `.mclp`アニメーション再生。
- Visual Studioのプロジェクト項目変更監視。
- GPU表示、HelixToolkit、ボーン編集。

## 対象範囲

- `Tools/ModelViewerExtension`にVSIXプロジェクトを追加する。
- 既存のMSKN/MMAT ReaderとWPF表示をUserControlへ移植する。
- ToolWindowPane、Package、表示コマンド、VSIX manifestを追加する。
- 実験用Visual Studioインスタンスで起動できる構成にする。
- 独立WPF Viewerは動作確認用として残す。

## スコープ外

- DirectX12ゲーム本体の変更。
- RuntimeFormatのバイナリ仕様変更。
- MCLP再生。
- 既存Viewerの削除。

## 設計前提

- VSSDK互換の従来型VSIX ToolWindowを採用する。
- Visual Studio 18 CommunityのSDKと現在利用可能な.NET Framework/WPF設定に合わせる。
- `RuntimeFormatReader.cs`は共通プロジェクトまたはリンク参照で重複実装を避ける。
- SDKテンプレートやビルドターゲットが不足する場合は、まず不足点を報告し、独自にVSIX形式を捏造しない。

## 受け入れ基準

- VSIXプロジェクトがビルドでき、`.vsix`を生成する。
- Visual Studioの実験用インスタンスが起動する。
- コマンドまたはメニューからModel Viewer ToolWindowを開ける。
- ToolWindow内で`.mskn`を選択し、モデル・マテリアル情報を表示できる。
- 独立WPF Viewerが引き続きビルドできる。
- DirectX12 Debug/Releaseビルドへ影響がない。
- SDK不足などで未達の場合、未達理由と次の必要なインストール項目を明記する。
