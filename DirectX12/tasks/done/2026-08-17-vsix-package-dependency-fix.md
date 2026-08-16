# タスク: VSIX Packageロード失敗の依存DLL調査・修正

## 目的

VSIXは`C:\Users\green\AppData\Local\Microsoft\VisualStudio\18.0_d7e8d001\Extensions`
へ配置されているが、Runtime Model Viewerがメニューに表示されない。Packageのロード失敗原因を
特定し、必要な依存DLLをVSIXへ含める。

## 必須調査

- `ModelViewerExtension.dll`のAssemblyRefとVSIX内エントリを比較する。
- `System.Memory`、`System.Buffers`、`System.Runtime.CompilerServices.Unsafe`などnet472で必要な依存を確認する。
- ActivityLogに依存アセンブリロード失敗が出る場合の確認手順をREADMEへ書く。
- Packageがロードされる最小構成と、WPF UserControl生成時に必要な依存を区別する。

## 対象範囲

- `ModelViewerExtension.Vsix.csproj`とVSIX manifestの依存DLL同梱設定を修正する。
- VSIX Debug/Releaseをビルドし、必要なDLLがZIPへ含まれることを確認する。
- Package GUID、ToolWindow GUID、CTMENU登録を維持する。

## スコープ外

- MSKN/MMAT読込仕様。
- DirectX12本体。
- エクスプローラーのファイル関連付け。

## 受け入れ基準

- VSIX Debug/Releaseが0エラー・0警告で生成される。
- `ModelViewerExtension.dll`の依存アセンブリがVSIXへ含まれる。
- インストール後のPackageロード失敗原因を確認できる。
- READMEに再インストールとActivityLog確認方法がある。

## 追加対応

既に1.0.2がインストール済みで同一バージョンの再インストールを拒否されるため、修正版を1.0.3へ更新した。Debug/Releaseを再ビルドし、Release VSIXへ依存DLL 4点と本体DLLが同梱されることを再確認した。
