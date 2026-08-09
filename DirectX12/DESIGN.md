# 設計方針・進捗メモ

コード規約は`README.md`を参照。ここには設計の考え方と、今の到達点・未着手項目をまとめる。

## モデルデータの共通化

PMX/PMDそれぞれのバイナリ形式を読むパーサーと、ゲームが実際に使うデータを分離している。

- `SourceCode/10_Ggraphic/Model/ModelData.h` — フォーマットを問わない共通データ(`Model::Vertex` / `Model::Material` / `Model::Bone` / `Model::ModelData`)。頂点レイアウトはPMXの4ボーン形式を基準にしており、PMDの2ボーンデータもここに変換して格納する。
- `SourceCode/10_Ggraphic/Model/IModelParser.h` — `Load(FilePath, ModelData&)`を持つパーサーの共通インターフェース。新しいモデルフォーマットに対応するときは、これを実装したパーサーを追加すればよい。
- `SourceCode/10_Ggraphic/Model/PMXParser.h/.cpp`、`PMDParser.h/.cpp` — 各フォーマットの生バイナリ解析だけを担当。GPUリソース生成やアニメーション実行時ロジックは`PMXActor`/`PMDActor`側に残している。

## カメラシステム

`SourceCode/00_Game/31_Camera/`配下。`CameraBase`(抽象基底)を継承する形で用途別に分割している。

- `00_Base/CameraBase` — View/Proj行列、位置/注視点、Yaw・Pitch(`Transform::Rotation`を流用)、FOV/アスペクト比などの共通機能。コピー・ムーブは禁止(継承前提のためスライシング防止)。
- `10_First/FirstPersonCamera` — 一人称、WASD移動+矢印キーでの視点回転。
- `20_LookAt/LookAtCamera` — 固定注視点を中心に周回するだけのカメラ(モデル閲覧用)。
- `30_Debug/DebugCamera` — 元々`DirectX12.cpp`に直書きされていたWASD/QEフリーカメラをクラス化したもの。
- `40_Third/ThirdPersonCamera` — `SetTargetPosition()`を毎フレーム呼ぶことで移動対象を追従できる三人称周回カメラ。`Player`未実装のため依存なし。
- `99_Manager/CameraManager` — 名前でカメラを登録・切り替えるだけの薄いクラス。**シングルトンにはしていない**(下記「マネージャーの所有方針」参照)。今は`Main`が`unique_ptr`で直接所有し、`"Debug"`という名前で`DebugCamera`を登録・有効化している。

`DirectX12`側は`SetCamera(View, Proj, Eye)`で行列を受け取るだけで、`CameraBase`/`CameraManager`の存在を一切知らない。`Main`が両者を仲介している(`10_Ggraphic`が`00_Game`に依存しない向きを維持するため)。

## アーキテクチャ方針(決定事項)

- **オブジェクト基底は継承ベース**。コンポーネント合成方式は採用しない。`GameObject`(仮称)は純粋インターフェースにはせず、Transform・Update/Drawフックなど実装を持つ具象基底クラスとする。
- **HPなど横断的関心事は小さいインターフェースを多重継承させて乗せる**。例: `IHealthSystem`のような`I`+PascalCaseの純粋仮想インターフェース(README「3. クラス設計規則」準拠)を`class Character : public GameObject, public IHealthSystem`のように追加で継承させる。単一の巨大な基底クラスに全部詰め込まない。
- **マネージャー類はサービスロケーター方式を採用する**。`SourceCode/99_Utility/ServiceLocator/ServiceLocator.h`(`Provide<T>()`/`Get<T>()`、型ごとに非所有ポインタを1つ保持するだけの薄いクラス)。**所有権は今まで通り所有者側(`Main`等)が持ち、ロケーターは参照経路を提供するだけ**。`CameraManager`は`Main`が`unique_ptr`で所有したまま、`Main::Create()`で`ServiceLocator::Provide<CameraManager>(m_upCameraManager.get())`と登録し、`Main::Release()`で`Provide<CameraManager>(nullptr)`してから破棄する形にした。
- **シングルトンの破棄タイミングは未決定**。`Singleton<T>`(Meyerのシングルトン)は生成順=初回アクセス順で安全だが、破棄順は不定。`GameTime`/`MeshManager`は現状お互い非依存なので問題化していないが、今後依存関係ができる前に方針(例: 明示的な`Shutdown()`の導入)を決める必要がある。

## 今後のロードマップ

依存関係の強い順にStageで区切っている。Stage内は並行して進めてよい。

### Stage 0(決定事項・実装ゼロ)
- [x] オブジェクト基底: 継承ベースで確定
- [x] 横断的関心事(HP等)の乗せ方: 小さいインターフェースの多重継承で確定
- [x] マネージャーの所有方針: サービスロケーター採用、`ServiceLocator`実装済み、`CameraManager`を登録済み
- [x] シングルトンの破棄タイミング方針を決める → `GameTime`/`MeshManager`とも`Singleton<T>`継承を廃止しサービスロケーターへ移行済み。破棄順序は`Main::Release()`で明示的に制御する方針で解決。

### Stage 1(基礎・以降の開発を加速させる)
- [x] 仮想入力コントローラー — Senzan方式(`KeyInput`/`Mouse`/`XInput`/`Input`/`VirtualPad`)を移植・統合済み。
- [x] ImGui — `ImGuiManager`(サービスロケーター経由)を実装・統合済み。`Text`/`Slider`/`Input`/`CheckBox`/`Combo`/`Tweak`を提供、日本語ラベルはANSI→UTF-8自動変換で文字化けなし。
- [ ] デバッグテキスト — `ImGuiManager::Text()`等で代替可能になったため、専用の実装は現状不要と判断(常時表示のオーバーレイ等が別途必要になったら再検討)。
- [x] インターフェイス整理 — `IUpdatable`/`IDrawable`を`SourceCode/00_Game/05_Object/00_Base/`に実装済み。

### Stage 2(ゲームオブジェクトの骨格)
- [x] オブジェクト基底(`GameObject`) — `IUpdatable`/`IDrawable`を多重継承した具象クラスとして実装済み(Transform保持、Update/Drawは既定で何もしないフック)。コピー・ムーブはCameraBase同様に禁止(スライシング防止)。まだ`Main`等どこからも生成・使用されていない(骨格のみ)。
- [ ] キャラ(`Character`) — `GameObject`を継承し、`IHealthSystem`等の小インターフェースを必要に応じて追加継承
- [ ] FSM — Senzan側で作ったPasskeyパターン(特定クラスにのみ公開する`friend`の代替)を移植・参考にする

### Stage 3(ワールド)
- [ ] ファイル — スコープ要確認(汎用I/Oユーティリティなのか、シーン/マップのファイル形式なのか)
- [ ] マップ
- [ ] 当たり判定 — マップの表現が決まらないと設計できないため、マップの後

### Stage 4(UI・演出、最も後)
- [ ] 色 — Color構造体/変換ユーティリティ
- [ ] UIの設計方針決定 → UI実装
- [ ] カットシーンエディター — ツール的性質が強く、前提が揃ってから

### 完了済み(参考)
- モデルデータ層・PMX/PMDパーサー分離・`IModelParser`
- `PMDRenderer`/`PMXRenderer`のパイプライン生成・ルートシグネチャ共通化は未着手のまま(次のステップ候補として残存)
- `GameTime`/`MeshManager`を`Singleton<T>`から`ServiceLocator`へ移行済み(上記「マネージャーの所有方針」参照)
- カメラシステム一式(上記参照)
- `PlayAnimation()`/`StopAnimation()`の実装は未着手のまま(現状は空スタブで、`Update()`が常時再生している)
- PMXパーサーの符号あり/なしIndex読み込みバグ修正(ボーン/マテリアル/テクスチャIndexの「-1=指定なし」を誤読していた)
- 間欠クラッシュの原因調査・修正(`Data\Shader\PMX\Header.hlsli`がビルド出力先に未コピーでシェーダーコンパイルが失敗し、`Main::Create()`が未捕捉例外で`abort()`していた)。`DirectX12.vcxproj`にポストビルドイベントを追加して`Data\Shader`/`Data\Model`/`Data\Sound`を出力先へ確実にコピーし、`Main::Create()`を`try/catch`で保護
- `SourceCode/99_Utility/Diagnostics/`にメモリリーク検知(`MemoryLeakDetector`、CRTヒープのCreate〜Release間の差分検知)とクラッシュダンプ機構(`CrashDumpHandler`、SEH例外・`std::terminate`両方に対応し`Dumps\`へ`.dmp`を書き出す)を追加
- Senzan方式の仮想入力コントローラー(`KeyInput`/`Mouse`/`XInput`/`Input`/`VirtualPad`)を移植・`Main`へ統合
- Dear ImGui(コア+Win32+DX12バックエンド、v1.90.6)を導入し`ImGuiManager`でラップ。`Text`/`Slider`/`Input`/`CheckBox`/`Combo`/`Tweak`(値をC++リテラルとしてクリップボードへコピー)を提供。日本語ラベルは実行時文字コード(ANSI)→UTF-8の自動変換で文字化けを解消
- `MyComPtr`に`Attach`/`As<U>`/変換コピーコンストラクタ/等価比較演算子を追加し本物の`ComPtr`に近づけた
- 未使用の古いPMXシェーダーファイル(`SourceCode/10_Ggraphic/Shader/PMX/`)とvcxprojの空参照を削除
