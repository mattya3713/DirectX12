# Task: PMX/Xからランタイム形式への変換パイプライン

## Goal

## Current execution phase: standalone converter executable with automatic project scan

### Follow-up requirement: no-argument conversion and mmdl hierarchy

When `RuntimeConverter.exe` is launched without arguments, it must locate the project `Data\\Model` directory relative to the executable/project layout and convert the source assets automatically.

The custom runtime assets must be placed under the following hierarchy:

```text
Data\\Model\\mmdl\\
  mskin\\  skin/mesh data (`.mskn`)
  mstc\\   animation data (`.mclp`)
  mmat\\   material data (`.mmat`)
```

`Data\\Model\\X` and `Data\\Model\\PMX` remain source asset directories. The converter must not introduce a `Converted` directory. Existing explicit CLI forms must continue to work, while no arguments performs the automatic scan/conversion. Invalid explicit arguments remain errors. The implementation must create missing destination directories, report each converted asset and warnings/errors, and return nonzero if an automatic conversion fails.

Scope out: GUI, drag-and-drop, interactive source selection, changes to game startup/runtime asset loading, and new binary format semantics.

### Follow-up requirement: tolerate known source asset limitations

- PMX SDEF vertices must be exported by approximation as BDEF2: preserve the two SDEF bone indices and weight, use the complementary weight for the second bone, and deliberately ignore SDEF-specific `C`, `R0`, and `R1` parameters. Emit a warning that the deformation is approximate; do not reject the whole PMX because of SDEF.
- A PMX without a VMD must still produce `.mskn` and `.mmat`. It produces no `.mclp` and is not an automatic conversion failure.
- When X contains an out-of-range parent bone index, treat that bone as a root (`ParentIndex = -1`), emit a warning, and continue conversion. Other genuinely unrecoverable parse errors remain failures.
- Automatic conversion should return nonzero only when no usable conversion can be completed or an unrecoverable error occurs; skipped animation and recoverable warnings must not make a valid model conversion fail.

### Follow-up requirement: large mesh index support

The current `.mskn` index array uses `uint16_t` and prevents conversion of valid PMX files with more than 65,535 vertices. Upgrade the `.mskn` binary format to Version 3 and store `MsknData::Indices` as `std::uint32_t` values. Update the header/version validation, read/write byte sizes, converter validation, and all affected runtime-format structures consistently. `SkinVertex::BoneIndices` remains a skin-slot index and may remain `std::uint16_t` for this task; only mesh triangle indices are widened. Existing Version 2 files must be rejected explicitly rather than silently reinterpreted.

RuntimeFormat v2と`RuntimeConverter`は実装済み。今回のCodex実行では、既存ゲームEXEから独立して変換を実行できる専用コンソールEXEを追加する。

- `RuntimeFormat.h`に`.mskn` Version 2のSkinSlot配列と`SkinSlotCount`を追加する。SkinSlotは`BoneIndex`と`DirectX::XMFLOAT4X4 OffsetMatrix`を持つ。
- `RuntimeFormat.h`に`.mmat` Version 2のDiffuse、Specular、SpecularPower、Ambient、Base/Normal/Toon/Sphereの固定長パス、Sphere使用フラグを追加する。
- `RuntimeFormatIO.cpp`のVersion定数とヘッダー検証をv2へ更新し、既存の書き込み・読み込み順序を壊さず、SkinSlotと拡張マテリアルの全フィールドを往復できるようにする。
- `MsknHeader`の既存フィールド順は変更せず、SkinSlotCountを末尾に追加する。`MsknData`のストリーム順序はHeader、Vertices、Indices、Bones、SkinSlots、Submeshesとする。
- `MmatData`は既存API利用箇所を壊さないよう拡張し、読み書き時に固定長文字列の終端と上限を検証する。
- Version 1を黙って読み込まない。Version不一致は明示的に失敗させる。
- 専用の`RuntimeConverter.vcxproj`を追加し、既存のDirectX12ゲームプロジェクトとは別のコンソールアプリとしてビルドする。ソリューションファイルがない場合は、プロジェクト単体でビルド可能にする。
- CLI形式は次の2種類に固定する。
  - `RuntimeConverter.exe x <input.x> <output-directory>`
  - `RuntimeConverter.exe pmx <input.pmx> <input.vmd> <output-directory>`
- 引数不足、未知の形式、入力ファイル不在、変換失敗はUsage/Errorを標準エラーへ表示し、0以外の終了コードを返す。成功は生成ファイル一覧を標準出力へ表示し、終了コード0を返す。
- `RuntimeConverter::ConvertX`/`ConvertPmx`を呼び出すだけの薄いCLIにし、変換ロジックを複製しない。既存ゲームEXEのWinMain/Mainや実行時アセットロードは変更しない。
- 明示的なPMX CLIではVMD引数を受け付ける。無引数の自動変換ではVMDなしPMXもモデルだけ変換する。
- Debug/Releaseのx64プロジェクトを独立ビルドし、`RuntimeConverter.exe --help`相当の引数検証を行う。実アセット変換は可能な範囲で行い、できなければ理由をCompletion Reportへ記録する。

既存のPMX/Xモデルを、エンジン専用ランタイム形式へ変換するロジックを実装する。

対象は以下の3種類とする。

- `.mskn`: スキニングメッシュとスケルトン
- `.mclp`: アニメーションクリップ
- `.mmat`: マテリアルとテクスチャ参照

変換後のランタイム側は、PMX/Xの形式差や時間軸の差を意識せず、共通の
`RuntimeFormat`データとして扱えることを目的とする。

## User-approved design decisions

### 1. XのSkinSlotをランタイム形式へ保存する

XParserの現在の`Model::Vertex::BoneIndices`は、`XSkeleton::Bones`ではなく
`XSkeleton::SkinSlots`を参照している。SkinSlotにはメッシュごとに異なり得る
`OffsetMatrix`が含まれる。

Xの各メッシュ・ボーン組に固有の`OffsetMatrix`を失わないため、`.mskn`へ
`SkinSlot`相当のデータを追加する。頂点の`BoneIndices`は`SkinBone`を直接指さず、
`.mskn`内のSkinSlot配列を指す。SkinSlotが参照する実ボーンは`BoneIndex`で指定し、
スキニング時にはSkinSlotの`OffsetMatrix`とSkinBoneの現在のワールド変換を合成する。

`.mskn`へ保存するSkinSlotは、少なくとも以下を持つ。

- `BoneIndex`
- メッシュ・ボーン組に固有の`OffsetMatrix`

このため、現在の`Model::ModelData`だけでは失われている以下の情報を、変換用の
中間データとして保持できるようにする。

- Xのメッシュ単位の頂点・インデックス範囲
- メッシュごとのFrame階層からの変換行列
- メッシュごとのSkinSlotとOffsetMatrix
- メッシュごとのマテリアル範囲

既存のゲーム側`Model::ModelData`の意味を壊さず、変換専用のデータ構造または
`XParser`の変換用APIを追加して対応する。

### 2. PMXのアニメーション入力

PMX本体にはアニメーションが含まれないため、PMXとVMDを入力にして`.mclp`を生成
する。VMDのボーン名をPMXのボーン名へ解決し、対応しないVMDボーンは警告として
扱い、変換全体は継続する。

VMDのフレーム番号は30fps基準の秒へ変換する。

```text
time_seconds = FrameNo / 30.0f
```

### 3. `.mclp`の補間

今回の`.mclp`では、既存の`RuntimeFormat::Keyframe`レイアウトを維持する。
VMDの補間曲線は今回保存せず、ランタイムでは既存設計どおり線形補間を使用する。
VMD補間曲線を保存・再現するためのフォーマット拡張は後続タスクとする。

### 4. `.mmat`の拡張

`.mmat`は現在のベースカラー/法線テクスチャパスだけの仕様から拡張し、少なくとも
以下を保持できるようにする。

- Diffuse color
- Specular color
- Specular power
- Ambient color
- Base color texture path
- Normal map texture path(入力に存在する場合)
- Toon texture path
- Sphere texture path
- Sphere map使用フラグ

PMXのSphere/Toon情報と、XのMaterial情報を変換する。入力形式に存在しない項目は
ランタイム形式の既定値を使用する。テクスチャパスはプロジェクトのData配下を基準
とする既存の相対パス規則へ正規化する。

### 5. マテリアルの重複排除

マテリアルは内容を正規化したキーから決定的な64bit FNV-1aハッシュを計算し、同じ
内容の`.mmat`を共有する。

ハッシュ対象は、拡張後の全マテリアル値と正規化済みテクスチャパスとする。
ハッシュ衝突を正しい一致とみなしてはならない。同じハッシュの既存候補がある場合は
全フィールドを比較し、一致した場合だけ既存ファイルを再利用する。
衝突して内容が異なる場合は、決定的な衝突回避用サフィックスを付けて別ファイルを
生成する。

### 6. SDEFはBDEF2へ近似変換する

SDEF固有の`C`、`R0`、`R1`はランタイム形式へ保存せず、2ボーンのインデックスと
ウェイトだけをBDEF2相当として出力する。SDEF固有の関節補正は失われるため、変換時に
近似であることを警告する。完全なSDEF再現は今回のスコープ外とする。

マテリアルファイル名のハッシュ表現、出力ディレクトリ、衝突時の命名規則は既存の
アセットパス規則を調査してプロジェクト内で一貫させること。既存規則がない場合は
`<hash>.mmat`を基本形とし、仕様書または実装コメントに理由を残す。

## Scope

- `RuntimeFormat::MmatHeader`/`MmatData`と`RuntimeFormatIO::WriteMmat`/
  `ReadMmat`の拡張
- PMXの`Model::ModelData`から`.mskn`と`.mmat`を生成する変換処理
- PMXとVMDから`.mclp`を生成する変換処理
- Xの変換用中間データ/API拡張
- Xから共通モデル空間へ正規化した`.mskn`と`.mmat`、および各AnimationSetの
  `.mclp`を生成する変換処理
- 16bit index上限、固定長文字列上限、空データ、未解決ボーン、壊れた入力に対する
  明示的なエラーまたは警告処理
- 変換結果の最小限の検証コードまたはスタンドアロンテスト
- 変換処理の構造と座標系の説明を`DESIGN.md`へ追記

## Out of scope

- `.mskn`/`.mclp`をGPUで描画・再生する新Actor
- オンデマンド変換と更新日時キャッシュ
- `.mstc`変換
- Action Timeline Editor本体
- VMD補間曲線の保存・再現
- SDEF、モーフ、IK、物理演算、表情アニメーション
- Skin用法線マップの完全対応
- 既存`PMXActor`/`XActor`の置き換え

## Required learning checkpoints

実装前に、ユーザーが以下を説明できる状態にする。

1. `SkinBone`のローカルバインド姿勢と、頂点のボーンウェイトが何を表すか。
2. Xの`SkinSlot::OffsetMatrix`をメッシュ・ボーン組ごとに保存し、スキニング時に
   `SkinBone`の階層変換と合成する理由。
3. PMXのワールド空間ボーン位置から親相対ローカル位置を計算する方法。
4. VMDのフレーム番号を秒へ変換する理由と、補間曲線を捨てた場合の影響。
5. マテリアル内容の正規化、決定的ハッシュ、ハッシュ衝突検証の関係。

実装へ進む前に、これらを短い説明と既存コードの対応確認で扱うこと。

## Implementation requirements

### Common conversion representation

PMX/Xのローダー出力を直接書き出し処理へ散在させない。変換処理の内部で共通の
中間表現を作り、そこから`RuntimeFormat::MsknData`、`MclpData`、`MmatData`へ
変換する。

中間表現では少なくとも以下を明示する。

- 共通モデル空間の頂点・インデックス
- ローカル空間のボーン階層
- 頂点から`SkinSlot`を参照する4影響までのウェイト
- マテリアル単位のインデックス範囲
- 秒単位のアニメーション時間
- 変換元と変換先の名前解決結果

### PMX conversion

- `Model::Bone::Position`はワールド空間として扱う。
- ルート以外のローカル位置は`childWorld - parentWorld`で計算する。
- PMXのバインド回転は単位クォータニオン、スケールは`(1,1,1)`とする。
- PMXの頂点indexは`uint16_t`上限を検証してから変換する。
- PMXのBDEFウェイトを最大4影響へ正規化する。
- SDEFはBDEF2相当へ近似変換し、`C`、`R0`、`R1`は無視する。
- PMXのマテリアルごとの連続インデックス範囲を`SkinSubmesh`へ変換する。

### VMD conversion

- VMDボーン名をPMXボーン名へ解決する。
- 1つのVMDを1つの`.mclp`へ変換する。
- クリップ名は入力VMDのファイル名から拡張子を除いたものを既定値とする。
- 同一ボーンのキーは時刻順に並べる。
- 回転は正規化したクォータニオンを保存する。
- 現行`.mclp`のKeyframeへ位置・回転・スケールを格納し、存在しないキー種別は
  既定値または該当トラック無しで表現する。

### X conversion

- `FrameTransformMatrix`は親相対ローカル行列として扱う。
- 変換用中間データで各メッシュのローカル頂点、メッシュ変換、SkinSlotを保持する。
- スキニングメッシュは共通モデル空間へ頂点を変換し、メッシュ・ボーン組ごとの
  `OffsetMatrix`をSkinSlotとして保存する。頂点のBoneIndicesはSkinSlot indexへ
  再マッピングする。
- 剛体メッシュは共通モデル空間へ変換した頂点を持つ。ただしSkin用`.mskn`へ混在
  させるか、将来の`.mstc`へ送るかは、既存Xモデルの実データを確認して判断し、
  判断をCompletion Reportに記録する。
- 各AnimationSetを1つの`.mclp`へ変換する。
- Xの時刻軸は`TicksPerSecond`で秒へ変換する。
- Xの変換でSkinSlotと頂点の対応を再構成できないケースは、頂点を黙って変形せず、
  入力名と理由を含むエラーとして報告する。

### Validation

- 書き出した各ファイルを`RuntimeFormatIO`で読み戻し、要素数・名前・座標・ウェイト・
  マテリアル参照・クリップ時間が一致することを確認する。
- PMXの小さな検証データで、親子ボーンのローカル位置が期待値になることを確認する。
- Xの少なくとも1メッシュについて、頂点のSkinSlot index、SkinSlotのBoneIndex、
  OffsetMatrixの対応が保持されていることを確認する。
- 同一内容のマテリアルを複数回変換しても`.mmat`が1つだけになることを確認する。
- ハッシュが同じで内容が異なるテストでは、別ファイルになることを確認する。
- 変換対象の未対応要素は、成功扱いで黙って破棄しない。

## Acceptance criteria

- PMX + VMDから`.mskn`、必要な`.mmat`、`.mclp`を生成できる。
- Xから`.mskn`、必要な`.mmat`、AnimationSetごとの`.mclp`を生成できる。
- PMX/Xのマテリアル値とテクスチャ参照が、拡張後`.mmat`へ保持される。
- 同一内容のマテリアルがハッシュで共有され、衝突時に誤共有されない。
- PMXのボーンローカル化とXのSkinSlot保存・参照関係がテストで検証される。
- 変換後の各バイナリを読み戻せる。
- 既存のゲーム側PMX/X読み込み・描画は壊さない。
- Debug|x64ビルドが0エラー0警告で成功する。
- 変更した`.cpp`/`.h`はUTF-8 BOMを持ち、`using`宣言を追加しない。

## Design risks to report before implementation

## 実装開始時点での追加確定事項

上記の設計リスクは、今回の実装を停止させる未決定事項ではない。以下を実装上の確定事項とする。

- `.mskn` と `.mmat` は既存のVersion 1を上書き互換にせず、Version 2へ更新する。ヘッダーのVersion値で拒否できるようにし、Version 1の自動変換や黙った読み替えは今回行わない。
- `.mskn` のVersion 2には `SkinSlotCount` と、各SkinSlotの `BoneIndex` および `OffsetMatrix` を追加する。`SkinVertex::BoneIndices` は引き続きSkinSlot配列のインデックスを指す。PMXは1ボーン1スロット（OffsetMatrixは単位行列）として出力する。
- X変換では、XParser内部のRawMesh/SkinWeight情報を変換専用の中間表現へコピーし、メッシュ単位の頂点範囲、FrameTransformMatrix、SkinSlot、マテリアル範囲を失わない。変換不能な対応関係は推測で補正せずエラーにする。
- PMXのSDEF判定は既存のゲーム用 `Model::ModelData` の意味を変更せず、既存の2ボーン情報をBDEF2近似として利用する。SDEF固有データは無視し、警告を記録する。
- PMXのBDEF入力は変換専用中間表現から処理し、既存 `Model::ModelData` がSDEF情報を失うことによる誤判定を避ける。既存のゲーム実行時ロード経路の挙動は変更しない。
- `.mmat` のハッシュは共有候補を検索するためだけに使う。ハッシュ一致後は正規化済み全フィールドを比較し、一致しなければ別ファイル名を発行する。ハッシュ衝突を理由に異なるマテリアルを共有しない。
- VMDの補間曲線は今回保存しない。既存 `Keyframe` へフレーム番号/30秒、位置、正規化済み回転、既定スケールを格納し、未登場ボーンのトラックは生成しない。
- 変換処理は既存ゲーム起動時に自動実行せず、明示的に呼び出せるConverter API/小さな検証入口として追加する。既存PMXActor/XActorのGPUスキニング実装は今回変更しない。
- 変換処理とRuntimeFormat IOの最小検証を、実在アセットに依存しない小さなテストデータでも実行できるようにする。実在PMX/X/VMDでの確認が環境依存になる場合は、Completion Reportに明記する。

Codexは以下に未解決点が残る場合、独断で簡略化せずCompletion Reportで停止する。

- Xの複数メッシュのSkinSlotと頂点の対応を再構成できない場合
- 現行`.mmat`拡張が既存ファイルとの互換性を壊す場合
- VMDキーを現行`.mclp`のKeyframe表現へ正しく対応できない場合
- Xの剛体メッシュを`.mskn`へ混在させる設計が既存データと衝突する場合
- ハッシュ命名が既存アセットパス規則と矛盾する場合
# Task: mmdlランタイム描画経路への切り替え

## Goal

ゲーム実行時のPMX/X直接読み込みを廃止し、事前変換済みの`.mskn`、`.mmat`、
`.mclp`だけを読み込んで、既存のPlayer/Bossおよび現在のモデル描画を行う。
PMX/XはRuntimeConverterによる事前変換時だけ使用し、ゲーム起動時のフォールバック
読み込みには使わない。

## Scope

- `.mskn`の読み込みからGPU描画用メッシュ・スキニングデータを構築するランタイム側API
- `.mmat`の読み込みから既存マテリアル・テクスチャ設定を構築する処理
- `.mclp`の読み込みと、既存`Update()`/`PlayNamedClip()`/`SetCurrentFrame()`への接続
- 既存のPMX/X描画入口をmmdl描画入口へ置き換える
- Player/BossのMainScene設定をmmdl出力へ変更する
- AnimationEditorを含む、既存のPMX/X直接描画経路の利用箇所を確認し、実行時にPMX/Xを
  読まないようにする
- mmdlが存在しない、読み込みに失敗する、関連マテリアルやクリップが壊れている場合の
  明示的なエラー処理

## Out of Scope

- 実行時のPMX/Xへのフォールバック
- 実行時のPMX/Xからmmdlへの自動変換
- `.mstc`の描画経路
- モーフ、IK、物理、SDEF完全再現、VMD補間曲線
- 既存バイナリフォーマットの意味変更
- Action Timeline Editor本体

## Agreed Design

- ゲーム実行時はmmdlだけを読み、PMX/Xは一切参照しない。
- `.mskn`の頂点BoneIndicesはSkinBoneを直接指さず、SkinSlot配列を指す。
- SkinSlotが実BoneIndexとメッシュ・ボーン固有のOffsetMatrixを持つ既存設計を維持する。
- mmdlが見つからない場合は、黙って別形式へ切り替えず、入力パスを含むエラーを報告する。
- まず既存の描画・スキニング処理をできるだけ再利用し、mmdl専用の新GPUアルゴリズムを
  増やさない。既存APIと互換しない箇所だけ最小限のランタイムモデル実装を追加する。

## Required Learning Checkpoints

実装後、ユーザーが以下を説明できることを確認する。

1. `.mskn`の頂点、SkinSlot、SkinBone、Submeshの参照関係。
2. SkinSlotのOffsetMatrixと現在のBone姿勢を合成する理由。
3. `.mmat`をメッシュのサブメッシュ描画へ結び付ける方法。
4. `.mclp`の時間・キーを既存のUpdate/SetCurrentFrameへ接続する方法。
5. mmdlがない場合にフォールバックしないことの利点と欠点。

## Implementation Requirements

- `RuntimeFormatIO::ReadMskn`、`ReadMmat`、`ReadMclp`を利用し、バイナリの個別解釈を
  描画クラスへ重複実装しない。
- mmdlのロード失敗は成功扱いにせず、既存のDebugLog/エラー経路で明示する。
- `.mskn` Version 3のuint32メッシュインデックスをGPUインデックスバッファへ正しく渡す。
- SkinSlotのBoneIndexとOffsetMatrixを失わない。
- `.mmat`の色、スペキュラ、トゥーン、スフィアマップ、テクスチャパスを可能な範囲で
  既存シェーダー／マテリアル設定へ接続する。未対応の表示要素は黙って同一視せず、
  既存制約をCompletion Reportへ記録する。
- `.mclp`は既存のアニメーションAPIへ接続し、クリップ名指定と外部フレーム指定を維持する。
- 既存PMX/Xクラスを削除せず、変換ツールの入力側としてビルド可能な状態を保つ。
- 変更した`.cpp`/`.h`はUTF-8 BOMを持ち、`using`宣言を追加しない。

## Acceptance Criteria

- MainSceneのPlayer/Bossがmmdlから読み込まれて描画される。
- ゲーム実行時にPlayer/BossのPMX/Xを直接開かない。
- `.mskn`のメッシュ、ボーン、SkinSlot、サブメッシュが読み込める。
- `.mmat`のマテリアル参照が既存描画へ渡る。
- `.mclp`の少なくとも既存クリップ再生または外部フレーム指定が動作する。
- mmdl欠落時に明示的なエラーとなり、PMX/Xへフォールバックしない。
- Debug/Release x64ビルドが0エラー0警告で成功する。
- 既存RuntimeConverterのビルドと変換動作を壊さない。

---
