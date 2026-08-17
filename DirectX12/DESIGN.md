# 設計方針・進捗メモ

コード規約は`README.md`を参照。ここには設計の考え方と、今の到達点・未着手項目をまとめる。

## モデルデータの共通化

PMX/PMDそれぞれのバイナリ形式を読むパーサーと、ゲームが実際に使うデータを分離している。

- `SourceCode/10_Ggraphic/Model/ModelData.h` — フォーマットを問わない共通データ(`Model::Vertex` / `Model::Material` / `Model::Bone` / `Model::ModelData`)。頂点レイアウトはPMXの4ボーン形式を基準にしており、PMDの2ボーンデータもここに変換して格納する。
- `SourceCode/10_Ggraphic/Model/IModelParser.h` — `Load(FilePath, ModelData&)`を持つパーサーの共通インターフェース。新しいモデルフォーマットに対応するときは、これを実装したパーサーを追加すればよい。
- `SourceCode/10_Ggraphic/Model/PMXParser.h/.cpp`、`PMDParser.h/.cpp` — 各フォーマットの生バイナリ解析だけを担当。GPUリソース生成やアニメーション実行時ロジックは`PMXActor`/`PMDActor`側に残している。
- `SourceCode/10_Ggraphic/Model/XParser.h/.cpp` — DirectXの`.x`ファイル(テキスト形式)用パーサー。`Data\Model\X\Cube.x`(6面すべて四角形、単一Mesh、ボーン無し)と`Data\Model\X\player.x`(Senzanからコピーした、頭・胴・腕・脚・剣などパーツごとに分かれた91ボーン・18メッシュ・12アニメーションクリップを持つ実践的なリグ付きキャラクターファイル)の2ファイルを対象に実装・検証した。単純な`Load()`(`IModelParser`実装、`Model::ModelData`のみ)と、ボーン・アニメーションクリップも返す`LoadSkeletal()`の2つを提供する。
  - `.x`のMesh/MeshNormals/MeshTextureCoords/MeshMaterialListはそれぞれ独立したインデックス空間を持つ(位置・法線は面ごとに別々のIndex配列、UVのみ位置と同じIndex空間)ため、単純にPosition Indexをそのまま共有Indexバッファとして使うことができない。重複排除(頂点キーでのハッシュ結合)はモデル規模的に不要と判断し、三角形の各コーナーごとに新しい`Model::Vertex`を1つ作る単純な展開方式にした(Indicesは単調増加の連番になる)。
  - N角形(4頂点以上の面。`Cube.x`が該当)はファン三角形分割(`0,i,i+1`)で三角形化する。
  - マテリアルの割り当ては元の面ごとの`MeshMaterialList`のIndex配列を見て、三角形をマテリアルごとにグルーピングしてから`Indices`へ書き出す(PMX/PMD同様、同一マテリアルの区間が連続している前提で`Material.NumFaceCount`を使う描画側と合わせるため)。
  - `.x`のMaterialテンプレートにはAmbient相当のフィールドが無いため、emissiveColorを`Model::Material::Ambient`へ流用している(厳密な等価ではない、という前提で決めた設計上の妥協)。
  - **Frame階層(`FrameTransformMatrix`)・ボーン・スキニング(`SkinWeights`)・キーフレームアニメーション(`AnimationSet`)に対応**。`Cube.x`のような単一Mesh構成では想定していなかったが、Senzan版`player.x`は頭・胴・腕・脚・剣等それぞれ独自のローカル座標系を持つ`Mesh`が`Frame`階層の中にネストされ、`SkinWeights`で滑らかにスキニングされ、`AnimationSet`でボーンごとのキーフレーム(回転/拡縮/移動)が定義された、PMX+VMDに匹敵する規模の本格的なリグ付きモデルだった。ボーン・アニメーション用のデータは`Model::Bone`(PMX用、名前+初期位置のみの単純な形)では表現できないため、`SourceCode/10_Ggraphic/X/XSkeletonData.h`に専用の`XSkeleton::{Bone, SkinSlot, BoneAnimation, AnimationClip, SkeletalData}`を新設した。
    - `Bone`はFrame階層をそのまま反映(名前・親Index・`FrameTransformMatrix`=バインドポーズのローカル変換)。親は必ず自分より小さいIndexになるように構築する(Frame階層を親から子へ辿りながら`push_back`するため)ので、ワールド変換は前から1回なめるだけで計算できる。
    - **`SkinWeights.matrixOffset`はボーンの定数ではなく(メッシュ,ボーン)の組ごとに値が異なる**ことが実機検証で判明した(同じボーン名を複数のメッシュが参照する場合、各メッシュ自身のローカル原点からの相対位置が違うため、オフセット行列の並進成分がメッシュごとに微妙に異なる)。当初は各`Bone`にオフセット行列を1つだけ持たせていたところ、複数メッシュが同じボーンを参照した際に後勝ちで上書きされ、パーツが体幹から分離して表示される不具合が発生した(実機スクリーンショットで確認・修正済み)。対策として、ボーン(階層・アニメーション用)とは別に`SkinSlot`(ボーンIndex+そのメッシュ固有のオフセット行列)を`SkinWeightsブロック1件ごと`に割り当て、GPUのボーン行列バッファは「ボーン数」ではなく「SkinSlot数」(`player.x`ではボーン91に対しSkinSlot131)ぶん確保するようにした。`Model::Vertex::BoneIndices`はボーンではなくSkinSlotのIndexを指す。
    - キーフレーム(`AnimationKey`)の回転はクォータニオンだが、ファイル上の値の並びは`(w,x,y,z)`(スカラー先頭)であり、DirectXMathが要求する`(x,y,z,w)`への並べ替えに加えて**共役(x,y,z成分を反転、wはそのまま)を取ることで正しい向きになる**ことを実機検証で確認した(`XMFLOAT4(-values[1], -values[2], -values[3], values[0])`。左手系/右手系の解釈違いによるものと推測)。単純な並べ替えのみ・反転のみでは上下逆さま/ねじれた姿勢になり、両方を組み合わせて初めて正しい`player_run`の走行モーションが再生されることを、ImGuiパネルに隠れない全身が映るスクリーンショットで確認した。バインドポーズ(アニメーション無し)は`FrameTransformMatrix`をそのまま使うため元々正しく表示されており、この問題はキーフレーム側だけに存在した.
    - `AnimTicksPerSecond`(`player.x`では4800)をパースし、キーフレームの`time`(ファイル固有の目盛り)を実時間へ変換する基準として使う。無ければ既定値4800を使う。
  - **トップレベルの名前付き`Material`定義 + `MeshMaterialList`内の`{name}`参照に対応**。SenzanのX出力は`Material player_player {...}`のようにトップレベルでマテリアルを定義し、各パーツの`MeshMaterialList`からは`{player_player}`という名前参照だけを書く形式だった(全パーツで同じ`Material`を使い回すための一般的なXファイルの書き方)。`ParseChildren`が走査中に名前付き`Material`を`std::unordered_map`へ蓄積し、`ParseMeshMaterialList`が`{`から始まる子オブジェクトを名前参照として解決する。この対応が無いと`TextureFilename`を一切読めず、全マテリアルがテクスチャ無し(白)になる(実際に発生し修正済み)。ボーン名の参照(`SkinWeights`/`Animation`の`{name}`)も同様の理由で、ファイル全体を1回パースし終えてからボーン木を使って解決する2段階構成にした(Materialは定義が参照より前に出現する前提のまま、Bone/Animationは出現順に依存しない設計にした).
  - 検証: スタンドアロンの`cl.exe`単体テストで`Cube.x`(N角形の三角形分割、法線の面ごとの別インデックス解決)と`player.x`(全18マテリアルのテクスチャパス解決、ボーン91・SkinSlot131・クリップ12件それぞれの`BoneAnimations`数とループ長`MaxTime`)を確認。実機(`XActor`経由の実際の描画)でもSenzan版`player.x`が正しく組み立てられ、髪・上着・ズボン・双剣が正しいテクスチャで表示され、`player_run`クリップの再生でポーズが時間とともに変化する(脚が曲がる・コートが揺れる、正しい向きの走りポーズになる)ことをスクリーンショットで確認済み。
- `SourceCode/10_Ggraphic/X/XActor.h/.cpp` — `.x`(XParser経由)を実際に描画・アニメーションするクラス。頂点レイアウト・ルートシグネチャ・シェーダーは`PMXRenderer`とそのまま共用する(専用の`XRenderer`は作っていない — `Model::Vertex`という共通フォーマットの上に構築している以上、PMXRenderer側を「Model::Vertex用の汎用パイプライン」として再利用するのが自然だと判断).
  - `Update()`毎フレーム、再生中クリップがあれば`AnimTicksPerSecond`換算で時刻を進めループさせ、`UpdateBoneMatrices()`で各ボーンのローカル変換(キーフレームが無いボーンはバインドポーズのまま)から階層をたどってワールド変換を求め、SkinSlotごとに`FinalMatrix = OffsetMatrix * ワールド変換`(行ベクトル規約のためオフセットを先に掛ける)を計算してGPUのボーン行列バッファへ書き込む。既存のPMXシェーダー(`Data\Shader\PMX\Vertex.hlsl`)がそのまま使える(スキニング数式は共通).
  - ボーンを持たない(`Cube.x`等)場合は頂点の`BoneWeights`を明示的に全て0にし、シェーダー側の「重み0なら生の頂点座標を使う」フォールバック経路を使う。ルートシグネチャ上はBone StructuredBuffer(t3)に有効なSRVを渡す必要があるため、SkinSlotが無い場合は単位行列1要素のダミーバッファを作成してバインドする。
  - `PMXRenderer`が持つ既定テクスチャ(白/黒)をトゥーン・スフィアマップの代用として使う(`.x`にはその概念が無いため)。
  - **テクスチャ読み込み失敗時の挙動を修正**: 元々`DirectX12::CreateTextureFromFile`はテクスチャ読み込みに失敗すると`MessageBoxA`でアプリ全体をブロックする実装だったが、1枚のテクスチャ欠損でアプリが固まるのは過剰と判断し、`std::cerr`へのログのみに変更した(呼び出し側が`nullptr`を見て復帰できるように)。あわせて`PMXActor::LoadTexture`/`XActor::LoadTexture`を、読み込み失敗時(`nullptr`が返った場合)も白テクスチャへフォールバックするよう修正(元々は空パスの場合しかフォールバックしておらず、読み込み失敗時は該当ディスクリプタスロットが未初期化のまま残る潜在バグだった)。
  - `MainScene`に動作確認用として1体配置(Playerの反対側、15倍スケール)、`PlayAnimation("player_run")`で起動時から再生。ImGuiに`"XActor Animation"`ウィンドウを追加し、`GetClips()`で取得した12クリップ名をボタン一覧表示(再生中のものには"> "を表示)、クリックで`PlayAnimation(name)`を呼んで切り替えられる。
  - 未対応・既知の制約: ルートボーンの移動キーがそのままワールド座標に反映される(いわゆるルートモーション)ため、`player_run`等を再生し続けるとキャラクターがワールド空間を移動し続ける(ループ境界で位置が飛ぶ)。腰位置だけ動かして脚は足元に固定する「ルートモーション抽出」は未実装(Player/EnemyのようにXActorをゲームプレイ用キャラクターとして動かす段になったら検討する)。

## 独自ランタイムモデルフォーマット(Static/Skin/Clip)

Action Timeline Editor(Player攻撃アクションのフレーム単位調整)の設計中に、PMXとXで
アニメーションの時間軸の扱いが根本的に違う(PMXは壁時計経由の秒、Xは`GameTime`秒
×TicksPerSecond)ことが障害になると判明した。`ActionFrame`(30fps換算の共通フレーム
番号、`SetCurrentFrame`で両フォーマットに実装済み)はその場しのぎの変換層に過ぎず、
根本解決のため、PMX/Xを都度パースする現状をやめ、**エンジン専用のランタイム
バイナリフォーマットへインポート時に変換して以後はそれだけを読む**、というアセット
パイプライン方式を導入することにした(ユーザー発案、設計は対話で詰めた)。

### フォーマット構成

3種類に分割する(用途ごとに頂点フォーマットが根本的に違うため混在させない):

- **`.mstc`(Static)** — 静的配置オブジェクト用。頂点は位置・法線・UVのみ
  (タンジェント無し)。テクスチャはベースカラー+**オブジェクト空間法線マップ**の
  2枚。オブジェクト空間法線マップはテクスチャの値がそのままローカル座標系での
  絶対法線方向であり(タンジェント空間のような「基準からの相対的なズレ」ではない)、
  ピクセルシェーダーでの法線再構成にタンジェント/従法線が不要になる。ただし
  **変形しない剛体オブジェクト限定**(オブジェクトのワールド回転だけで正しく
  追従するが、スキニングで面が動くと破綻するため)。だからこそStatic専用とし、
  Skinには使わない。
- **`.mskn`(Skin)** — メッシュ+スケルトン(ボーン階層)のみ。アニメーションは
  含まない。頂点は位置・法線・UV・ボーンindex×4・ボーンweight×4。ボーン階層は
  「親indexを持つフラット配列」(ルートは-1)で持つ(PMX同様の標準的な持ち方)。
- **`.mclp`(Clip)** — `.mskn`のスケルトンに対する疎なキーフレームアニメーション
  (ボーンごとの位置・回転・スケールのキーフレーム列、間はスラープ/線形補間)。
  Skinとは別ファイル。

### 主な設計判断とその理由

- **メッシュ/スケルトンとアニメーションを分離する(PMX+VMD方式)**: 現状Xは
  1ファイルにアニメーションまで埋め込む方式だが、Action Timeline Editorは
  「攻撃アクション1つだけを繰り返し細かく調整する」用途なので、キャラクター本体
  (重い頂点・スケルトンデータ)を毎回書き出し直さずクリップ単位で編集・追加
  できるPMX+VMD方式の方が実用上有利、と判断してこちらを採用した。
- **疎なキーフレーム+補間を採用**(1フレームごとの姿勢を全部焼き込む方式は不採用):
  時刻を浮動小数点(秒 or ActionFrame)で持てるため、整数フレームだけでなく
  途中の時間でも正確に補間姿勢を計算できる。焼き込み方式は逆に整数フレーム
  単位でしか止められず、Editorでの精密なスクラブ用途にはむしろ不利と判断した。
  既存のPMX+VMD(補間の考え方)からの知見も流用できる。
- **バイナリ形式(ヘッダー+サイズ)**: 頂点・キーフレームのような大量の数値配列を
  テキスト(JSON)で持つと読み込み速度・サイズの面で不利なため。プロジェクトの
  Combat設定等はJSONのままで、モデル・アニメーションデータのみバイナリ化する
  住み分け。
- **変換タイミングはエンジン内でオンデマンド+キャッシュ**(別プロセスの変換
  ツールは作らない): 個人開発規模では身軽さを優先。ローダーが独自形式ファイルの
  有無・PMX/Xソースより古いかを見て、必要なら都度変換してディスクにキャッシュする。
- **Static/Skinで頂点フォーマットが違う理由**: Skinはボーンindex/weightが要る
  スキニング用頂点、Staticはそれが不要な軽量頂点。混在させると全メッシュが
  スキニング用の重い頂点フォーマットを強制されるため分離した。

### バイナリレイアウトの確定事項

- 全フォーマット共通: 先頭にマジックナンバー(4byte)+バージョン(uint32)。可変長
  データ(テクスチャパス・クリップ名等)は**必ずファイル末尾**に置き、固定サイズの
  構造体配列(頂点・ボーン・キーフレーム等)を先に連続させる。理由: ヘッダーに
  全セクションの個数(=サイズ)を持たせておけば、ヘッダーを読んだ時点で残り
  バイト数を計算でき、**残り全部を1回のreadでメモリに読み込み、ポインタ計算で
  各セクションを切り出す**という実装ができる(小分けに何度もreadするより速い)。
  可変長データを先頭寄りに置くと後続の構造体配列がバイト境界でズレるため、
  常に末尾に置く。
- Index型は`uint16_t`(1メッシュ最大65,536頂点。超過時は変換ツール側で
  エラー停止する)。
- ボーン名は固定長`char Name[64]`。変換元の名前が64byte(終端込み)を超える
  場合は変換ツール側でエラー停止する(黙って切り詰めない)。
- 回転は全てクォータニオン`(x,y,z,w)`(`DirectXMath`の`XMFLOAT4`と同じ成分順)。
  X形式で問題になった`(w,x,y,z)`+共役変換のような沼を最初から避ける狙い。
- Skinの法線マップは今回スコープ外(頂点にタンジェントを持たせない)。将来
  追加する場合は`Tangent(float3)`をSkinVertexへ追加するバージョンアップが必要。
- **SDEF(PMXの滑らか変形用特殊スキニング)は非対応**。`SkinVertex`はボーン
  index/weightのみ(BDEF相当)を持ち、`SDEF_C/R0/R1`は変換しない。理由: (1)現行の
  `Vertex.hlsl`は元々SDEFデータを受け取るだけで実際のブレンド計算には使っていない
  (既に事実上無効)、(2)実際にPMXファイル(`Data/Model/PMX/haku/
  SakurabaEma_ByPOWER.pmx`)を`PMXParser`で読み込み、`case 3: // SDEF`に
  ブレークポイントを置いて検証したところ、このモデルではSDEFが使われていなかった
  ことを確認した。関節部の見た目がわずかに変わる可能性はあるが、実害が出る
  サンプルが見つかっていないため今回は割り切った。
- 構造体定義の詳細は実装先のヘッダーファイル自体を正とする(このファイルには
  設計判断の記録のみ残す)。
- **マテリアルは`.mstc`/`.mskn`に埋め込まず、`.mmat`という4つ目のファイル形式へ
  外出しし、パス参照する形にした**(ユーザー発案)。理由: (1)実際のキャラクター
  モデル(`player.x`は18メッシュ、`haku.pmx`もテクスチャ多数)は1メッシュに複数
  マテリアルを持つため、`.mskn`が単一テクスチャパスしか持てないのは不十分だった。
  (2)モデルはマテリアルへの参照だけを持つ形にすることで、複数モデルで同じ
  マテリアルを共有でき、テクスチャ差し替えのたびにモデル本体を書き出し直す必要が
  なくなる。`.mskn`側は`Model::Material::NumFaceCount`と同じ考え方(マテリアル
  ごとに連続したインデックス数を持つ`SkinSubmesh`の配列)でマルチマテリアルに
  対応する。マテリアルパスは固定長`char MaterialPath[128]`(ボーン名と同じ理由・
  同じ「超過時エラー」方針)。
- **MSKN v4ではSkinSubmeshへモデル固有のFrontComposite役割(Normal/Source/
  RelaxedOccluder)を持たせ、ヘッダーへOpacity/MaxDistanceを保存する**。共有される
  MMATへ役割を入れないのは、同じマテリアルでもモデル内の描画用途が異なるため。
  変換元隣接の`*.mmdl.json`が無い場合は全サブメッシュをNormalとして従来描画し、
  設定がある場合だけ番号・値を検証してMSKNへ反映する。旧MSKNはバージョン不一致で
  明示的に拒否する。
- **構造体はメンバー間・末尾にパディングが生まれないよう、4byte境界に揃う型
  (float/uint32_t、および4の倍数サイズの固定長`char`配列)だけで構成している**
  (ユーザー指摘を受けて確認・徹底)。バイト列としてそのまま読み書きする都合上、
  パディングは無駄なだけでなく将来メンバー追加時に気づかずレイアウトが変わる
  リスクがあるため。各構造体に`static_assert(sizeof(...) == N)`を付けて
  意図しないレイアウト変更をコンパイル時に検知できるようにする。
  - **CPUキャッシュライン(64byte)は別の話として区別する**: ファイル形式
    (ディスクI/O中心)には直接関係なく、関係してくるのは将来実装する
    ランタイムActor側(ボーン階層走査・スキニング計算のような毎フレーム大量
    データを舐める処理)。ただし本プロジェクトの実績(上記「パフォーマンス」節)
    では効いたのはアルゴリズム上の改善(O(n²)→O(n))であり、キャッシュ配置の
    微調整ではなかった。よって今から先回りして構造を複雑にせず、Actor実装時に
    実測して必要なら見直す方針とする。GPU頂点バッファ用の`SkinVertex`のような
    「GPUがそのまま読む」データは、1頂点分をまとめて並べる現状の形(AoS)が
    GPU側の頂点フェッチとも整合しており、これはこのままでよい。

### 変換ロジック実装前に判明した注意点(次タスク向け)

- **PMXとXでボーン位置の基準が違う**。PMX(`PMXParser.cpp`が`bone.Position`へ直接
  readする値)は**ワールド空間**位置。X(`XSkeletonData.h`のコメントに明記)の
  `FrameTransformMatrix`は**親ボーンからの相対(ローカル)変換**。新フォーマットの
  `SkinBone::BindPosition/BindRotation/BindScale`はローカル(親相対)で統一する
  ため、PMX変換時は「自分のワールド位置 − 親のワールド位置」でローカル位置を
  計算する必要がある(PMXはバインド時の回転・スケールを持たないため、回転は
  単位クォータニオン・スケールは1として扱ってよい)。X側はそのまま使える。
- PMX/X両方を変換対象にする(Player/BossはX、`Data\Model\PMX`配下のテスト
  モデルはPMXのため)。

### PMX/X変換パイプライン実装済み事項

- `RuntimeConverter`はPMX+任意VMD、またはXを明示的に呼び出して`.mskn`/`.mmat`/`.mclp`へ変換する。ゲーム起動時の自動変換は行わない。
- Xの変換用中間データは、頂点ごとのMesh変換とメッシュ・ボーン組ごとのSkinSlotを保持する。共通モデル空間へ頂点を移す際はSkinSlotのオフセットへMesh変換の逆行列を合成する。不正な親ボーンはルートとして警告付きで近似し、逆行列を作れない場合や固定長名の超過はエラーにする。
- PMXのSDEFは`PMXParser`で検出し、`C`/`R0`/`R1`を無視してBDEF2相当へ警告付きで近似変換する。VMDは30fps、XのAnimationSetは`TicksPerSecond`で秒へ正規化し、補間曲線は保存しない。
- `.mmat`は拡張値と4本の固定長パスを含むVersion 2、`.mskn`はSkinSlot配列とuint32インデックスを含むVersion 3とし、旧Versionは読み込まない。マテリアル共有はFNV-1aを候補検索だけに使い、全フィールド比較後に再利用する。

### mmdlスキニング姿勢・クリップ規約

- `.mclp`はVersion 2とし、各`Keyframe`へ完全な親相対ローカル姿勢を保存する。PMX+VMDはPMXのバインド位置へVMD差分を加え、Xは全チャンネルの時刻和集合で各成分を補間してから書き出す。
- PMXボーンは親が必ず前に来るトポロジカル順へ並べ替え、SkinSlotとVMDトラックの参照も同じ対応で再マップする。SkinSlotのOffsetMatrixはバインド時モデル空間行列の逆行列とする。
- クリップ名は`<model>__<clip>.mclp`でモデルごとに名前空間化し、ランタイムはMSKNのstem接頭辞に一致するクリップだけを読み込む。ModelPreviewは頂点AABBの高さから目標高さ20へ自動縮尺する。

### mmdlランタイムのResource/Actor分離

- 旧`XActor`/`XMesh`は入力形式名と実行時の責務が一致しなくなったため、`MmdlActor`/`MmdlMesh`へ改名した。実行時は`.mskin`だけを読み、PMX/Xはオフライン変換入力として扱う。
- `MmdlResource`はモデルの基準CPUデータと、Vertex/Indexバッファ、マテリアル定数バッファ、Base/Toon/Sphereテクスチャを所有する。同じ`shared_ptr<MmdlResource>`を複数の`MmdlMesh`へ渡した場合、これらの静的GPU資源を再生成しない。
- `MmdlActor`はWorldTransform、再生クリップと時刻、外部指定フレーム、Transform CB、ボーン行列StructuredBuffer、Actor専用ディスクリプタヒープを個体ごとに持つ。個体ごとに変化する資源は共有しない。
- Resourceは最初のActor生成時に遅延初期化されるため、共有コンストラクタは`shared_ptr<MmdlResource>`を受け取る。グローバルなモデルキャッシュ、非同期ロード、GPU遅延破棄は今回は導入しない。
- 現在のActorは既存コードへの影響を抑えるため、Resourceの基準CPUデータをローカル配列へ複製して参照している。CPU側の完全なzero-copy化は、Actorの参照寿命設計と合わせて後回しとする。

### 実装順序

Action Timeline Editor本体より**先に**この基盤を完成させる方針(ユーザー決定)。
`PMXParser`/`XParser`は今後「オフライン変換の入力読み取り専用」という役割に
変わる想定(既存のランタイム側`PMXActor`/`MmdlActor`との置き換え範囲は実装しながら
判断する)。

**2026-08-15〜17に基盤が完成した**(詳細は`tasks/done/2026-08-15〜17`の各ファイル
参照): `RuntimeConverter`(PMX/X→`.mskn`/`.mmat`/`.mclp`変換、自動スキャン対応)、
`MmdlActor`/`MMdlMesh`/`MMdlResource`/`MmdlRenderer`(新形式を描画・アニメーション
するランタイム)。**Player/Bossは既に両方とも`MMdlMesh`(`player.mskn`/
`boss.mskn`)へ移行済み**であり、当初の課題だった「PMXは秒駆動・Xはtick駆動」の
不一致はゲームプレイコードから解消されている。`10_Ggraphic`配下も責務別に
再編済み(`10_Device`/`20_Render`/`30_Asset/{Parser,RuntimeFormat,RuntimeModel/MMdl}`/
`90_Legacy`)。あわせてVisual Studio拡張機能(VSIX)`Runtime Model Viewer`
(`Tools/ModelViewerExtension`)も新設し、`.mskn`/`.mmat`をVisual Studio内で
確認できるようにした(メニュー非表示問題を継続調査中)。

## Combat/アニメーション時間軸の統合(Action Timeline Editorの前提・続き)

上記の基盤完成を受けて、Action Timeline Editor本体に進む前に残っていた設計課題
(`Combat`の秒ベース時間管理と、`MmdlActor`のActionFrame(30fps)をどう繋ぐか)を
対話で詰めた。

### 検討の経緯

- 当初案(a): `Combat`自体を秒からフレームベースへ書き換える。
- 当初案(b): JSON側はフレームで保存し、読み込み時に秒へ変換する。
- ユーザーから「フレームベースだとエディタとゲームでズレそう、秒で統一したい」
  という指摘。検証の結果、`MmdlActor::SetCurrentFrame`の式
  (`m_CurrentTime = ActionFrame / 30 * TicksPerSecond`)を使う限り、
  「秒を真実の値とし、プレビュー時だけ`ActionFrame = 秒 × 30`を計算する」
  やり方は、ゲーム内の通常再生(`m_CurrentTime += 経過秒 × TicksPerSecond`)と
  数式的に完全に一致することを確認した(30が計算途中で相殺されるため、
  秒を保持する分には案(a)(b)いずれとも異なる、より単純な第3の形になる)。
- 次に「区間ごとのアニメーション速度」(攻撃の振りだけ速くしたい等)を検討した際、
  「Combatが独立した時計を持ち続ける限り、速度を変えるたびに2つの時計が
  ズレる」という問題が判明。
- **最終決定(ユーザー発案)**: 実行時の速度倍率機能は作らない。区間ごとの速さは
  **アニメーションクリップ自体のキーフレーム間隔**で表現する(`.mclp`は疎な
  キーフレーム形式なので、間隔を詰めれば速く・空ければ遅く見える、という表現力を
  既に持っている)。クリップは常に等速(1倍)で再生し、**`Combat`は独自の時計を
  持つのをやめて、紐づく`MmdlActor`の現在の再生位置(秒換算)をそのまま自分の
  経過時間として使う**。こうすることでCombat側とアニメーション側は最初から
  同じ1本の時間軸を共有するだけになり、速度調整をしても原理的にズレない。

### 必要な実装(次タスク)

- `MmdlActor`に現在の再生位置を秒で返すゲッターを追加する(内部は
  `m_Skeleton.TicksPerSecond`基準のtick単位で持っているため変換が要る)。
- `IMesh`インターフェースに同等のメソッドを追加し、`MMdlMesh`が`MmdlActor`へ
  委譲する形にする(既存の`SetCurrentFrame`/`PlayNamedClip`と同じパターン)。
- `MeshObject`に委譲メソッドを追加する(既存の`GetLocalHeight`等と同じパターン)。
- `Combat.h/.cpp`の`m_CurrentTime`(`GameTime::GetDeltaTime()`を積算する独自時計)を
  廃止し、`GetPlayer()`経由で紐づくメッシュの再生位置を毎フレーム読むように
  変更する。`ColliderWindow`・`ComboStartTime`等のJSON側スキーマは変更不要
  (今のまま秒).

## カメラシステム

`SourceCode/00_Game/30_Camera/`配下。`CameraBase`(抽象基底)を継承する形で用途別に分割している。

- `00_Base/CameraBase` — View/Proj行列、位置/注視点、Yaw・Pitch(`Transform::Rotation`を流用)、FOV/アスペクト比などの共通機能。コピー・ムーブは禁止(継承前提のためスライシング防止)。
- `10_First/FirstPersonCamera` — 一人称、WASD移動+矢印キーでの視点回転。
- `20_LookAt/LookAtCamera` — 固定注視点を中心に周回するだけのカメラ(モデル閲覧用)。
- `30_Debug/DebugCamera` — 元々`DirectX12.cpp`に直書きされていたWASD/QEフリーカメラをクラス化したもの。
- `40_Third/ThirdPersonCamera` — `SetTargetPosition()`を毎フレーム呼ぶことで移動対象を追従できる三人称周回カメラ。`Player`未実装のため依存なし。
- `99_Manager/CameraManager` — 名前でカメラを登録・切り替えるだけの薄いクラス。**シングルトンにはしていない**(下記「マネージャーの所有方針」参照)。今は`Main`が`unique_ptr`で直接所有し、`"Debug"`という名前で`DebugCamera`を登録・有効化している。

`DirectX12`側は`SetCamera(View, Proj, Eye)`で行列を受け取るだけで、`CameraBase`/`CameraManager`の存在を一切知らない。`Main`が両者を仲介している(`10_Ggraphic`が`00_Game`に依存しない向きを維持するため)。

## パフォーマンス

実機で60FPSに安定しない(Releaseビルドでも重い)という報告を受けて調査・修正した2件。

- **`PMXActor::UpdateBoneGlobalTransforms()`のO(N²)化**: ボーン階層を再帰でたどる際、「このボーンの子は誰か」を毎フレーム全ボーンから線形探索していたため、ボーン数nに対しO(n²)かかっていた。しかも`PMXMesh`が内部で専用の`PMXActor`を持つ設計のため、`MainScene`に置いている同一の重いPMXモデル(初音ミク、単独表示用+Player+Enemyの3体)がそれぞれ独立にこの重い処理を毎フレーム行っていた。対策として`RuntimeBone`に`ChildIndices`(自分を親に持つボーンのIndex一覧)を追加し、`InitializeRuntimeBones()`(コンストラクタで1回のみ)で構築、毎フレームの`UpdateBoneGlobalTransforms()`はこのリストを辿るだけにしてO(n)へ削減した。
- **`DirectX12::EndDraw()`が毎フレームCPU/GPUを完全に直列化していた**: `Present()`直後に`WaitForGPU()`(GPU完了までCPUを止めて待つ)を呼んでからコマンドアロケータを`Reset()`する実装だったため、1フレームの所要時間が「CPU時間+GPU時間」の足し算になり、DirectX12本来のCPU/GPU並行実行(パイプライニング)ができていなかった。対策としてバックバッファ数(`FrameBufferCount=2`)ぶんのコマンドアロケータを用意する「フレームインフライト」方式に変更: `EndDraw()`はフェンスに目印(Signal)を立てるだけで待たず、`BeginDraw()`の先頭で「これから使うバッファ枠のGPU処理が完了しているか」をフェンス値で確認してから(通常は2フレーム前の処理なのでほぼ待たずに)`Reset()`する。
- 修正後、実機のDebug HUDで`FPS: 60.0 / Delta Time: 16.667ms`が数秒間安定して表示されることをスクリーンショットで確認済み。

## シェーダーの配布方式(Debug=実行時コンパイル / Release=事前コンパイル)

人に配布する予定があるため、Releaseビルドの配布物にシェーダーのソースコード(`.hlsl`)が残らないようにした。DebugとReleaseで方式を分けている。

- **Debug**: 従来通り`Data\Shader\PMX\*.hlsl`をビルド後に出力先へコピーし、`PMXRenderer::CompileShaderFromFile()`(`D3DCompileFromFile`、`D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION`付き)で起動のたびに実行時コンパイルする。編集して即実行できる開発の身軽さを維持するため。
- **Release**: `PostBuildEvent`で`fxc.exe`を使い、ビルド時に`Vertex.hlsl`/`Pixel.hlsl`をそれぞれ`Vertex.cso`/`Pixel.cso`(コンパイル済みバイナリ、最適化あり・デバッグ情報なしが既定)へ変換して出力先へ直接書き出す。実行時は`PMXRenderer::LoadCompiledShader()`(`D3DReadFileToBlob`)で読むだけ。`Directory.Build.targets`の`Data\Shader\**\*.hlsl`のContentコピーは`'$(Configuration)'=='Debug'`条件を付けてReleaseでは行わないようにし、`.hlsl`ソースが配布物に含まれないようにした。
- 分岐は`PMXRenderer::CreateGraphicsPipelineForPMX()`内の`#if _DEBUG` / `#else`で行っている。PMD(`PMDRenderer`)は現状どのシーンからも生成されない未使用コードのため対象外とした。
- ついでに発覚した不具合: Release構成の`RuntimeLibrary`が`MultiThreadedDebugDLL`(デバッグ版CRT)のままだった上に、`AdditionalLibraryDirectories`がDebug/Release両方のDirectXTex.libパスを同時に含んでいたため、Releaseビルドでも常にDebug版のDirectXTex.libがリンクされていた(配布物としては致命的: VC++デバッグランタイムが無い環境で起動できない)。`RuntimeLibrary`を`MultiThreadedDLL`に、`AdditionalLibraryDirectories`をDebug/Releaseそれぞれ自分の構成のパスのみに修正した。
- 検証: DebugとReleaseそれぞれクリーンビルド後に起動し、Debugは`.hlsl`実行時コンパイル、Releaseは出力先に`.cso`のみ(`.hlsl`/`Header.hlsli`は含まれない)が生成されること、両方とも正常に描画されることをスクリーンショットで確認済み。

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
- [x] インターフェイス整理 → 検討の結果`IUpdatable`/`IDrawable`への分離はしない方針に決定(下記参照)。

### Stage 2(ゲームオブジェクトの骨格)
- [x] オブジェクト基底(`GameObject`) — `SourceCode/00_Game/10_Object/00_Base/`に実装済み。Update/Drawは別インターフェースに分けず`GameObject`自身の仮想関数として直接持たせている。理由: 分離の利点(GameObjectの外側でUpdateだけ実装したいクラスが出てきたときに効く/選択的な適合)は現時点で活かせる箇所が無く、`GameObject`自体は結局Update/Draw両方を無条件に持つため今は分ける実利が無いと判断。GameObject以外でUpdate単体が欲しいクラスが出てきたら改めて検討する。Transform保持、コピー・ムーブはCameraBase同様に禁止(スライシング防止)。まだ`Main`等どこからも生成・使用されていない(骨格のみ)。
- [x] キャラ(`Character`) — `GameObject`を継承する具象クラスとして実装済み。HPは`IHealthSystem`の多重継承ではなく、それを実装した具象クラス`HealthSystem`を`Character`がメンバとして持つ形(コンポジション、`GameObject`が`Transform`をメンバに持つのと同じ形)に変更した。
  - 経緯: 当初`Character : public GameObject, public IHealthSystem`としたが、インターフェース(`IHealthSystem`)はメンバ変数禁止のためHPの実体は結局`Character`側に置く必要があり、HP関連の処理が2ファイルに分裂していた。インターフェースが意味を持つのは型を問わずポリモーフィックに扱いたい場合のみで、今回はその用途が無かったため、HPデータ・`ApplyDamage`・コールバックを全て`HealthSystem`(`IHealthSystem`の唯一の具象実装)に閉じ込め、`Character`はそれをメンバとして持つだけにした。`IHealthSystem`自体は将来ポリモーフィックに扱いたくなった時のために残してある。
  - `HealthSystem`はHP取得・`ApplyDamage()`に加え、`SetOnDamage`/`SetOnDeath`で`std::function`ベースのコールバックを外部から設定できる(死亡は生存→死亡に変化した瞬間のみ1回発火)。
  - Character公開APIをさらに絞り込み: HPは`GetHealth()`(const参照)経由でのみ外部から読める(個別の`GetHP()`/`IsAlive()`フォワーダーは廃止)。`SetOnDamage`/`SetOnDeath`は`protected`化し、派生Character(Player/Enemy/Boss)だけがコールバックを登録できる形にした。`ApplyDamage()`はCharacterの公開APIから撤去し、将来HitEventベースの仕組み経由に変更する予定(`CharacterAccessKeys.h`の`DamageKey`は移行までの間未使用のまま残っている)。
  - Senzan実物調査の結果: SenzanのCharacterはHPをインターフェースに分けず直接メンバに持つ(`IHealthSystem`相当の分離はしていない)。本プロジェクトではStage0の決定(小さいインターフェースの多重継承)を優先し、`IHealthSystem`として分離する方針を維持。
  - Senzanの継承は`Player`/`Boss`が`Character`の直接の兄弟(`Enemy`クラスは存在しない)。本プロジェクトは将来Player/Enemy/Bossの3種に分かれる想定のため、`Enemy`を新設して`GameObject → Character → {Player, Enemy → Boss}`という形にした(BossはEnemyの索敵・敵対AI等を共有できるようにする狙い)。
  - `SourceCode/00_Game/10_Object/{10_Character, 20_Player, 30_Enemy, 40_Boss}/`に骨格のみ実装済み(入力・移動・AI・FSM・当たり判定は全て未実装で、これから)。
- [x] FSM — `StateBase<FSM_Owner>`/`StateMachine<FSM_Owner>`(テンプレート、`SourceCode/99_Utility/StateMachine/`)をSenzanから移植。移植時に以下を修正: `<memory>`の未インクルード(潜在バグ)、`StateBase`に`virtual`デストラクタとコピー・ムーブ禁止を追加(規約4番準拠)、`m_pCurrentState`→`m_spCurrentState`(shared_ptrなのに`m_p`だった命名ミスを修正)、コンストラクタ引数のタイポ(`ownwr`)修正、`#pragma once`の重複除去。単体コンパイル+実行で動作確認済み(状態遷移・Enter/Exit・CanChangeStateによる遷移拒否)。
- [x] Passkey(Attorney-Client)パターン — Senzanの`PlayerAccessKeys.h`は特定のState具体クラスへ丸ごとfriendする代わりに、操作の意味ごとの鍵クラス(コンストラクタprivate、必要なクラスだけfriend)を経由させる仕組み。State実装がまだ無いため、Player固有の鍵(MovementKey等)はそのまま移植できなかった。代わりに現時点で実在する`Character::ApplyDamage()`を実例に`SourceCode/00_Game/10_Object/10_Character/CharacterAccessKeys.h`(`CharacterAccess::DamageKey`、Player/Enemy/Bossのみfriend)として一般化して移植し、パターン自体を確立した。単体コンパイルでfriend側は呼べる/非friend側はC2248でコンパイルエラーになることを確認済み。今後Player/BossのState実装に入るタイミングで、同じ形の鍵クラスを追加していく想定。
- [x] Player移動State(Idle/Run) — `SourceCode/00_Game/10_Object/20_Player/`にSenzanの`01_Player/State/`以下を参考に移植。`StateMachine<Player>`/`StateBase<Player>`(FSMで作った汎用テンプレートそのもの)を実際に使う形にし、`PlayerAccessKeys.h`(`PlayerAccess::MovementKey`、Idle/Runのみfriend)で`Player::SetMoveVec()`をCharacterAccessKeysと同じ形で保護した。
  - Senzanからの意図的な差分: Senzanは`Root`(全ステートを`unique_ptr`で所有し`reference_wrapper`で切り替える独自のFSM実装)を経由しており、せっかく作った`StateMachine<Player>`テンプレートを使っていなかった。本プロジェクトでは`Root`層を廃止し、`Player::ChangeState(eID)`が対象ステートを`make_shared`して`StateMachine<Player>::ChangeState()`に渡す形にした(既存のFSM実装をそのまま活用)。
  - Senzanの`Action`/`Movement`中間基底クラス(コンボ攻撃・回避・当たり判定コンポーネント保持が目的)も未実装のため省略。`PlayerStateBase`にAction::LateUpdate相当の「MoveVecの向きへラープ回転する」処理だけをデフォルト実装として残した。Combat/Dodge/当たり判定を実装するタイミングで必要なら`Action`/`Movement`層を復活させる。
  - `PlayerState::eID`もIdle/Runのみ(System/Combat/Dodge系のIDは未実装のステートを先に生やさないため省略、実装時に追加する).
  - SenzanのMoveVecは`XMFLOAT3`型なのに実際は`.x`/`.y`しか使わない(`.y`が実質ワールドZ)命名の紛らわしさがあったため、本プロジェクトでは`.x`=ワールドX, `.y`=常に0, `.z`=ワールドZという素直な3成分に整理した(`GameObject::AddPosition()`をそのまま渡せる).
  - Run時の移動はアクティブカメラ(`CameraManager::GetActive()`)の`GetForward()`/`GetRight()`をXZ平面へ投影・正規化してVirtualPadの入力と合成するカメラ相対移動。Senzanにあったエフェクト(Effekseer)・アニメーション切り替え(`ChangeAnim`)・当たり判定(`CapsuleCollider`)は未移植の関連システムに依存するため今回は含めていない(それぞれのシステムを作るタイミングで追加).
  - 動作確認用に`MainScene`へ`Player`を1体所有させ(`std::unique_ptr`)、`Update()`で毎フレーム駆動、ImGuiの`"Player"`ウィンドウにPosition/現在ステート名を表示するようにした。ビルド確認済み(`EXITCODE:0`)、起動直後は`Position: (0,0,0)`/`State: Idle`と正しく表示されることをスクリーンショットで確認済み。
- [x] Playerの見た目(`MeshObject`/`PMXMesh`) — SenzanのSkinMesh/MeshBase(`Resource/Mesh/`)を参考に、`GameObject → MeshObject → Character`という中間層を追加した(Senzan実物調査で`Character : public MeshObject`であることを確認した上での移植。多重継承ではなく単一継承の中間層を採用).
  - `PMXMesh`(`SourceCode/10_Ggraphic/PMX/`) — `PMXActor`をラップするファサード。`shared_ptr<PMXActor>`を保持し、`Update`/`Draw`/`SetWorldTransform`/`ApplyAnimationClip`等の最低限の操作だけを`MeshObject`へ見せる(ボーン・GPUバッファ等の詳細はPMXActor内に隠蔽)。`shared_ptr`で持たせているのは、将来「同じモデルデータを複数GameObjectで使い回す」リソース共有の拡張余地を残すため(SenzanのSkinMeshは`weak_ptr<MeshBase>`でMeshObjectから参照される設計だった)。ただしPMXActor自体を「静的な共有データ」と「インスタンス固有の状態(ボーン行列等)」に分割する大規模リファクタリングは今回はやらない(現状1インスタンス=1所有のまま).
  - `PMXActor::SetWorldMatrix()`を追加 — 従来`CreateResources()`で固定のスケール行列を1度書き込むだけで以後更新されなかった(=モデルは常にワールド原点に固定表示だった)不備を修正し、GameObjectの移動を見た目へ反映できるようにした。
  - `MeshObject`(`SourceCode/00_Game/10_Object/05_MeshObject/`) — `shared_ptr<PMXMesh>`を保持し、`Update()`でGameObjectのTransformをメッシュへ反映してから更新、`Draw()`を委譲する。`Character`はこれを継承する形に変更(旧: `Character : public GameObject`).
  - `MainScene`にPlayer用の`PMXMesh`を1つ追加で生成しアタッチ(既存のPMXRenderer/パイプラインを共有)。動作確認用に初期位置を原点から少しずらして配置し、既存のHatuneモデル表示と重ならないようにした。
  - **`IMesh`インターフェースへの一般化(2026-08-14、Codexへ委任)** — `MeshObject`が`PMXMesh`専用(`shared_ptr<PMXMesh>`)だったため、Player/Bossを専用の`.X`モデル(`Data/Model/X/Player/player.X`・`Data/Model/X/Boss/boss.X`、共に埋め込み済みの名前付きアニメーションクリップを持つ)へ切り替えられなかった。`10_Ggraphic/Model/IMesh.h`に`Update/Draw/SetWorldTransform/PlayNamedClip`の4つだけを持つ最小インターフェースを新設し、`PMXMesh`/新設`XMesh`(`10_Ggraphic/X/`、`XActor`をラップするファサード、`PMXMesh`と同じ形)の両方がこれを実装する形にした。`MeshObject::m_pMesh`を`shared_ptr<IMesh>`に変更、`ApplyAnimationClip(Start,End,Speed)`(PMX専用のフレーム範囲指定)を`PlayNamedClip(ClipName)`(名前指定、フォーマット非依存)に置き換えた。
  - **PMX/X間でのアニメーション方式の違いの吸収**: PMXは`AnimationClipTable`(名前→{Start,End,Speed}のテキストテーブル、`AnimationEditor`で編集)経由のフレーム範囲再生、`.X`はファイルに埋め込まれた名前付きクリップをTickベースで再生する、という互換性の無い2方式が既に存在していた。`PlayerStateBase::ApplyNamedClip`が持っていた`AnimationClipTable`検索ロジックを`PMXMesh::PlayNamedClip`側へ移し(Stateからはフォーマットの詳細を隠蔽)、`XMesh::PlayNamedClip`は`XActor::GetClips()`から名前一致するクリップを探し`PlayAnimation()`、見つからなければ新設した`XActor::StopAnimation()`(`m_CurrentClipIndex=-1`、バインドポーズへ戻す)を呼ぶようにした。Playerの`.X`には`player_idle`に相当するクリップが存在しないため、Idle状態は意図的に(存在しない)`"Idle"`を渡し続け、この「見つからない→バインドポーズ」のフォールバックへ委ねている。
  - **クリップ名の対応**: Player本体(`player_run`/`player_attack1〜3`/`player_parry`/`player_perfect_dodge`)、Boss(`boss_idle`/`boss_walk1`/`boss_attack1`/`boss_die`/`boss_take_damage`)ともに`.X`ファイルの`AnimationSet`ブロックを直接grepして実在する名前を確認した上で割り当てた。Bossの`.X`には`attack2/3`・`walk2/3`・`spin_attack*`・`jump_attack*`・`beem*`・`down*`等の未使用バリエーションも存在するが、Boss側の攻撃・移動パターンは現状1種類のみ(下記Boss項参照)のため今回は使っていない(攻撃バリエーション追加は別タスク)。
  - **スケール**: `.X`モデルは単位系がPMXと異なるため、当初は以前の単体検証時と同じ15倍スケール(`Transform::Scale`)を暫定値としてPlayer/Bossへ設定していたが、後述のモデルサイズ検知パネルで実測した結果、Boss側は`Height: 19.07 / Expected: 2.00 (x9.54)`と大幅にズレていることが判明した(15倍という値自体、この2モデル向けに検証されたものではなく単に使い回された値だったため)。`m_DamageCollider`の高さ(2.0、当たり判定そのもの)を基準に逆算し、Player=1.36倍・Boss=1.05倍へ修正、警告が出ないことを実機確認済み.
  - **モデルサイズ検知パネル(`_DEBUG`限定)** — `Character::Draw()`にモデルの実表示高さ(バインドポーズ高さ×Scale.y)と`m_DamageCollider.GetHeight()`の比率チェックを追加し、0.5〜2.0倍の範囲外ならImGuiで赤字警告(`Model Size Warning: class Player`等)を表示する。`PMXActor`/`XActor`にバインドポーズ時のY軸高さ(`GetLocalHeight()`、頂点データから一度だけ計算)を追加し、`IMesh`→`PMXMesh`/`XMesh`→`MeshObject`経由で取得する。あわせて`MainScene`に`"Actor Scale (Debug)"`パネルを追加し、Player/BossのScaleを実行中にImGuiから調整できるようにした(スケール調整はこのパネルで実測しながら行う運用).
- [x] アニメーションクリップの名前管理(`AnimationClipTable`) — `{StartFrame, EndFrame, Speed}`を名前(例: "Idle"/"Run")で引けるテーブル。`SourceCode/10_Ggraphic/PMX/`に配置。シンプルな独自テキスト形式(1行1クリップ、`名前 開始フレーム 終了フレーム 速度`)でファイル保存・読込する。保存先は`Data\Config\AnimationClips.txt`(実行時の作業ディレクトリ基準)で、`imgui.rul`(ImGuiのウィンドウ配置保存)と同様に実行時生成データとして扱う(ProjectDir側の`Data\`とは別物としてOutDir側にのみ存在させる方針。ビルドのxcopyでは上書きされない).
  - `AnimationEditor`にクリップ名の入力欄・Save/Loadボタンを追加。Saveで現在編集中のStart/End/Speedを名前付きでテーブルに登録しファイルへ書き出す。Loadで指定名のクリップ値を編集中のPMXActorへ反映してプレビューできる。
  - `PlayerStateBase::ApplyNamedClip(ClipName)`(protected) — `AnimationClipTable`をファイルから読み込み、該当名のクリップが見つかれば`Player::ApplyAnimationClip()`(`MeshObject`経由でPMXMeshへ)を適用する。`Idle::Enter()`/`Run::Enter()`からそれぞれ`"Idle"`/`"Run"`で呼び出し、AnimationEditorで保存したクリップに基づいてPlayerの再生範囲・速度が切り替わるようにした。

### 独自モデルフォーマット(Static/Skin/Clip) — Action Timeline Editorの前提
Action Timeline Editor本体に進む前に完成させる基盤(詳細は上記「独自ランタイム
モデルフォーマット」節参照)。
- [ ] `.mstc`/`.mskn`/`.mclp`のヘッダー・データレイアウト定義とバイナリ読み書き
      ユーティリティ
- [ ] PMX/X → `.mskn`(+`.mclp`)変換ロジック(既存`PMXParser`/`XParser`を入力
      読み取りに再利用)
- [ ] オンデマンド変換+キャッシュを行うローダー
- [ ] `.mskn`/`.mclp`を実際に描画・アニメーションする新Actor(既存の
      `PMXActor`/`XActor`を置き換えるか、共存させるかは実装しながら判断)
- [ ] `.mstc`用の静的メッシュ描画パス(既存PMXRendererのパイプラインを流用するか
      要検討)
- [ ] Action Timeline Editor本体(この基盤完成後に着手)

### Stage 3(ワールド)
- [ ] ファイル — スコープ要確認(汎用I/Oユーティリティなのか、シーン/マップのファイル形式なのか)
- [ ] マップ
- [x] 当たり判定 — 当初「マップの後」としていたが、実装はGameObject同士の形状ベース判定(Capsule/Sphere)でマップ表現に依存しないと判明したため前倒しで実装した。`SourceCode/00_Game/40_Collision/`にSenzanの`Game/03_Collision/`を参考に移植。
  - `ColliderBase`(`00_Core/`) — 形状の基底クラス。持ち主の`Transform`は`const Transform*`の非所有ポインタで持つ(SenzanはGameObjectが`shared_ptr<Transform>`で持ち、Colliderは`weak_ptr`で参照していたが、本プロジェクトの`GameObject`は`Transform`を値メンバとして持つ設計のため、生ポインタ参照に変更。Colliderは所有者と同じ寿命のメンバとして持たせる想定なので安全).
  - `CapsuleCollider`(`00_Capsule/`)・`SphereCollider`(`10_Sphere/`)・`BoxCollider`(`00_Box/`) — 3形状とも実装。SenzanはBox絡みの`DispatchCollision`が全組み合わせ`return {};`(未実装)だったが、本プロジェクトでは全パターン(Box-Box/Box-Sphere/Box-Capsule)を実装し直した。
  - **Senzan調査で見つかった問題点とその対策**:
    1. **二重ディスパッチが非対称で登録順依存のバグ** — `ColliderBase::CheckCollision`は`other.DispatchCollision(*this)`という二重ディスパッチ(Visitorパターン)で形状ごとの判定へ振り分ける設計だが、Senzanは各形状ペアの片方向にしか実際の判定ロジックが実装されていなかった(例: `CapsuleCollider::DispatchCollision(const SphereCollider&)`は実装済みだが、逆の`SphereCollider::DispatchCollision(const CapsuleCollider&)`は`return {};`のスタブ)。`CollisionDetector`のループは`colliderA->CheckCollision(*colliderB)`を1回しか呼ばないため、どちらが先に登録されたか(=`m_Colliders`内のインデックスの大小)によって同じペアでも判定が抜け落ちる。対策として、狭域判定の実体を`CollisionMath.h`(`00_Core/`)の共通関数(`TestCapsuleVsCapsule`/`TestCapsuleVsSphere`/`TestSphereVsSphere`/`TestBoxVsBox`/`TestBoxVsSphere`/`TestBoxVsCapsule`)に一本化し、形状の優先順(Box→Capsule→Sphere)を決めて必ず同じ関数・同じ引数順で計算するようにした。単体テスト(後述)で全ペア×両方の登録順で正しく検出されることを確認済み。
    2. Senzanは`BoxCollider`絡みの`DispatchCollision`が全て`return {};`(未実装)、`SphereCollider`同士の判定も未実装、`CapsuleCollider`-`BoxCollider`もコメントで「未実装」と明記——今回は全て実装した(下記).
    3. `CollisionDetector::ExecuteCollisionDetection()`内に、`Press`/`BossPress`マスクを判定した後`int i = 0; i++;`しかしない(外側ループの`i`をシャドーイングまでしている)完全に無意味なデバッグコードが残っていた。移植せず削除.
  - **Box判定の実装方針**: このプロジェクトのコライダーは持ち主のYaw(Y軸周り)のみで回転する制約(Pitch/Roll非対応)なので、一般的なOBB用の最大15軸SATではなく、Y軸+お互いのローカルX/Z軸の5軸のみで十分と判断した(残りの軸は水平回転のみという制約下では冗長になるため).
    - `TestBoxVsBox`: 上記5軸SAT。最小重なり量の軸を近似的な押し出し方向として`Normal`/`PenetrationDepth`に採用(接触点は簡易的に両中心の中点).
    - `TestBoxVsSphere`: 球の中心をBoxのローカル軸へ射影しhalfExtentでクランプして最近接点を求める一般的な手法.
    - `TestBoxVsCapsule`: `CapsuleCollider`の中心線分が構造上常にワールドY軸と平行である(`GetSegmentStart/End`がYawでしか回転せず鉛直オフセットが不変)ことを利用し、一般的な線分-OBBのSATを実装する代わりに「高さ(Y)は区間オーバーラップ」「水平面(XZ)は半径付き円とBoxの最近接点距離」に分解して判定. この用途では正確かつ実装がシンプル.
  - `CollisionDetector`(`00_Game/40_Collision/`) — SenzanはSingletonだったが、本プロジェクトの方針(マネージャーはサービスロケーター経由)に合わせServiceLocator経由で利用する形にした。総当たり判定(O(n²))は維持、上記の死んだデバッグコードと未使用の`m_PendingResponses`(外部から参照する手段が無い状態変数)は移植していない。`CompositeCollider`(複数コライダーを1つとして扱うラッパー)も現状使う予定が無いため見送り。総当たりの計算量については、現状のオブジェクト数では最適化不要と判断(将来Boss等で増えたらSweep and Prune等を検討).
  - `eCollisionGroup`はSenzanの`Player_Attack`/`Enemy_Damage`等ゲーム固有の区分をそのまま持ち込まず、`None`/`Default`のみの最小構成にした(Combat関連のステートがまだ無いため、具体的な区分は必要になったタイミングで追加する).
  - 動作確認: スタンドアロンの`cl.exe`単体テストで、Capsule-Sphere/Box-Sphere/Box-Capsuleを両方向の登録順で(登録順依存バグが直っていることを含め)、Box-Box/Capsule-Capsule、範囲外での非衝突、マスクフィルタによる判定除外まで計11ケース全て想定通りの結果になることを確認済み(全プロジェクトビルドも`EXITCODE:0`)。まだ実際のGameObject(Player等)へのアタッチは行っていない(Combat関連のステート実装時に、当たり判定→ダメージへの接続(HitEvent)と合わせて行う想定)。
  - 動作確認: スタンドアロンの`cl.exe`単体テストで、Capsule-vs-Sphereを両方の登録順で検証(登録順依存バグが直っていることを確認)、Capsule-vs-Capsule、範囲外での非衝突、マスクフィルタによる判定除外の5ケース全て想定通りの結果になることを確認済み。まだ実際のGameObject(Player等)へのアタッチは行っていない(Combat関連のステート実装時に、当たり判定→ダメージへの接続(HitEvent)と合わせて行う想定)。
- [x] Combat/Dodgeステート — Senzanを参考に移植(仕様上おかしい点は下記の通り複数見つけて修正)。（※ この作業の前後でユーザーが`00_Game/10_Object/`配下を`10_MeshObject/00_Character/00_Player`のようにSenzanの入れ子構成へ手動で整理したため、以降のPlayer関連パスは新構成が前提）。
  - `HitEvent`ベースのダメージ伝達 — `Character::ProcessHits()`が毎`Update()`後半で`m_DamageCollider.GetCollisionEvents()`をポーリングし、ヒットごとに`HitEvent{AttackAmount, ContactPoint, Normal}`を組み立てて`ApplyDamage()`へ渡す方式にした。Player固有ではなくCharacter層に置いたことでEnemy/Bossでもそのまま使える。旧`CharacterAccess::DamageKey`(Passkeyパターン)はこの方式では不要になったため未使用のまま残している(削除は保留、将来別用途で使うか判断).
  - `eCollisionGroup`を`PlayerAttack`/`PlayerDamage`/`EnemyAttack`/`EnemyDamage`に拡張。`Character`コンストラクタで`m_DamageCollider`/`m_AttackCollider`(共にCapsule)のマスクを設定し、`CollisionDetector`へ登録・デストラクタで解除する。
  - JSON読み込み(`FileManager::JsonLoad`/`JsonSave`、`nlohmann::json`をSenzanから単体ヘッダとしてそのままvendor) — Senzanは`nlohmann::json::parse_error`を握りつぶす(catchせず未処理例外のまま)実装だったため、本プロジェクトでは`try/catch`で捕捉し失敗時は空の`json{}`を返す+`_ASSERT_EXPR`で気付けるようにした(Combatのタイミング調整用JSONを読む前に確認済みの改善).
  - `SoundManager`(XAudio2) — SenzanはDirectSound+mmioベースだったが、調査の過程で複数のバグ(ピッチ変更が`DSBCAPS_CTRLFREQUENCY`未設定で無効化されたまま気づかれない、拡張子判定が大文字小文字を区別してしまう、エラーを握りつぶす、`Stop()`が多重再生用の複製バッファまで止めない)が見つかったため、移植ではなくXAudio2で新規に書き直した。`ServiceLocator`経由、`Main`が`unique_ptr`で所有。SE再生のみ(BGMクロスフェード等は未着手).
  - `CameraBase::Shake(Intensity, Duration)` — カメラ種別ごとに実装せず基底に1つ追加する方針にした(検討の結果`CameraBase`側に追加が妥当と判断)。`ViewUpdate()`内で減衰する乱数オフセットを視点位置へ加算する。
  - コンボ/必殺ゲージ経済 — `Player`に`m_Combo`/`m_CurrentUltValue`/`m_MaxUltValue`を追加、`ComboMultiplier()`(コンボ数に応じたダメージ倍率)を提供。操作用セッターは`PlayerAccess::ComboEconomyKey`(Passkey)経由に限定し、`AttackCombo_0/1/2`/`Parry`のみfriendにした(既存のPasskey運用を踏襲).
  - `Combat`基底クラス(`State/20_Combat/`) — `ColliderWindow`(開始時刻+持続時間)のリストをJSON(`Data\Json\Player\AttackCombo\*.json`)から読み込み、`Update()`内で現在時刻がウィンドウ内に入ったら攻撃コライダーを有効化する方式。コンボ受付は`ComboStartTime`〜`ComboEndTime`の間の攻撃入力を`UpdateComboInput()`で共通化(3つのAttackComboステートで重複させない). Senzanにあったボス追尾(`GetTargetPos()`ベースの接近・向き直し)はBossが未実装のため意図的に省略し、その場で攻撃する形にした(コメントで明記).
  - `AttackCombo_0/1/2` — `AttackCombo_2`から`AttackCombo_0`へループする3段コンボ。ダメージ量は25/30/40。専用アニメクリップ("AttackCombo_0"等)を`ApplyNamedClip()`で適用。`AttackCombo_2`のJSONのみ、Senzanの`ColliderWindows`持続時間(0.001)をそのまま使うと判定が不安定になりかねないと判断し0.05へ変更(意図的な数値の差分).
  - `Parry` — Senzanは`Enter()`内で基底`Combat::Enter()`の呼び出しを忘れている(コンボウィンドウ等が初期化されないバグ)ことを発見・修正して移植。構え中は`SetDamageColliderActive(false)`で無敵化、`PARRY_MAX_WAIT_TIME`経過でIdleへ自動遷移。成功/失敗判定(Just Parry)は対象のBossが無いため未実装(あと回し).
  - `Dodge`基底 + `DodgeExecute` — Senzanは「`CollisionDetector`への登録解除」と「コライダーの`SetActive(false)`」という2種類の無敵化手段が混在していた(Parryは後者)ため、本プロジェクトでは`SetDamageColliderActive()`に統一した。移動方向はカメラ相対(入力なしなら現在の正面)、`DodgeExecute::LateUpdate()`は`MyEasing::UpdateEasing`で`InOutCubic`と`Liner`を50:50でブレンドした距離を毎フレーム差分計算して加算する(加減速のある回避移動)。専用のアニメクリップ名("DodgeExecute")を使う点は、Senzanが`Attack_0`のクリップを回避モーションとして流用していた(プレースホルダーの残骸と思われる)のを踏襲しない意図的な修正.
  - 効果音・エフェクトへのフック — `Character`に`PlayEffect*`系の空関数(中身は未実装)を追加済み。Effekseer採用か自作パーティクルかは未決定のため後回し.
  - 未着手・意図的にあと回し: `JustDodge`(回避成功判定)、`AfterImage`(残像演出)、`JustDodgeEffect`、`PostEffectManager`、`CombatCoordinator`(Senzanの実装は時間が無く作った急ごしらえの設計だったため、移植時に設計をやり直す予定)、Boss/Enemyの実体(現状は骨格のみ)。
- [x] Enemy(最小限のAI) — Combat/Dodgeを実際に試せる相手として実装。Senzan調査の結果、Senzanには汎用の`Enemy`クラスが存在せず(`Boss`が`Character`を直接継承しており、AI・当たり判定・ダメージ処理などボス戦専用ロジックが約5,700行に渡って詰め込まれている)、移植元と呼べるものが無かったため、既存の`Character`/`HitEvent`/`ProcessHits()`基盤の上に最小構成で新規設計した.
  - `StateMachine<Enemy>` + `EnemyState::{Idle, Chase, Attack, Dead}`(`10_Enemy/State/`) — Player同様`EnemyStateBase : public StateBase<Enemy>`を経由する。`DistanceToTargetXZ()`/`AngleToTargetDeg()`をEnemyStateBaseに共通実装として持たせ、各ステートから使う.
    - `Idle`: ターゲットが`AggroRange`以内に入るまで待機(Senzanの`BossIdolState`は最初のUpdateで無条件にMoveStateへ遷移する=実質待機しないバグがあったため、そこは踏襲せずちゃんと距離判定してから遷移するようにした).
    - `Chase`: ターゲットへ向き直りながら直進。`AttackRange`以内で`Attack`へ、`LoseRange`を超えたら`Idle`へ戻る(Senzanのボスにはこの「見失う」概念が無かったが、雑魚敵がマップ全体をどこまでも追い続けるのは不自然なので追加した).
    - `Attack`: 予備動作→攻撃判定(`Character::SetAttackColliderActive`)有効化→硬直の3フェーズをタイマーで管理する固定1パターン攻撃。SenzanのBossMoveStateにあった8種の重み付きランダム攻撃選択・JSON外部調整・ImGuiチューニングパネルは意図的に持ち込んでいない(最小限のEnemy用途には過剰).
    - `Dead`: `HealthSystem::SetOnDeath`のコールバックから遷移。攻撃/被弾コライダーを無効化してその場に残り続ける(SenzanのBossDeadStateは`Update()`が空で死亡演出も何も進行しない未完成スタブだったため、それをそのまま踏襲するのは避け、最低限「もう攻撃されない/攻撃しない」状態には確実に落とし込んだ。ただし消滅・リスポーン等はまだ無い).
  - ダメージ経路はSenzanのBossのような専用実装(`HandleDamageDetection`/`HandleAttackDetection`を各クラスで個別実装、かつ`Boss::Hit()`という`ApplyDamage`を経由しない別経路が並存し、ダメージ二重適用やHPクランプ漏れが起きうるバグがあった)を踏襲せず、既存の`Character::ProcessHits()`(HitEvent方式)をそのまま利用。Enemyは`eCollisionGroup::EnemyDamage`/`EnemyAttack`(対`PlayerAttack`/`PlayerDamage`)のマスク設定を追加しただけで、専用のダメージ処理コードは1行も書いていない.
  - ターゲット追跡は`Enemy::GetTargetPos()`/`SetTargetPos()`のみ(Senzanの`Player`⇔`Boss`相互`SetTargetPos()`と同じく、ロックオン等の探索機構は無く、シーン側が毎フレーム位置を渡すだけの単純な仕組み)。`MainScene::Update()`から`m_upEnemy->SetTargetPos(m_upPlayer->GetPosition())`を毎フレーム呼ぶ形で配線した。Player側の`GetTargetPos()`(Enemyの位置をPlayerへ渡す経路)はCombat側がまだ`GetTargetPos()`ベースの接近・向き直りを使っていない(Stage3当たり判定の節で明記した通り意図的に未移植)ため、今回は追加していない。Boss実装時にPlayer側のCombatを拡張するタイミングで必要になる想定.
  - `GameObject::RotateToTarget()` — 元々`Player`だけに実装していたYawラープ回転処理を、Enemyでも同じロジックが必要になったタイミングで`GameObject`へ引き上げた(2箇所目の利用が実際に出てきたので、コピペではなく共通化した).
  - 見た目は専用モデルを用意せず、動作確認用にPlayerと同じHatuneモデルを別インスタンスとして流用(位置で区別)。Cube.pmx等の別アセットはこのプロジェクトのPMXパイプラインで未検証のため、リスクを避けてPlayerで実績のあるモデルを使った.
  - `MainScene`にImGuiの`"Enemy"`ウィンドウを追加(Position/State/HP表示、Player同様のデバッグ表示).
  - ビルド設定の不備を発見・修正: `DirectX12.vcxproj`が`ClCompile`の`ObjectFileName`を既定値のままにしていたため、フォルダを跨いで同名の`.cpp`(Player側`State/00_Idle/Idle.cpp`とEnemy側`State/00_Idle/Idle.cpp`)が同じ中間出力ファイル名(`Idle.obj`)に衝突し、後から生成された方が前者を上書きしてリンクエラーになる問題を発見した(MSB8027警告が出ていたにもかかわらずSenzanはおそらく同名ファイルが無く顕在化していなかった類の不備)。全4構成(Debug/Release × Win32/x64)の`ClCompile`に`<ObjectFileName>$(IntDir)%(RelativeDir)\</ObjectFileName>`を追加し、中間出力をソースの相対フォルダ構成にミラーリングすることで解消した.
- [x] Boss(最小構成) — Enemyを土台にBoss本体を実装(専用攻撃パターンは未実装、あと回し).
  - `Boss : public Enemy`だが、`Enemy::m_StateMachine`(`StateMachine<Enemy>`、private)はBossから触れず再利用できないため、Boss専用に`StateMachine<Boss>` + `BossState::{Idle, Move, Attack, Dead}`(`20_Boss/State/`)を別途持たせた(`BossStateBase : public StateBase<Boss>`、EnemyStateBaseと同じ形で`DistanceToTargetXZ`/`AngleToTargetDeg`を共通実装)。Enemy自身の`m_StateMachine`はBoss構築時にIdleへ初期化されたまま以後一切Updateされない未使用状態で残る(Enemyのコンストラクタが共有ロジックのため許容している).
  - `Boss::Update()`は`Enemy::Update()`を経由せず`Character::Update()`を直接呼ぶ(Enemy側の未使用StateMachineを動かさないため)。`Boss::ChangeState(BossState::eID)`は同名の`Enemy::ChangeState(EnemyState::eID)`を意図的に名前で隠蔽する(Bossに対して誤ってEnemy側のステート変更を呼べないようにするため).
  - Enemyの索敵AI調整値(MoveSpeed/AggroRange/AttackRange/LoseRange)は元々private固定値だったため、`Enemy(float,float,float,float)`という保護コンストラクタを追加してBossから上書きできるようにした(Enemy自身のコメントが「将来種類ごとに変えたくなったらコンストラクタ引数化する」と予告していた通りの対応)。Bossは仮値(MoveSpeed=3.0/AggroRange=15.0/AttackRange=3.5/LoseRange=30.0、Enemyの4.0/10.0/2.5/20.0より広め・強め)を渡している.
  - `SetOnDeath`はEnemyのコンストラクタが一度`ChangeState(EnemyState::eID::Dead)`を登録するが、Boss自身の`StateMachine<Boss>`を正しく死亡させるため、Bossのコンストラクタで`ChangeState(BossState::eID::Dead)`を呼ぶコールバックに登録し直している.
  - HP(`Character::m_Health`、既定100固定)は今回変更していない。HP調整もEnemyのAI調整値と同様にコンストラクタ引数化が必要で、あと回し.
  - 攻撃は1パターンのみ(予備動作0.6s→判定0.3s→硬直0.8s、威力25。EnemyのAttackより大振り・高威力だが仮値)。SenzanのBossMoveStateにあった8種の重み付き攻撃選択・JSON調整・ImGuiパネルは今回持ち込んでいない.
  - パリィ演出用に`KeyframeCamera`(`00_Game/30_Camera/50_Keyframe/`)を新設。`CameraKeyframe{Position, Look, FovY, Duration, Easing}`の列を`Easing.inl`で補間再生し、`CameraManager::PlayOneShot(Name, Keyframes, IsRelativeToFirst)`で一時的に切り替えると、再生終了時に自動で元のアクティブカメラへ戻る(呼び出し元はタイマーを持たなくてよい)。`IsRelativeToFirst`は先頭キーフレームを基準に以降を相対座標として指定できるオプション。実際の発火箇所は`CombatCoordinator::OnParrySuccess()`(後述).
  - `MainScene`への実体配線は、`MainScene`自体の整理が完了した後にPlayer/Boss/CombatCoordinatorとまとめて行った(下記参照).
- [x] CombatCoordinator(設計やり直し版) — SOLIDのS(単一責任)/O(開放閉鎖)を中心に設計し、後から操作権限を限定する`PlayerCombatView`/`BossCombatView`も追加した。Player/Bossの所有権は抽象化せず、`MainScene`の`unique_ptr`に一本化したままにしている.
  - **限定公開View**: Player/Bossへインターフェースを多重継承させず、非所有ポインタを内包する`PlayerCombatView`/`BossCombatView`をCoordinatorへ値渡しする。各Viewが公開するのは現在位置の取得とパリィリアクション開始だけで、Coordinatorから他のpublic操作へアクセスできない。リアクション開始はPlayer/Bossのprivateメソッドとし、各Viewだけをfriendにしている。Coordinatorは未初期化状態を表す`std::optional<View>`を保持し、シーン破棄時は`MainScene`のデストラクタから`Clear()`を呼んで非所有参照を破棄前に解除する.
  - **責務分離**: `CombatCoordinator`(`00_Game/60_Combat/`)は「いつ・どんな演出データにするか」の計算とトリガーだけを担当し、実際にTransformへ書き込むのは各アクター自身のステートに一任する。同じフレームに2箇所からTransformが書き換えられる競合(CombatCoordinatorと、そのアクター自身の現在ステートの両方が同時に位置を操作してしまう)を避けるための分離であり、SenzanのCombatCoordinator(計算と書き込みを両方自分でやっていた)から意図的に変えた点.
  - Boss側は`Boss::EnterParryReaction(TargetPosition, TargetYawDeg, Duration)`という専用エントリ経由で`BossState::ParryReaction`(`20_Boss/State/40_ParryReaction/`)へ入る(汎用の`ChangeState(BossState::eID)`は追加データを渡せないため、あえて経由しない)。位置は`MyEasing`で補間、向きは既存の`RotateToTarget`(最短経路ラープ)を流用し、硬直中は攻撃判定を無効化する。終わったらMove(見失っていればIdle)へ戻る.
  - Player側は新しいStateクラスを作らず、既存の`PlayerState::Parry`自身に「`Player::HasParryReactionTarget()`が立っていれば目標位置・向きへ遷移する」処理を追加した。パリィ成立は定義上Playerが既に`Parry`ステートに入っている時にしか起こり得ないため、リアクションの責任も既にそのステートが持つべきと判断し、Boss側のような新規ステートは作らなかった(この非対称性は意図的).
  - データの受け渡しは、Boss同様の専用エントリではなく`Player::SetParryReactionTarget(...)`という設定メソッド+Playerメンバのフラグ経由にした(Playerの現在ステートがParryか外部から確実に判別する手段が無いため。Parry::Update()が毎フレームフラグを確認し、無ければ何もしない)。CombatCoordinatorだけが書き込めるよう、既存の`PlayerAccessKeys.h`(Passkeyパターン)に`CombatCoordinatorKey`を追加して鍵越しにした.
  - `CameraManager`と同じくMain.cppで構築しServiceLocatorへ登録(`Main::Release()`は他の解放処理より前に`Clear()`してから登録解除する)。`Initialize(player, boss)`は`MainScene::Create()`から呼ばれている(下記「MainSceneへの実体配線」参照).
  - **未実装だった箇所を後日実装**: パリィの成立判定自体。`eCollisionGroup`に`PlayerParry`を追加し、`Enemy`(Bossも含む)の攻撃コライダーの対象マスクへ`PlayerDamage | PlayerParry`を設定(Parry中は`PlayerDamage`が無効化されているため、これが無いとパリィ中の攻撃が誰にも衝突しなくなる)。Playerには`m_DamageCollider`とは別に専用の`m_ParryCollider`(`PlayerParry`マスク、通常時は無効)を追加し、`PlayerState::Parry::Enter/Exit`で有効/無効を切り替える。`Parry::Update()`が毎フレーム`m_ParryCollider`の衝突結果を確認し、ヒットしていれば`CombatCoordinator::OnParrySuccess()`を呼ぶ(これで「配線待ち」だった`OnParrySuccess()`が実際に呼ばれるようになった). Senzanの`Player_Parry_Suc/Fai/Noc`(成功/失敗/無効の3分岐)は今回持ち込まず、「当たれば成功」の1段階のみにした(失敗判定はまだ無い. あと回し).
  - **パリィ演出カメラの発火** — `CombatCoordinator::OnParrySuccess()`の末尾で`CameraManager::PlayOneShot("ParryReaction", Keyframes)`を呼び、`KeyframeCamera`(前述)を発火させるようにした。Boss側の`BossState::ParryReaction`から呼ばない設計にしたのは、演出カメラもBoss/Playerの位置反応と同じく「いつ・どんな演出データにするか」を決める仕事であり、CombatCoordinatorの既存の責務(演出の計算とトリガーだけを担当)にそのまま収まるため。構図はBoss-Player中点を挟んだ側面(Boss→Player方向に垂直なオフセット)からの寄りショット2キーフレーム(広め→`OutCubic`で寄る、`PARRY_REACTION_DURATION`と別に`PARRY_CAMERA_DURATION`(0.6秒)で管理)。Boss/Playerの位置反応と同時に走るが、両者は独立した状態(カメラは`CameraManager`、位置は各アクター自身のState)なので競合しない.
  - **`MainScene`への実体配線(完了)** — `MainScene::Create()`で`Player`/`Boss`をそれぞれ`std::make_unique`し、共通の初音ミクPMXモデル(`Data/Model/PMX/Hatune/`)を独立した`PMXMesh`インスタンスとしてそれぞれにアタッチ(ボーン/アニメーション状態を共有しないため)、Bossを`Player`から8ユニット離れた位置(AggroRange内・AttackRange外)に配置。`ServiceLocator::Get<CombatCoordinator>()`経由で`Initialize(PlayerCombatView{*m_upPlayer}, BossCombatView{*m_upBoss})`を呼ぶ。`Update()`/`Draw()`双方から両アクターを毎フレーム呼び出す。`MainScene`のデストラクタは`CombatCoordinator::Clear()`を呼んでから両アクターを破棄する(非所有Viewが破棄済みポインタを指したままにならないようにするため).

### Stage 4(UI・演出、最も後)
- [ ] 色 — Color構造体/変換ユーティリティ
- [ ] UIの設計方針決定 → UI実装
- [ ] カットシーンエディター — ツール的性質が強く、前提が揃ってから

### Scene基盤(前倒しで実装済み)
- [x] `SceneBase`/`SceneManager` — Senzanの`SceneBase`/`SceneManager`(シングルトン)を移植。このプロジェクトの方針(マネージャーはサービスロケーター経由)に合わせ、`SceneManager`はSingletonではなく`Main`が所有し`ServiceLocator`へ登録する形にした。`SceneBase`は`Initialize()`/`Create()`/`Update()`/`LateUpdate()`/`Draw()`が純粋仮想(継承前提のためコピー・ムーブ禁止)。
  - シーン切り替え(`LoadScene()`)は即時ではなく予約制。シーン自身の`Update()`の中から`LoadScene()`を呼んでも、実際の切り替え(`m_upScene.reset()`)は次の`SceneManager::Update()`の先頭で行われるため、シーンが自分自身のUpdate実行中に自分自身を破棄する事故を防いでいる(Senzanはフェード完了待ちで同じ問題を回避していたが、このプロジェクトにまだFadeManagerが無いため単純な1フレーム遅延にした)。
  - `SourceCode/99_System/Scene/`に配置(GameObject/Character等と違いゲーム内容に依存しないため`00_Game`ではなく`99_System`)。
- [x] `MainScene` — 元々Main.cppが直接持っていたPMXモデル表示部分(カメラ登録・PMXActor生成・Update・Draw)をシーンとして抽出.
- [x] `AnimationTuningScene`(デバッグ専用) — Senzanの同名シーンを参考に新設。`F1`でMainScene⇔AnimationTuningを切り替え可能。デバッグ時は`SceneManager`のImGuiウィンドウ(現在のシーン名表示+切り替えボタン)からも切り替えられる。
- [x] `ModelPreviewPanel`(`SourceCode/99_Utility/Debug/Imgui/`) — `Data\Model\PMX`・`Data\Model\X`配下で見つかった全モデルをドロップダウンで切り替えながら`AnimationEditor`で再生・調整できるデバッグパネル。元々`AnimationTuningScene`専用だった実装(モデル走査・切り替え・PMXActor/XActor所有)をシーンに依存しない部品として切り出した。
  - 切り出した理由: 「モデル確認のためだけにシーンを切り替える(=MainSceneを一旦破棄して作り直す)」のはUnityのシーンビュー/ゲームビューの感覚と違い不便、という指摘を受けた。`MainScene`にも同じ`ModelPreviewPanel`を常駐させることで、ゲーム本体(Player/Enemy等)を止めずにいつでもモデルプレビュー・アニメーション確認ができるようにした。`AnimationTuningScene`は現在ではこの`ModelPreviewPanel`を専用カメラ付きで表示するだけの薄いラッパーになっている(重複コードを避けるため、両シーンから同じクラスを利用する形にした)。
  - 落とし穴: PMX選択時は既定で一時停止状態(Stepボタンでのみ進む)のため、`LoadModel()`内で最初の`StepFrame()`を1回呼ばないとボーン変換が一度も計算されずモデルが非表示になる(実際に発生し修正済み).
- [x] ImGuiウィンドウの初期位置固定 — `imgui.rul`(実行時生成のレイアウト保存ファイル)が無い状態(初回起動・削除後)だと全てのウィンドウが既定位置(60,60)に重なって表示され邪魔だったため、`Debug HUD`・`Scene`・`Model Select`・`Animation Editor`・`XActor Animation`の各ウィンドウに`ImGui::SetNextWindowPos(..., ImGuiCond_FirstUseEver)`で重ならない初期位置を指定した。`FirstUseEver`のため、一度`imgui.rul`に保存されればユーザーが動かした位置がそれ以降優先される(強制的に固定するわけではない).

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

## 独自ランタイムモデルViewer初期実装

`Tools/ModelViewerExtension/`に、VSIX SDKやHelixToolkitがまだ導入されていない環境でも変換後データを確認できるWPFホストを追加した。`RuntimeFormatReader`はC++の`RuntimeFormatIO`と同じリトルエンディアンの固定レイアウトを読み、MSKN v4の件数・ペイロード範囲・親順序・インデックス・スキンスロット・サブメッシュ役割と、MMAT v2/v3の値・固定長パスを検証してから実行時データへ変換する。

初期ViewerはバインドポーズのCPUスキニングを`Viewport3D`へ表示し、サブメッシュ単位のテクスチャ・Diffuse・Specular・Ambient・トゥーン・スフィア情報と、FrontCompositeの設定値を表示する。VSIXの`ToolWindow`への接続、HelixToolkit依存、MCLP再生、ボーン表示、特殊合成はSDK骨格が配置された後の次段階とする。
