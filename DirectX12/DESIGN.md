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
- [x] アニメーションクリップの名前管理(`AnimationClipTable`) — `{StartFrame, EndFrame, Speed}`を名前(例: "Idle"/"Run")で引けるテーブル。`SourceCode/10_Ggraphic/PMX/`に配置。シンプルな独自テキスト形式(1行1クリップ、`名前 開始フレーム 終了フレーム 速度`)でファイル保存・読込する。保存先は`Data\Config\AnimationClips.txt`(実行時の作業ディレクトリ基準)で、`imgui.rul`(ImGuiのウィンドウ配置保存)と同様に実行時生成データとして扱う(ProjectDir側の`Data\`とは別物としてOutDir側にのみ存在させる方針。ビルドのxcopyでは上書きされない).
  - `AnimationEditor`にクリップ名の入力欄・Save/Loadボタンを追加。Saveで現在編集中のStart/End/Speedを名前付きでテーブルに登録しファイルへ書き出す。Loadで指定名のクリップ値を編集中のPMXActorへ反映してプレビューできる。
  - `PlayerStateBase::ApplyNamedClip(ClipName)`(protected) — `AnimationClipTable`をファイルから読み込み、該当名のクリップが見つかれば`Player::ApplyAnimationClip()`(`MeshObject`経由でPMXMeshへ)を適用する。`Idle::Enter()`/`Run::Enter()`からそれぞれ`"Idle"`/`"Run"`で呼び出し、AnimationEditorで保存したクリップに基づいてPlayerの再生範囲・速度が切り替わるようにした。

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

### Stage 4(UI・演出、最も後)
- [ ] 色 — Color構造体/変換ユーティリティ
- [ ] UIの設計方針決定 → UI実装
- [ ] カットシーンエディター — ツール的性質が強く、前提が揃ってから

### Scene基盤(前倒しで実装済み)
- [x] `SceneBase`/`SceneManager` — Senzanの`SceneBase`/`SceneManager`(シングルトン)を移植。このプロジェクトの方針(マネージャーはサービスロケーター経由)に合わせ、`SceneManager`はSingletonではなく`Main`が所有し`ServiceLocator`へ登録する形にした。`SceneBase`は`Initialize()`/`Create()`/`Update()`/`LateUpdate()`/`Draw()`が純粋仮想(継承前提のためコピー・ムーブ禁止)。
  - シーン切り替え(`LoadScene()`)は即時ではなく予約制。シーン自身の`Update()`の中から`LoadScene()`を呼んでも、実際の切り替え(`m_upScene.reset()`)は次の`SceneManager::Update()`の先頭で行われるため、シーンが自分自身のUpdate実行中に自分自身を破棄する事故を防いでいる(Senzanはフェード完了待ちで同じ問題を回避していたが、このプロジェクトにまだFadeManagerが無いため単純な1フレーム遅延にした)。
  - `SourceCode/99_System/Scene/`に配置(GameObject/Character等と違いゲーム内容に依存しないため`00_Game`ではなく`99_System`)。
- [x] `MainScene` — 元々Main.cppが直接持っていたPMXモデル表示部分(カメラ登録・PMXActor生成・Update・Draw)をシーンとして抽出.
- [x] `AnimationTuningScene`(デバッグ専用) — Senzanの同名シーンを参考に新設。専用のPMXActor+カメラを持ち、`AnimationEditor`を常時表示する。`F1`でMainScene⇔AnimationTuningを切り替え可能。デバッグ時は`SceneManager`のImGuiウィンドウ(現在のシーン名表示+切り替えボタン)からも切り替えられる。
- [x] `AnimationEditor`(`SourceCode/99_Utility/Debug/Imgui/`) — アニメーションの再生範囲(開始/終了フレーム)・再生速度を調整し、1フレームずつステップ実行できるImGuiツール。`PMXActor`に`SetPlaybackRange()`/`SetAnimationSpeed()`/`StepFrame()`(壁時計に依存しない単一フレーム前進)を追加して対応。
  - 落とし穴: `AnimationTuningScene`は既定で一時停止状態(Stepボタンでのみ進む)のため、`Create()`で最初の`StepFrame()`を1回呼ばないとボーン変換が一度も計算されずモデルが非表示になる(実際に発生し修正済み).

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
