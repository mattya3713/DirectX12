# DebugBridge Protocol 仕様 (v1)

> C++ゲーム(閃斬)と外部C# Editor間のNamed Pipe通信契約。
> 本文書は後続のC++サーバー実装タスク・C#クライアント実装タスクが参照する唯一のプロトコル原本。
> 担当: 玄武(2026-08-23)。実装(Named Pipeサーバー/C# UI)は本書では扱わない。

---

## 1. 全体構成

```text
+------------------+        Named Pipe (制御用)         +------------------+
|  C# Editor       | <== request / response / event ==> |  C++ Game        |
|  (Client)        |                                    |  (Server)        |
|                  |        Named Pipe (Telemetry用)     |                  |
|                  | <== event のみ・単方向・高頻度 ==== |                  |
+------------------+                                    +------------------+
```

- **Client** = C# Editor。常に要求(Request)を送る側。
- **Server** = C++ゲーム。要求に応答(Response)し、状態変化をイベント(Event)として能動通知する。
- ゲームはServer起動を常時行い、Clientの接続待ち受け中もゲームループは止めない。

## 2. 経路分離(制御とTelemetry)

| 経路 | パイプ名 | 方向 | 頻度 | 信頼性方針 |
|---|---|---|---|---|
| 制御(Control) | `senzan.debugbridge.control.v1` | 双方向 | 低頻度(目安30msg/s以下) | Request/Responseの対応を必ず保証する |
| Telemetry | `senzan.debugbridge.telemetry.v1` | Server→Client単方向 | 高頻度(既定10Hz、設定可能) | 輪損許容。再同期はClient側の責務 |

- 2経路は**別パイプ**とし、高頻度Telemetryで制御メッセージが滞留しないようにする。
- Telemetryの受信開始は制御経路の`telemetry.subscribe`でのみ行う(勝手に流さない)。
- パイプ名にバージョン(v1)を含めるため、メジャーバージョン不一致時はそもそも接続が成立しない。

## 3. メッセージ形式

- エンコーディング: UTF-8。
- フレーミング: **1メッセージ=1行**(改行`\n`区切り。JSON内に生の改行を含めない)。
- 最大サイズ: **1MB**超のメッセージは受信側が破棄する(エラーカウンタを進める)。

### 3.1 共通フィールド

| フィールド | 型 | 必須 | 説明 |
|---|---|---|---|
| protocolVersion | number | ○ | メジャー番号(現在1)。マイナーは含めない |
| type | string | ○ | `"request"` / `"response"` / `"event"` |
| id | string | request/responseのみ必須 | 相関ID。Clientが採番(例: `"req-42"`)。ResponseはRequestのidをそのまま返す。Eventでは省略 |
| command | string | requestのみ | `<ドメイン>.<動作>` 形式(§5) |
| ok | bool | responseのみ | 成功true/失敗false |
| error | object | responseのok=false時 | `{ "code": "...", "message": "..." }`(§6) |
| payload | object | 任意 | コマンド固有データ。無い場合は省略可 |

### 3.2 Request

```json
{
  "protocolVersion": 1,
  "type": "request",
  "id": "req-42",
  "command": "combat.get_state",
  "payload": {}
}
```

### 3.3 Response(成功)

```json
{
  "protocolVersion": 1,
  "type": "response",
  "id": "req-42",
  "ok": true,
  "payload": {
    "player": { "hp": 100.0, "maxHp": 100.0, "state": "Run", "combo": 3 },
    "boss":   { "hp": 55.0,  "maxHp": 100.0, "state": "Move" },
    "timeScale": 0.4,
    "paused": false
  }
}
```

### 3.4 Response(失敗)

```json
{
  "protocolVersion": 1,
  "type": "response",
  "id": "req-43",
  "ok": false,
  "error": { "code": "E_UNKNOWN_COMMAND", "message": "unknown command: combat.hoge" }
}
```

### 3.5 Event(Server→Clientへの一方的通知)

```json
{
  "protocolVersion": 1,
  "type": "event",
  "event": "entity.hp_changed",
  "payload": { "who": "boss", "hp": 48.0, "maxHp": 100.0 }
}
```

## 4. 接続ライフサイクル

### 4.1 ハンドシェイク

接続直後、Clientは**最初のメッセージとして必ず** `handshake` を送る:

```json
{
  "protocolVersion": 1,
  "type": "request",
  "id": "h1",
  "command": "handshake",
  "payload": { "client": "SenzanEditor", "clientVersion": "0.1.0" }
}
```

Server応答:

```json
{
  "protocolVersion": 1,
  "type": "response",
  "id": "h1",
  "ok": true,
  "payload": { "server": "SenzanGame", "serverVersion": "0.4.0", "commands": ["handshake","ping","combat.get_state"] }
}
```

### 4.2 状態遷移図

```text
Client:  Disconnected → Connecting → AwaitingHandshake → Ready ──(切断/タイムアウト)→ Disconnected
Server:  Listening → Connected(AwaitingHandshake) → Ready ──(切断)→ Listening
```

- `Ready`到達前(`AwaitingHandshake`中)に`handshake`以外を受けたら即切断(E_PROTOCOL_VERSION/E_INVALID_PARAMS)。
- Clientのメジャーバージョン不一致は`E_PROTOCOL_VERSION`応答後に切断。

### 4.3 タイムアウト方針

| 項目 | 既定値 | 責務 |
|---|---|---|
| ClientのRequestタイムアウト | 3000ms | Client側計測。タイムアウトしても接続は切らない(idを破棄して継続) |
| Handshakeタイムアウト | 2000ms | Server側。`handshake`が来なければ接続を閉じる |
| Server処理バジェット | 2ms/フレーム | 制御メッセージ処理がゲームフレームをブロックしないための上限(溢れた分は次フレームへ繰り越し) |

### 4.4 切断・不正入力の方針

| 状況 | Serverの挙動 |
|---|---|
| Client切断 | セッション状態を破棄、Telemetry配信停止、エラーカウンタ進行。**ゲームは正常続行** |
| 不正JSON(パース失敗) | idを特定できないため応答せず破棄+カウンタ進行。`bridge.error`イベントを送れる場合は送信 |
| 未知コマンド | `E_UNKNOWN_COMMAND`でResponse(切断はしない) |
| 不正パラメータ | `E_INVALID_PARAMS`でResponse |
| 1MB超メッセージ | 破棄+カウンタ進行 |
| Server側例外 | `E_INTERNAL`でResponse。ゲームループへの影響は絶対に出さない |

## 5. 命名規約

- コマンド名: **`<ドメイン>.<動作>` のsnake_case**。例: `combat.get_state`, `game.set_timescale`, `player.set_ult`, `ui.reload_layout`
- イベント名: **`<ドメイン>.<過去形/状態名>`**。例: `entity.hp_changed`, `game.state_changed`
- ドメイン一覧(v1): `game` / `combat` / `player` / `boss` / `ui` / `telemetry` / `protocol` / `bridge`
- 予約コマンド: `handshake`, `ping`, `protocol.list_commands`(Serverが対応コマンド一覧を返す)
- JSONフィールド名: LowerCamelCase(`maxHp` 等)。enum的な文字列値はPascalCase(`"AttackCombo_0"`等、ゲーム内State名と一致させる)
- **前方互換規約**: 受信側は未知のフィールドを必ず無視する。フィールド追加=マイナー更新(互換)、既存フィールドの削除/意味変更=メジャー更新(非互換)

## 6. エラーコード一覧

| code | 意味 |
|---|---|
| E_PARSE | JSONとして解釈できない(原則応答なし。可能なら bridge.error) |
| E_PROTOCOL_VERSION | メジャーバージョン不一致 |
| E_UNKNOWN_COMMAND | 未登録のコマンド |
| E_INVALID_PARAMS | payloadの型/範囲/必須項目不正 |
| E_NOT_IMPLEMENTED | 登録済みだが未実装 |
| E_INTERNAL | Server内部エラー |
| E_TIMEOUT | Server内処理が時間切れ |

## 7. Telemetry契約

- 購読: 制御経路で `{"command":"telemetry.subscribe","payload":{"hz":10}}` を送る。
- 開始時、ServerはTelemetry経路へ1回だけスキーマ通知を流す:

```json
{ "protocolVersion": 1, "type": "event", "event": "telemetry.hello",
  "payload": { "hz": 10, "fields": ["t","fps","player.hp","player.state","boss.hp","boss.state","timescale"] } }
```

- 以降、定周期で行形式を流す(lossy。Clientは取りこぼしを気にしない):

```json
{ "type":"frame", "seq": 120, "values": [12.50, 60.1, 100.0, "Idle", 55.0, "Move", 1.0] }
```

- 停止: `telemetry.unsubscribe`。

## 8. サンプル交換シーケンス(制御経路)

```text
C→S  handshake                       (id=h1)
S→C  ok=true, commands=[...]         (id=h1)
C→S  ping                            (id=r1)
S→C  ok=true, payload={"pong":...}   (id=r1)
C→S  combat.get_state                (id=r2)
S→C  ok=true, payload={...戦闘状態}  (id=r2)
S→C  event entity.hp_changed         (Boss被弾)
C→S  game.set_timescale {0.4}        (id=r3)
S→C  ok=true                         (id=r3)
C→S  unknown.command                 (id=r4)
S→C  ok=false E_UNKNOWN_COMMAND      (id=r4)
```

## 9. 実装上の注意(後続タスクへの申し送り)

- Serverはゲームループ内でポーリング方式を推奨(スレッドを増やす場合もゲーム状態へのアクセスはメインスレッドに集約すること)。
- 応答生成時にゲーム状態のコピーを取ってからJSON化する(ロック長時間保持を避ける)。
- C#側は`System.IO.Pipes.NamedPipeClientStream`+改行区切りテキスト読み取りで足りる。
- 将来のバイナリ化(Telemetry高速化)はメジャー更新としてv2で検討する。

---
Last updated: 2026-08-23(玄武。初版策定)
