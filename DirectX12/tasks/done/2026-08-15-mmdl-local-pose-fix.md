# Task: mmdlスキニング姿勢・クリップ対応・プレビュー縮尺の修正

## Goal

PMX/Xから変換した`.mskn`/`.mclp`をランタイム表示した際に発生している、
以下の変形不具合を修正する。

- PMX由来モデルが縦方向へ大きく引き伸ばされる。
- X由来モデルは静止表示が比較的正しいが、アニメーション中に各部位が中心へ集まる。
- 別モデル用`.mclp`が読み込まれ、偶然BoneIndex範囲内に収まる場合に誤再生される。
- Model PreviewでPMX由来モデルにもX向けの固定15倍スケールが適用される。

変換元ごとの差異はオフライン変換時に吸収し、ランタイムは変換元を知らずに
「完全な親相対ローカル姿勢」を補間・再生できる状態にする。

## Confirmed Root Causes

1. `RuntimeConverter::ConvertPmx()`が全SkinSlotの`OffsetMatrix`へ単位行列を保存している。
   PMX頂点はモデル空間のバインド姿勢にあるため、実行時の
   `OffsetMatrix * CurrentBoneWorld`でボーンのバインド位置が二重適用される。
2. `RuntimeConverter::ConvertX()`は位置・回転・スケールのキー時刻をmapで結合した際、
   その時刻に存在しないPositionをゼロ、Rotationを単位、Scaleを1で補っている。
   元Xではチャンネルごとにキー時刻・終端が異なるため、Positionが欠けた時刻に
   ボーンの親からの距離がゼロになり、中心へ集まる。
3. PMXのVMD Positionはバインド姿勢からの差分だが、現在はそのまま完全な
   ローカルPositionとしてランタイムへ渡している。
4. `XActor::LoadRuntimeModel()`が`mmdl/mstc`直下の全`.mclp`を読み、BoneIndexの
   最大値だけで互換性を推測している。同名クリップも変換時に上書きされる。
5. `ModelPreviewPanel`が全`.mskn`へ固定15倍スケールを適用している。実データでは
   X由来の高さは約1.7～2.1、PMX由来は約20である。
6. 現在の`UpdateBoneMatrices()`は親Indexが子Indexより小さいX由来の順序を前提に
   1回走査する。実測でPMX由来1モデルに親Indexが子より後ろのボーンが2本ある。

## Agreed Design

- `.mclp`の各`Keyframe`は変換元を問わず、対象ボーンの**完全な親相対ローカル姿勢**
  (`Position`/`Rotation`/`Scale`)を保存する。
- PMX+VMDは、PMXの親相対バインド位置へVMDの差分Positionを加えて完全なPositionを作る。
  PMXのバインドRotationは単位、Scaleは1なので、VMD RotationとScale=1を保存する。
- Xは全チャンネルのキー時刻の和集合を使い、各時刻でPosition/Scaleを線形補間、
  RotationをSlerpする。チャンネル自体が無い場合だけ、そのボーンのバインド成分を使う。
  前後範囲外はそのチャンネルの先頭/末尾キーでクランプする。
- PMXのSkinSlotは各ボーンにつき1枠とし、`OffsetMatrix`へそのボーンの
  バインド時モデル空間行列の逆行列を保存する。PMXはバインド回転・スケールを
  持たないため、現在の仕様ではワールド位置の逆平行移動と等価になる。
- PMXボーンは変換時に親が子より前へ来るトポロジカル順へ並べ替える。
  Bone、SkinSlotのBoneIndex、VMDトラックのBoneIndexを同じold→new対応で再マップする。
  頂点のBoneIndicesはSkinSlot Indexなので、SkinSlot配列自体を並べ替えない場合は
  頂点Indexを書き換えず、各SkinSlotのBoneIndexだけを再マップする。
- PMXの循環親子関係、自己参照、範囲外親は変換エラーにする。Xの不正な親Indexは
  既存方針どおりルートへ補正し、警告付きで近似変換を続ける。ランタイム読み込み側では
  `ParentIndex == -1 || (0 <= ParentIndex < ChildIndex)`を検証して壊れたmsknを拒否する。
- `.mclp`ファイル名を`<model stem>__<clip stem>.mclp`とし、ClipName自体は従来名を維持する。
  `XActor`は読み込んだ`.mskn`のstemと一致する接頭辞の`.mclp`だけを読む。
  これにより`Cube`と別PMXの`LateralMove`等が衝突・混入しないようにする。
- MCLPのデータレイアウトは変えないが、Keyframeの意味を完全ローカル姿勢へ確定するため
  MCLPだけVersion 2へ上げ、旧Version 1を読まない。コメントと`DESIGN.md`も更新する。
- Model Previewは頂点AABBのローカル高さから、表示上の目標高さ20へ合わせる均一スケールを
  計算する。高さがほぼ0の場合は1倍へフォールバックする。任意の極端な値を避けるため、
  必要なら妥当な上下限へclampする。
- `XActor`はランタイムモデル読込時にローカル高さを計算し、Previewから取得できるようにする。
  Release構成でも参照可能なAPIにし、1回のロード時計算に限定する。

## Scope

- `RuntimeConverter.cpp`のPMX OffsetMatrix生成、PMXボーン順序、PMX/VMD完全姿勢変換
- `RuntimeConverter.cpp`のXチャンネル補間と完全姿勢変換
- PMX/X両方のモデル名付きMCLPファイル名
- `RuntimeFormatIO.cpp`のMCLP Version 2化
- `RuntimeFormat.h`のKeyframe意味・親順序のコメント明確化
- `XActor.cpp/.h`のモデル固有クリップ読み込み、親順序検証、ローカル高さ計算/API
- `ModelPreviewPanel.cpp`のAABB高さベース自動スケール
- `DESIGN.md`の変換・ランタイム規約更新
- RuntimeConverterの再ビルドと`Data/Model/mmdl`の再変換
- 変換後データの構造検査とDirectX12 Debug/Releaseビルド

## Out of Scope

- SDEF完全再現（既存方針どおりBDEF2近似）
- IK、モーフ、物理、VMDベジェ補間
- ルートモーション抽出
- MCLPへSkeleton hash/IDを追加するフォーマット拡張
- `.mstc`静的メッシュ描画
- PMX/Xの実行時フォールバック
- シェーダーの行列規約変更
- カメラ、DockSpace、Scene Viewの変更
- unrelatedなリファクタリング

## Required Learning Checkpoints

実装後、ユーザーが次を説明できる状態にする。

1. バインド姿勢で`InverseBind * BindWorld`が単位行列になる理由。
2. Xの存在しないチャンネルをゼロで補うと、なぜボーンが中心へ集まるか。
3. PMXのVMD Positionへ親相対バインド位置を足す理由。
4. クリップファイル名をモデル名で名前空間化する理由。
5. 親を子より前へ並べると、毎フレーム1回の前方走査でワールド姿勢を作れる理由。

## Implementation Requirements

- 現在の未コミット変更はすべてユーザーの作業として保持し、無関係な差分を戻さない。
- 既存の`XParser`のAnimationKey値は、旧`XActor`で正しく再生できていた完全な
  ローカルチャンネル値として扱う。追加のbind加算をXへ行わない。
- Xチャンネル補間は変換時だけ行い、ランタイムの毎フレーム処理を複雑化しない。
- PMXボーン再マップは、Bone/SkinSlot/VMD trackの全参照先で一貫させる。
- `RuntimeFormatIO::WriteMskn()`/`ReadMskn()`でも親順序の不変条件を検証する。
- MCLP VersionはMSTC等の共通Versionから分けた専用定数にする。
- `XActor`は接頭辞の一致をファイル名stemに対して厳密に確認し、別モデル用クリップを
  BoneCountだけで採用しない。接頭辞一致後もBoneIndex範囲検証は維持する。
- 変換再実行前に既存の生成済み`.mclp`が旧命名のまま残らないようにする。ただし削除対象は
  `Data/Model/mmdl/mstc`配下の生成物に限定し、入力PMX/X/VMDやユーザーソースを削除しない。
- 自動変換を複数回実行しても、古いモデル固有クリップが残らない仕組みにする。
- 変更した`.cpp`/`.h`はUTF-8 BOMを持つ。`.hlsl`は変更しない。
- `using`宣言・型エイリアスを追加しない。既存の無関係な`using`は今回触らない。
- コーディング規約に従い、意図が分かる1行コメントを付ける。

## Verification Requirements

1. RuntimeConverter Debug/Release x64をビルドする。
2. 引数なしRuntimeConverterを実行して全PMX/Xを再変換する。
3. 再変換後、以下を機械的に確認する。
   - PMX由来SkinSlotのOffsetMatrixが全単位行列ではない。
   - 全msknで親Indexが`-1`または子Index未満。
   - `.mclp`が`<model>__<clip>.mclp`命名で、旧グローバル名が残らない。
   - X由来MCLPの各キーに、その時刻で補間された完全なPosition/Rotation/Scaleが入る。
   - PMX由来MCLPのPositionが親相対bind位置+VMD差分になっている。
4. `scripts/check-bom.ps1 -Fix`を実行する。新規ファイルは必要なら先に追跡対象へ追加するが、
   タスク完了時にコミットはしない。
5. `scripts/build.ps1`でDirectX12 Debug x64とRelease x64を独立ビルドし、0エラー0警告を確認する。
6. 可能なら`x64/Debug/DirectX12.exe`を起動し、少なくとも起動直後クラッシュがないことを確認する。
   GUIの最終的な見た目はユーザーの実機確認が必要なら明記する。

## Acceptance Criteria

- PMX由来モデルのバインド姿勢がボーン位置の二重適用で縦に伸びない。
- X由来モデルのアニメーションがPosition欠落時刻で中心へ集まらない。
- PMX+VMDが完全な親相対ローカル姿勢として再生される。
- どのモデルも別モデル用MCLPを読み込まない。
- 親Indexが後方参照だったPMXモデルも正しい順序へ変換される。
- Preview上でX/PMX由来モデルが概ね同じ高さに自動調整される。
- VMDなしPMXはクリップなしのバインド姿勢で読み込める。
- Player/Bossの既存クリップ名APIを壊さない。
- RuntimeConverterとDirectX12のDebug/Release x64が0エラー0警告で成功する。
- 実行時に`ErrerMessege`/範囲外MCLPエラーで落ちない。

## Completion Report Notes

- 変更ファイル、各原因に対する修正箇所、変換結果、ビルド結果を分けて報告する。
- 実機GUI確認できなかった項目は成功扱いにせず、未確認として明記する。
- 仕様に疑義がある場合は推測でフォーマットを再設計せず、停止条件として報告する。

## Completion Result

- `.mclp`をVersion 2の完全な親相対ローカル姿勢へ統一した。
- PMXへ逆バインド行列・親優先のボーン順序・VMD差分位置へのbind位置加算を実装した。
- Xは全チャンネル時刻の和集合で完全なSRTを補間し、不正親は警告付きルート近似を維持した。
- クリップを`<model>__<clip>.mclp`へ名前空間化し、Preview縮尺をAABB高さ基準へ変更した。
- 全アセットを再変換し、6 MSKN・40 MCLP・193,867個の非ルートPositionキーを検査した。
  親順序違反、範囲外BoneIndex、NaN/Inf、時刻逆転、不審なPositionゼロ、PMX逆バインド不一致は全て0件だった。
- RuntimeConverterとDirectX12本体はDebug/Release x64とも0エラー・0警告だった。
- Debug版を実機起動し、ユーザーがモデルとアニメーションの見た目を「いい感じ」と確認した。
- 入力形式を跨いで同じstemを持つ`X/Cube.x`と`PMX/Cube/Cube.pmx`は、現状どちらも`Cube.mskn`へ出力するため後勝ちになる。
  今回の変形修正とは独立した出力ID設計の課題として、次回以降に扱う。
