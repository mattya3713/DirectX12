# Task (完了): RuntimeFormatのマテリアル外出し(.mmat新設)+SkinSubmesh化+static_assert追加

## 発端

前タスク(`2026-08-15-runtime-format-io.md`)で`.mstc`/`.mskn`/`.mclp`の構造体+
読み書きユーティリティを実装した後、実際のPMX/X変換タスクへ進む前の設計レビュー
過程で2つの見落としが判明した:

1. `.mskn`が単一テクスチャパスしか持てなかったが、実際のキャラクターモデル
   (`player.x`は18メッシュ、`haku.pmx`もテクスチャ多数)は1メッシュに複数
   マテリアルを持つため不十分だった。
2. ユーザーからの構造体パディング/キャッシュラインに関する質問をきっかけに、
   `sizeof`のレイアウトをコンパイル時に固定する`static_assert`を追加すること
   になった。

## 決定事項(ユーザー主導の設計対話)

- マテリアルを`.mstc`/`.mskn`に埋め込まず、4つ目のファイル形式`.mmat`
  (Material)へ外出しし、パス参照する形にした(ユーザー発案: 「モデルは参照先
  だけ持ち複数のモデルで共有のマテリアルを使える構成」)。
- `.mskn`は単一パスではなく、`Model::Material::NumFaceCount`と同じ考え方の
  `SkinSubmesh`配列(マテリアルパス+連続インデックス数)でマルチマテリアルに
  対応。
- 全構造体が4byte境界に揃う型のみで構成されパディングが無いことを確認し、
  `static_assert(sizeof(...) == N)`で固定した。
- CPUキャッシュライン(64byte)の話はファイル形式には直接関係なく、将来の
  ランタイムActor実装時の検討事項として区別・記録した(詳細は`DESIGN.md`参照)。

## 実装(Codexへ委任)

`RuntimeFormat.h`(`MmatHeader`/`MmatData`/`SkinSubmesh`新設、`MstcHeader`/
`MstcData`/`MsknHeader`/`MsknData`改修、全ディスク構造体へ`static_assert`追加)
と`RuntimeFormatIO.h/.cpp`(`WriteMmat`/`ReadMmat`新設、`WriteMstc`/`ReadMstc`を
マテリアルパス参照方式へ、`WriteMskn`/`ReadMskn`をサブメッシュ配列方式へ改修)を
改修。指示は書き込み側のみのバリデーションだったが、Codexは読み込み側にも
サブメッシュ整合性チェック(マテリアルパス終端・IndexCount合計)を対称的に
追加しており、妥当な判断だった。

## Claudeによる独立検証

前タスクで作成したスタンドアロン往復テストを新構造(`.mmat`追加、`MstcData`の
`MaterialPath`、`MsknData`の`Submeshes`配列)に合わせて更新し、以下19項目
全てパスを確認:
- `.mmat`/`.mstc`/`.mskn`/`.mclp`それぞれの往復一致
- 頂点数超過(2形式)、ボーン名未終端、サブメッシュのマテリアルパス未終端、
  サブメッシュIndexCount合計不一致、マジックナンバー破損 — いずれも正しく
  `false`/読み込み失敗になることを確認

本体プロジェクトのビルドも独立して実行し、0エラー0警告を確認済み。

## 変更ファイル

- `SourceCode/10_Ggraphic/RuntimeFormat/RuntimeFormat.h`
- `SourceCode/10_Ggraphic/RuntimeFormat/RuntimeFormatIO.h`
- `SourceCode/10_Ggraphic/RuntimeFormat/RuntimeFormatIO.cpp`
- `DESIGN.md`(マテリアル外出し・パディング/キャッシュラインの方針を追記)

## 副産物

- PMXファイルのSDEF(滑らか変形用特殊スキニング)使用状況を実機検証(一時的な
  診断コードを`Main.cpp`に追加→ブレークポイントで確認→削除)。
  `Data/Model/PMX/haku/SakurabaEma_ByPOWER.pmx`ではSDEFが使われておらず、
  かつ現行の`Vertex.hlsl`もSDEFデータを受け取るだけで実際には使っていない
  ことを確認した上で、SDEF非対応のまま`.mskn`を確定させた。

## 未着手・保留中(DESIGN.mdロードマップ参照)

- PMX/X → `.mskn`(+`.mclp`+`.mmat`)変換ロジック(次タスク)
- オンデマンド変換+キャッシュを行うローダー
- `.mskn`/`.mclp`を実際に描画・アニメーションする新Actor
- `.mstc`用の静的メッシュ描画パス
- Action Timeline Editor本体
