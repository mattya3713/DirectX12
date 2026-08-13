# Token Log

Claude(Lead)とCodex(Implementation Engineer)、どちらに実装を任せた方がトークンを節約できるかを比較するための記録。

## 計測方法・制約

- **Codex側**: `codex exec`が実行終了時に自己申告する`tokens used`をそのまま転記する(OpenAI API側の実測値、正確)。
- **Claude側**: Claude Code(このセッション)からは自分自身の正確なトークン消費量を読み取る手段が無い。代わりに「読んだファイル数・行数」「Bash/Edit等のツール呼び出し回数」をproxy指標として記録する(目安値、タスク間の相対比較用)。正確な実測値が欲しい場合はユーザーが自分のターミナルで`/cost`を実行して確認する。
- **公平な比較にならないケースに注意**: 同じタスクでも、Claudeが既に該当ファイルを読んで文脈を持っている状態で書く場合と、Codexがゼロから調査して書く場合とではコストの意味が違う。「ゼロから調査が必要なタスクをどちらに投げたか」を条件欄に明記する。

## 記録

| 日付 | タスク | 実行者 | 条件(既存文脈の有無) | トークン(実測/目安) | 備考 |
|---|---|---|---|---|---|
| 2026-08-14 | サンドボックス動作確認: `codex_write_test3.txt`作成(danger-full-access, apply-patch方式) | Codex (gpt-5.6-sol) | ゼロから(単発の単純な書き込み指示のみ) | 4,384 (実測) | 成功。apply patchベースでOS ACL機構を経由しないため実際に書き込まれた |
| 2026-08-14 | サンドボックス動作確認: `codex_write_test.txt`作成(workspace-write, PowerShell Set-Content経由) | Codex (gpt-5.6-luna) | ゼロから(単発の単純な書き込み指示のみ) | 19,697 (実測) | "phantom success"バグ再現(Codexは成功と報告したが実際は exit code 1 で失敗・未作成) |
| 2026-08-14 | `CombatCoordinator::OnParrySuccess()`にパリィ演出カメラ(`KeyframeCamera`)呼び出しを追加配線 | Claude (直接実装) | 既存文脈あり(同じ調査の流れで`CombatCoordinator.cpp`/`KeyframeCamera.h`/`CameraManager.h`を既読) | 目安: 追加で読んだファイル3件(合計約100行)、Edit呼び出し2回。既読分の再読み込みコストは無し | 既に文脈を持っていたため、Codexに投げていたら同じファイル群をゼロから読み直す形になり非効率だったと判断し直接実装した |

| 2026-08-14 | `.X`モデル差し替え一式: `IMesh`インターフェース新設、`PMXMesh`/`XMesh`両対応、Player/Boss全State(7+5箇所)のクリップ名差し替え、`MainScene`のモデル切り替え、vcxproj登録 | Codex (gpt-5.6-luna, danger-full-access) | ゼロから(Claudeは調査・設計・タスク仕様書作成のみ行い、実装はCodexに完全委任) | 114,515 (実測) | 成功。0エラー0警告でビルド。範囲外のBOM自動修正が1ファイル対で発生(害はない)。設計(IMesh/XMesh形状・クリップ名対応表)はClaudeが事前に確定し仕様書に明記していたため、Codexの実装判断の余地はほぼ無かった |

| 2026-08-14 | モデルサイズ検知(`_DEBUG`限定): `IMesh`/`PMXMesh`/`XMesh`/`MeshObject`へ`GetLocalHeight()`配線、`Character::Draw()`でコライダーサイズと比較しImGui警告 | Codex (gpt-5.6-luna, danger-full-access) | ゼロから(Claudeは`PMXActor`/`XActor`側の下準備のみ直接実装し、残りの配線はタスク仕様書のみ渡して委任) | 56,237 (実測) | Claude側の実装漏れ(XActor.hに`m_LocalHeight`本体を追加し忘れ)が原因でビルドが4エラーで失敗。CodexはXActor変更禁止のスコープを正しく守り、原因を`review_points`で正確に報告して停止した(スコープ逸脱で無理に直さなかった判断は正しい)。Claudeが自分のミスを2ファイルだけ直接修正して解決 |

| 2026-08-14 | Player/BossのScaleを実行中にImGuiで調整できるデバッグパネルをMainSceneへ追加 | Codex (gpt-5.6-luna, danger-full-access) | ゼロから(1ファイルのみの小タスク、仕様書を渡して完全委任) | 45,728 (実測) | 成功。既存の無関係なシェーダー警告(Vertex.hlsl)以外は0エラー0警告。作業前からあったPlayer Scale=1.0fのテスト値も指示通りスコープ外として触らず維持した |

| 2026-08-14 | ログコンソール機能: `DebugLog`(ServiceLocator登録)+`DebugConsole`(ImGuiスクロール表示、色分け)を新設、Senzanの`Log`シングルトンをServiceLocator版として移植 | Codex (gpt-5.6-luna, danger-full-access) | ゼロから(Senzanの参照ファイルパスのみ提示、実装は完全委任) | 112,334 (実測) | 成功。副産物として`.gitignore`の`[Ll]og/`パターンが新設フォルダを誤って除外することを`review_points`で正確に報告(自分では.gitignoreを直さず、正しくスコープ外として報告のみ)。Claudeが除外例外を追加して解決 |

## 傾向メモ

- Codexの`tokens used`は「そのタスクを実行するために必要だった調査+生成」の総量。単純な単発コマンド(ファイル1つ作成)でも約4,000〜20,000トークンかかっており、タスクの複雑さより「ゼロから何を読んだか」に強く左右される。
- Claudeが既に文脈を持っている状態でCodexに委任すると、Codexが同じ調査をやり直すため二重コストになりやすい。**委任の効果が一番出るのは、Claudeがまだ何も調べていない・大きめの実装タスクを最初から丸投げする場合**。
- 実例(`.X`モデル差し替え、114,515トークン): Claude側は設計(`IMesh`のメソッド構成、クリップ名対応表の確定)と`tasks/current.md`の作成に事前調査のトークンを使ったが、実際のコード編集(16ファイル、新規3+既存13)・vcxproj登録・ビルド確認・エラー時の自己修正は全てCodex側のこの114,515トークンに含まれる。もしClaudeが直接この分量を編集していたら、同程度かそれ以上のファイル読み込み+Edit呼び出しが発生していたはずで、体感的には「大きな実装ほど委任の効果が出る」という上記の仮説と整合する結果だった。
