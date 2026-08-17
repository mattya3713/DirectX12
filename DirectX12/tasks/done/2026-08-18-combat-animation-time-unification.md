# Task (完了): Combat/アニメーション時間軸の統合(独自時計の廃止)

## 発端

Action Timeline Editorの前提として、Combatステート系(コンボ攻撃・パリィ)の
判定タイミングと、実際に再生されているアニメーションクリップの再生位置が
将来ズレる可能性がある設計上の課題が判明した。ユーザーとの対話(前セッション)で、
以下の結論に至った:

- `ActionFrame = 秒 × 30`という変換は「秒を真実の値とする」考え方と数式的に
  完全に一致する(既存の`MmdlActor::SetCurrentFrame`の実装と、ゲーム内通常再生の
  `m_CurrentTime += 経過秒 × TicksPerSecond`の両方と整合)。
- 区間ごとのアニメーション速度は、実行時の倍率機能ではなく、クリップの
  キーフレーム間隔自体で表現する方針に決定(`.mclp`は疎なキーフレーム形式なので
  既にこの表現力を持つ)。クリップは常に等速(1倍)で再生する。
- 上記の結果、`Combat`が`GameTime::GetDeltaTime()`を積算する独自の時計を
  持ち続ける理由が無くなった。紐づく`MmdlActor`の再生位置(秒)をそのまま毎フレーム
  読み直せば、Combat側とアニメーション側は原理的にズレない。

この設計自体はユーザーが対話の中で自ら導いたもの(Claudeは数式的な検証と
明確化の質問のみ)。詳細は`DESIGN.md`の「Combat/アニメーション時間軸の統合」節に
記録済み。

## 実装(Codexへ委任)

`tasks/current.md`に具体的な実装要件(コード例つき)を明文化して委任:

1. `MmdlActor::GetCurrentAnimationSeconds()`を追加(`m_CurrentTime / TicksPerSecond`)。
2. `IMesh`に同名の純粋仮想メソッドを追加。
3. `MMdlMesh`で`MmdlActor`へ委譲するだけの実装。
4. `MeshObject`に委譲メソッドを追加(`_DEBUG`限定にしない)。
5. `Combat::Update()`の冒頭で`m_CurrentTime = GetPlayer()->GetCurrentAnimationSeconds();`
   に置き換え、独自の delta time 積算(`Time.h`の`GameTime::GetDeltaTime()`)を廃止。

`IMesh`への純粋仮想追加に伴い、スコープ外だった旧`PMXMesh`/`XActor`/`XMesh`
(90_Legacy)にもコンパイルを通すための最小限の互換実装が必要になった。

## レビュー結果(Claude)

- `git diff`で`MMdlActor.h`/`IMesh.h`/`MMdlMesh.h/.cpp`/`MeshObject.h`/
  `Combat.h/.cpp`の変更が仕様通りであることを確認。
- Legacy側の追加を確認: `PMXMesh::GetCurrentAnimationSeconds()`は既存の
  `PMXActor::GetCurrentAnimationTime()`(フレーム単位、`AnimationEditor.cpp`で
  既に表示に使われている既存API)を`/30.0f`しているだけで妥当。`XActor`は
  `MmdlActor`と全く同じパターン(`m_CurrentTime / TicksPerSecond`)。どちらも
  新規APIの発明ではなく既存の値へのアクセサ追加のみ。
- `tasks/current.md`のNotesで指摘していた懸念(`Enter()`時点の
  `m_CurrentTime = 0.0f`初期化と新方式の整合性)を検証: `Combat::Enter()`
  →`ApplyNamedClip(...)`という順序が`AttackCombo_0/1/2`・`Parry`の4状態全てで
  一貫しており、次フレームの`Update()`で`GetCurrentAnimationSeconds()`が
  自然に0近傍から始まるため矛盾なし。
- Claude側で独立して`scripts\build.ps1`を再実行し、Debug|x64で**0エラー・
  0警告**を確認(Codex自己申告の`build.passed: false`は既知の
  `pwsh.exe`欠如によるvcpkg AppLocal後処理の非致命的な失敗のみで、
  コンパイル・リンク自体は成功)。
- `scripts\check-bom.ps1 -Fix`で追跡対象217ファイル全てのBOMを確認。

## 未検証

- 実機起動してのPlayer攻撃コンボ・パリィの体感動作確認は未実施(コードレビュー
  ベースでは従来通りの挙動になるはずだが、CLIからの確認はできていない)。

## 結果

成功。設計・実装・レビューとも完了。
