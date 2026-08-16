# タスク: インストール済みVSIXのToolWindowがメニューに表示されない修正

## 目的

インストール済み一覧にはRuntime Model Viewerが存在するが、通常のVisual Studioの
「表示 > その他のウィンドウ」にToolWindowが表示されない問題を解消する。

## 必須調査

- 実際のVSIX内に`Menus.ctmenu`相当のVSCTリソースが埋め込まれているか確認する。
- VSCTの親メニューIDがVisual Studio 18 Communityで有効か確認する。
- `ProvideToolWindow`、Package GUID、ToolWindow GUID、VSCT GUIDの一致を確認する。
- Packageロード失敗が起きても確認できるよう、VSIXログまたはpkgdefを検証する。

## 対象範囲

- VSCT/Package/manifest/プロジェクト設定を修正する。
- ToolWindowを確実に開けるメニューまたはコマンド登録を追加する。
- VSIX Debug/Releaseをビルドし、生成物のリソースとGUIDを検証する。
- 通常Visual Studioでの確認手順をREADMEへ更新する。

## スコープ外

- MSKN/MMAT読込処理。
- DirectX12ゲーム本体。
- エクスプローラーのファイル関連付け。

## 受け入れ基準

- VSIX Debug/Releaseが0エラー・0警告で生成される。
- 生成VSIXに必要なVSCTメニューリソースが存在する。
- 通常Visual StudioでRuntime Model Viewerを開く方法が一つ以上確実に成立する。
- Packageロード失敗時に原因を確認できる手順がある。
