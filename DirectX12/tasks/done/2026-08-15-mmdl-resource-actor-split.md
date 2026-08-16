# タスク: mmdlランタイムモデルのResource/Actor分離

## 目的

`.mskin/.mmat/.mstc/.mclp`を読む実行時モデルの責務を、旧入力形式名に依存しない名前へ整理する。
現在の`XActor`/`XMesh`は`.X`由来の名前だが、実際にはmmdlランタイムデータを保持・描画しているため、`MmdlActor`/`MmdlMesh`へ移行する。
同時に、複数キャラクターで静的モデル資源を共有できる構造へする。

## 学習項目

### 今回必須

- ResourceとInstanceの違い。
- 複数キャラクターで共有できる不変データと、個体ごとに必要な状態の区別。
- ボーン行列・再生時間・TransformをActor側へ残す理由。
- GPUのVertex/Index/Texture/Material資源を共有する場合の所有権と寿命。
- `IMesh`、`MmdlMesh`、`MmdlActor`、`MmdlResource`の責務分担。
- `unique_ptr`と`shared_ptr`の所有関係、およびGPUリソースを二重生成しない設計。

### 理解推奨

- 現在の`XActor`が行っている`.mskin`読込、マテリアル読込、GPUバッファ生成、アニメーション更新、描画の各処理。
- PSO/ルートシグネチャ/テクスチャディスクリプタをモデルインスタンス間で共有できる条件。
- モデルリソースキャッシュのキーと、同一モデル判定。

### 今回は後回し

- FrontCompositeのオフスクリーンRenderTarget実装。
- `.mstc`と`.mskn`を同じ描画クラスで完全統合すること。
- 非同期ロード、ストリーミング、GPUリソースの遅延破棄。
- PMX/Xの入力パーサー削除。

## 実装方針

- `MmdlResource`: `.mskin`、`.mmat`、必要な`.mclp`メタデータ、頂点/インデックス/テクスチャ/GPU共有リソースを保持する共有オブジェクト。
- `MmdlActor`: `MmdlResource`を参照し、WorldTransform、現在のローカル姿勢、再生中クリップ、個体ごとのボーン変換GPUバッファを保持する。Update/Drawを担当する。
- `MmdlMesh`: 既存`IMesh`を実装する薄いファサード。内部に`MmdlActor`を保持し、既存の`MeshObject`から利用できるAPIを維持する。
- 旧`XActor`/`XMesh`の公開利用箇所、AnimationEditor、ModelPreviewPanel、`PMXRenderer`のfriend宣言、プロジェクトファイルをmmdl名へ変更する。
- 入力`.X`を実行時に読む経路は復活させない。MmdlActorは`.mskin`だけを入力にする。
- まず既存挙動を保つ。描画結果、アニメーション、`IMesh`呼び出し側の意味を変更しない。
- Resource共有が大規模な既存API変更を要求する場合は、無理に実装せず、共有境界と未解決点を報告する。
- `MmdlResource`が単なるファイルパス保持だけにならないようにする。少なくとも`.mskin`のCPUデータ、頂点/インデックスのGPUバッファ、マテリアル定数/テクスチャSRVの共有所有者をResource側へ移す。ボーン行列StructuredBufferとTransform CBはActor側に残す。
- リソースキャッシュを導入する場合でも、今回のスコープでは明示的なグローバルキャッシュを追加せず、`MmdlResource`の共有所有権を既存生成経路に最小限導入する。Resourceは初回Actor生成時に遅延初期化されるため、共有コンストラクタは`shared_ptr<MmdlResource>`を受け取り、constを外して変更しない。

## スコープ外

- FrontCompositeの実描画。
- 新しいモデルフォーマットやシリアライズ仕様の追加。
- PMXActorの削除またはPMX直接読込の廃止。
- 無関係なカメラ、ImGui、アニメーション補間の修正。
- Git commit。

## 受け入れ基準

1. ソースコード上のランタイムmmdlモデルの主要クラス名が`XActor`/`XMesh`ではなく`MmdlActor`/`MmdlMesh`になっている。
2. `MmdlActor`は`.mskin`からモデル資源を読み、`MmdlMesh`は`IMesh`経由で既存ゲームコードから利用できる。
3. モデル資源と個体状態が分離され、少なくとも頂点/インデックス/マテリアル/テクスチャなどの共有対象と、Transform/姿勢/再生状態/ボーン行列などの個体対象がコード上で区別されている。
4. Player/Bossまたは同一モデルの複数インスタンス生成で、静的モデル資源を二重生成しない設計になっている。実際に共有が不可能な場合は理由をレポートする。
5. AnimationEditorとModelPreviewPanelがmmdlクラスを利用する。
6. Debug/x64とRelease/x64ビルドが警告0で成功する。
7. 変更・新規`.cpp`/`.h`がUTF-8 BOM付きで、`using`宣言を追加していない。
8. 既存の未コミット変更を破壊せず、変更範囲をこのタスクに限定する。

## Codexへの実装指示

この仕様書の範囲だけを実装すること。まず既存`XActor`/`XMesh`の責務と利用箇所を調査し、Resource/Actor分離が安全に可能か確認すること。
大規模なRenderer API変更、GPUリソースの遅延破棄、モデルキャッシュの新設が必要になった場合は独断で拡張せず、可能な範囲だけ実装して停止条件として報告すること。
実装後、BOMチェック、Debug/Releaseビルド、差分確認を行うこと。
