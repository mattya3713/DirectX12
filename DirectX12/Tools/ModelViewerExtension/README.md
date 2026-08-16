# ModelViewerExtension

独自ランタイムモデルの変換結果を確認するためのWPF Viewerです。独立起動用の`MainWindow`と、Visual Studio 18 Community用VSIXの`ModelViewerControl`を同じ読込ロジックで提供します。

## 対応範囲

- `MSKN` version 4のヘッダー、件数、固定長文字列、親順序、インデックス、SkinSlot、サブメッシュ範囲を検証します。
- `MMAT` version 2/3を読み込み、Diffuse/Specular/SpecularPower/Ambient、トゥーン・スフィアフラグ、全テクスチャパスを表示します。
- バインドポーズのCPUスキニング結果をWPF `Viewport3D`へ表示します。Z軸はC++ランタイムの座標系からWPF表示用に反転します。
- `.mclp`、ボーン表示、アニメーション再生、FrontCompositeの特殊合成は次タスクです。

## 実行

```powershell
dotnet run --project Tools\ModelViewerExtension\ModelViewerExtension.csproj
```

## Visual Studio 18 Communityでの確認

1. `ModelViewerExtension.Vsix.csproj`をDebugまたはReleaseでビルドし、生成された`bin\Debug\net472\ModelViewerExtension.vsix`または`bin\Release\net472\ModelViewerExtension.vsix`をVSIX Installerでインストールします。修正版のVSIXバージョンは`1.0.3`です。
2. Visual Studio 18 Communityを再起動し、`表示`→`その他のウィンドウ`→`Runtime Model Viewer`を選択します。
3. ツールウィンドウの`MSKNを開く`から`.mskn`を選択し、モデルとマテリアル情報が表示されることを確認します。
4. メニューが表示されない場合は、Visual Studioを終了し、拡張機能をアンインストールしてから、インストール済み一覧からRuntime Model Viewerが消えたことを確認します。その後、VSIXのバージョンが`1.0.3`であることを確認して再インストールします。同一バージョンの旧VSIXは更新対象として扱われない場合があります。

## VSIX生成物とPackageロード失敗の確認

生成された`.vsix`はZIPとして開き、`extension.vsixmanifest`、`ModelViewerExtension.dll`、`ModelViewerExtension.pkgdef`に加えて、`System.Memory.dll`、`System.Buffers.dll`、`System.Numerics.Vectors.dll`、`System.Runtime.CompilerServices.Unsafe.dll`が含まれることを確認します。これらは出力フォルダーへコピーされるだけではVSIXへ自動収録されないため、`ModelViewerExtension.Vsix.csproj`で`ForceIncludeInVSIX="true"`を指定しています。`ModelViewerExtension.dll`のAssemblyRefとVSIX内エントリは次のコマンドで比較できます。

```powershell
$assembly = [System.Reflection.Assembly]::LoadFile((Resolve-Path bin\Release\net472\ModelViewerExtension.dll))
$assembly.GetReferencedAssemblies() | Sort-Object Name | Select-Object Name, Version

Add-Type -AssemblyName System.IO.Compression.FileSystem
$zip = [System.IO.Compression.ZipFile]::OpenRead((Resolve-Path bin\Release\net472\ModelViewerExtension.vsix))
$zip.Entries | Select-Object -ExpandProperty FullName
$zip.Dispose()

$required = 'ModelViewerExtension.dll', 'System.Memory.dll', 'System.Buffers.dll', 'System.Numerics.Vectors.dll', 'System.Runtime.CompilerServices.Unsafe.dll'
$entries = [System.IO.Compression.ZipFile]::OpenRead((Resolve-Path bin\Release\net472\ModelViewerExtension.vsix)).Entries.FullName
$required | Where-Object { $_ -notin $entries }
```

`ModelViewerExtension.dll`のAssemblyRefに直接現れる追加アセンブリは`System.Memory`です。`System.Buffers`、`System.Numerics.Vectors`、`System.Runtime.CompilerServices.Unsafe`は`System.Memory`のnet472実行時依存なので、ModelViewerExtension.dllの直接AssemblyRefに現れなくてもWPFの`ModelViewerControl`生成とMSKN/MMAT読込には必要です。Package登録だけを確認する最小構成では依存DLLのコード実行まで到達しない場合がありますが、実際のツールウィンドウ表示と読込を成立させるVSIXには4つすべてを同梱します。

pkgdefにはPackage GUID、ToolWindow GUID、`ModelViewer.ctmenu`のメニュー登録が必要です。`ModelViewer.vsct`の出力リソース名はファイル名に従うため、Packageの`ProvideMenuResource`も`ModelViewer.ctmenu`を指定します。

DLLのVSCTリソースはVisual Studio Developer PowerShellで次のように確認できます。`dumpbin /resources`ではマネージドDLLのリソース名が表示されないため、`/all`を使用します。

```powershell
dumpbin /all bin\Release\net472\ModelViewerExtension.dll | Select-String ModelViewer.ctmenu
Get-Content bin\Release\net472\ModelViewerExtension.pkgdef
```

`ModelViewer.vsct`はビルド時に`ModelViewer.cto`へコンパイルされ、`ModelViewerExtension.dll`の`ModelViewer.ctmenu`リソースとして埋め込まれます。生成pkgdefの`[$RootKey$\\Menus]`にPackage GUIDと`, ModelViewer.ctmenu, 1`があり、VSIX ZIPに`ModelViewerExtension.dll`とpkgdefがあれば、manifestからPackageを登録する経路は成立しています。VSCTの親Group `IDG_VS_WNDO_OTRWNDWS1` はVSSDKのOther Windowsグループです。

Packageのロード失敗を調べる場合は、Visual Studio Developer PowerShellで`devenv.exe /log`を実行して起動し、Visual Studioを終了してから`%APPDATA%\Microsoft\VisualStudio\18.0_*/ActivityLog.xml`を開きます。`ModelViewerExtension`、Package GUID、`ModelViewer.ctmenu`、`System.Memory`、`System.Buffers`、`System.Runtime.CompilerServices.Unsafe`を検索し、依存アセンブリのロード失敗やpkgdef登録失敗の有無を確認します。Packageロード失敗が無く、pkgdefのMenus登録も一致している場合は、古いインストールキャッシュが原因と判断して、1.0.3をアンインストール後に再インストールします。

VSIXのmanifestはVisual Studio 18 Community（`Microsoft.VisualStudio.Community`、`[18.0,19.0)`、`amd64`）を対象にしています。Package GUIDは`ModelViewerPackage`とVSCTの`guidModelViewerPackage`で共通化し、ToolWindow GUIDは`ModelViewerToolWindow`と生成pkgdefで一致させ、コマンドセットGUIDは`ShowModelViewerCommand`とVSCTの`guidModelViewerCommandSet`で一致させています。VSCTの親ID `IDG_VS_WNDO_OTRWNDWS1` はVisual StudioのOther Windowsグループです。VSCTのメニュー情報はビルド時に`ModelViewerExtension.dll`の`ModelViewer.ctmenu`リソースへ埋め込まれ、生成pkgdefの同名リソース登録を通じてVSIXへ含まれます。
