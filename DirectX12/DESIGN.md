# 設計方針・進捗メモ

コード規約は`README.md`を参照。ここには設計の考え方と、今の到達点・未着手項目をまとめる。

## モデルデータの共通化

PMX/PMDそれぞれのバイナリ形式を読むパーサーと、ゲームが実際に使うデータを分離している。

- `SourceCode/10_Ggraphic/Model/ModelData.h` — フォーマットを問わない共通データ(`Model::Vertex` / `Model::Material` / `Model::Bone` / `Model::ModelData`)。頂点レイアウトはPMXの4ボーン形式を基準にしており、PMDの2ボーンデータもここに変換して格納する。
- `SourceCode/10_Ggraphic/Model/IModelParser.h` — `Load(FilePath, ModelData&)`を持つパーサーの共通インターフェース。新しいモデルフォーマットに対応するときは、これを実装したパーサーを追加すればよい。
- `SourceCode/10_Ggraphic/Model/PMXParser.h/.cpp`、`PMDParser.h/.cpp` — 各フォーマットの生バイナリ解析だけを担当。GPUリソース生成やアニメーション実行時ロジックは`PMXActor`/`PMDActor`側に残している。

## 未着手・保留中の項目

- `PMDRenderer`/`PMXRenderer`のパイプライン生成・ルートシグネチャの共通化(データ層は揃ったので次のステップ候補)
- `Singleton<T>`テンプレートの活用(`MeshManager`/`GameTime`が現状は独自にシングルトンを手書きしている)
- `PlayAnimation()`/`StopAnimation()`の実装(現状は空スタブで、`Update()`が常時再生している)
- `Character`/`GameObject`基底クラス、`Camera`クラスの新設
