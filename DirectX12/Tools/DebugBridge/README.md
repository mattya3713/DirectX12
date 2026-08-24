# DebugBridge Tools

DebugBridge Protocol v1 (`docs/debug_bridge_protocol.md`) の共有契約テストと
C# Editorツールの雛形。**Named Pipe実装は含まない**(契約検証のみ)。

## 構成

```
samples/                     C++とC#が共通で読むサンプルJSON(原本)
  request_handshake.json / response_handshake.json
  request_ping.json       / response_ping.json
  request_get_runtime_info.json / response_get_runtime_info.json
  response_error_unknown_command.json
  event_hp_changed.json
CSharp/
  DebugBridge.Contract/      エンベロープ検証ライブラリ(EnvelopeValidator)
  DebugBridge.ContractTests/ 上記の契約テスト(コンソール)
DebugBridgeContractTest.cpp  C++側契約テスト(nlohmann json使用)
DebugBridgeTools.sln         C#ツール用ソリューション(ゲームvcxprojから独立)
```

## 実行方法

### C++ (DirectX12ディレクトリから)
```powershell
cl /nologo /EHsc /std:c++20 /W4 /I Data\Library tools\DebugBridge\DebugBridgeContractTest.cpp
.\DebugBridgeContractTest.exe tools\DebugBridge\samples
```

### C# (このディレクトリから)
```powershell
dotnet build DebugBridgeTools.sln
dotnet run --project CSharp\DebugBridge.ContractTests --no-build -- samples
```

どちらも `ALL PASSED` / `8 passed, 0 failed` になれば契約一致。

## 後続AIへのルール

1. Named Pipeサーバー(C++)/クライアント(C#)実装前に、必ず本テストを通すこと。
2. サンプルJSONを変更する場合は docs/debug_bridge_protocol.md のバージョンポリシー
   (フィールド追加=マイナー、破壊=メジャー)に従い、C++/C#両テストを同時に更新すること。
3. テスト失敗時は `field '<名>' — <理由>` の出力で不一致フィールドを特定できる。
