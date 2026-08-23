# DebugBridgeサーバーへの疎通確認クライアント(ServerSmokeHarness.cppから起動される).
# handshake → ping → bridge.get_runtime_info → debugbridge.get_snapshot → 未知コマンド の順に送信し、
# 応答を検証して「SMOKE PASS」を出力する.

$ErrorActionPreference = 'Stop'

$pipe = [System.IO.Pipes.NamedPipeClientStream]::new('.', 'senzan.debugbridge.control.v1', [System.IO.Pipes.PipeDirection]::InOut)
$pipe.Connect(10000)

$writer = [System.IO.StreamWriter]::new($pipe)
$writer.AutoFlush = $true
$reader = [System.IO.StreamReader]::new($pipe)

function Send-Receive([hashtable]$Message)
{
    $writer.WriteLine(($Message | ConvertTo-Json -Compress -Depth 6))
    $line = $reader.ReadLine()
    if (-not $line) { throw "no response" }
    return $line | ConvertFrom-Json
}

# 1. handshake
$r1 = Send-Receive @{ protocolVersion = 1; type = 'request'; id = 'h1'; command = 'handshake' }
if ($r1.id -ne 'h1')      { throw "handshake: id mismatch" }
if (-not $r1.ok)          { throw "handshake failed" }
if ($r1.payload.server -ne 'SenzanGame') { throw "handshake payload mismatch" }

# 2. ping
$r2 = Send-Receive @{ protocolVersion = 1; type = 'request'; id = 'p1'; command = 'ping' }
if (-not $r2.ok)          { throw "ping failed" }
if ($r2.payload.pong -ne $true) { throw "ping payload mismatch" }

# 3. runtime info
$r3 = Send-Receive @{ protocolVersion = 1; type = 'request'; id = 'ri1'; command = 'bridge.get_runtime_info' }
if (-not $r3.ok)                       { throw "runtime info failed" }
if ($r3.payload.player.hp -le 0)       { throw "runtime info player hp missing" }
if ($r3.payload.boss.hp -le 0)         { throw "runtime info boss hp missing" }
if ($r3.payload.timeScale -ne 1.0)     { throw "runtime info timescale mismatch" }

# 4. snapshot
$r4 = Send-Receive @{ protocolVersion = 1; type = 'request'; id = 's1'; command = 'debugbridge.get_snapshot' }
if (-not $r4.payload.snapshot)         { throw "snapshot flag missing" }

# 5. 未知コマンド(ゲームは落ちてはいけない)
$r5 = Send-Receive @{ protocolVersion = 1; type = 'request'; id = 'u1'; command = 'combat.hoge' }
if ($r5.error.code -ne 'E_UNKNOWN_COMMAND') { throw "unknown command error expected" }

# 6. 落ちた後もpingが通ること
$r6 = Send-Receive @{ protocolVersion = 1; type = 'request'; id = 'p2'; command = 'ping' }
if (-not $r6.ok) { throw "post-error ping failed" }

"SMOKE PASS"
