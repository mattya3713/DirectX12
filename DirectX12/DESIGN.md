# 設計方針・進捗メモ

コード規約は`README.md`を参照。ここには設計の考え方と、今の到達点・未着手項目をまとめる。

## モデルデータの共通化

PMX/PMDそれぞれのバイナリ形式を読むパーサーと、ゲームが実際に使うデータを分離している。

- `SourceCode/10_Ggraphic/Model/ModelData.h` — フォーマットを問わない共通データ(`Model::Vertex` / `Model::Material` / `Model::Bone` / `Model::ModelData`)。頂点レイアウトはPMXの4ボーン形式を基準にしており、PMDの2ボーンデータもここに変換して格納する。
- `SourceCode/10_Ggraphic/Model/IModelParser.h` — `Load(FilePath, ModelData&)`を持つパーサーの共通インターフェース。新しいモデルフォーマットに対応するときは、これを実装したパーサーを追加すればよい。
- `SourceCode/10_Ggraphic/Model/PMXParser.h/.cpp`、`PMDParser.h/.cpp` — 各フォーマットの生バイナリ解析だけを担当。GPUリソース生成やアニメーション実行時ロジックは`PMXActor`/`PMDActor`側に残している。

## カメラシステム

`SourceCode/00_Game/30_Camera/`配下。`CameraBase`(抽象基底)を継承する形で用途別に分割している。

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
