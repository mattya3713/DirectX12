# タスク: ModelViewer VSIXがインストール済みでも表示されない問題の修正

## 目的

Release版VSIXをインストールしても、通常のVisual Studioの「その他のウィンドウ」に
Runtime Model Viewerが表示されない問題を解消する。

## 必須調査

- VSIX manifestのIdentity、Version、InstallationTarget、ProductArchitectureを確認する。
- Package/ToolWindow/VSCTのGUID対応を確認する。
- VSIXへPackage DLL、pkgdef、VSCTメニューが実際に含まれているか確認する。
- Visual Studio 18 CommunityのVSSDK登録形式と一致させる。
- 同一Versionのインストール済み判定を避けるため、修正版のVersionを上げる。

## 対象範囲

- `Tools/ModelViewerExtension`内のVSIX設定、manifest、Package登録を修正する。
- VSIX Debug/Releaseをビルドし、生成物の内容を検証する。
- 通常のVisual Studioに表示されるための必要条件をREADMEへ追記する。

## スコープ外

- MSKN/MMAT読込仕様の変更。
- DirectX12ゲーム本体の変更。
- エクスプローラーのファイル関連付け。

## 受け入れ基準

- VSIX Releaseが0エラー・0警告で生成される。
- 生成VSIXにmanifest、Package DLL、pkgdef、VSCT由来のメニュー情報が含まれる。
- 同一Versionの古いインストール状態を回避できるVersionになっている。
- 通常Visual Studioでの確認手順を明記する。
