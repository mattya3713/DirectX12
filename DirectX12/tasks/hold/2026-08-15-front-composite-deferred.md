# タスク: PMXのAlpha修正と髪越しパーツの汎用合成

## 目的

PMXモデルの顔周辺で、非表示用のマテリアルまで描画されて黒くなる問題を修正する。
そのうえで、Hakuに付属するPostAlphaEyeの考え方を、モデル名に依存しないランタイム機能として移植する。
目・眉・まつ毛などの指定パーツを、顔や体は遮蔽したまま、指定した髪パーツだけを透過して表示できるようにする。

## 学習項目

### 今回必須

- PMXのマテリアルDiffuse AlphaとテクスチャAlphaを合成する理由。
- 半透明描画で、アルファブレンド・深度テスト・深度書き込みが別の役割を持つこと。
- 単純な描画順変更ではなく、オフスクリーンRenderTargetと深度マスクが必要な理由。
- PMXの標準マテリアル情報と、PostAlphaEyeのような外部MMEエフェクト情報を分離する理由。
- `.mskin`のサブメッシュ情報が、共有される`.mmat`ではなくモデル固有の描画役割を持つ理由。

### 理解推奨

- PMXParserからModel::Material、RuntimeConverter、`.mmat`/`.mskin`、XActor、PMXシェーダーへ情報が渡る経路。
- D3D12のRenderTargetリソース状態遷移と、フレーム中の一時リソースの寿命。
- 不透明パスと半透明パスを分離する一般的な描画設計。

### 今回は後回し

- MMEの.fxを汎用的に解釈する仕組み。
- AlternativeFull.fxの完全互換。
- PMXの追加UVを使うSphereMode 3。
- マテリアルモーフによるAlphaや描画役割の動的変更。
- 輪郭線、セルフシャドウ、スカート用PostAlphaEyeなど、今回の顔合成以外のMME機能。

## スコープ

### Phase 1: PMX標準Alphaの修正

- PMX/ランタイムのマテリアルAlphaをピクセルシェーダーの最終Alphaへ反映する。
- テクスチャAlphaとDiffuse Alphaを乗算し、Diffuse Alphaが0のマテリアルを見えない状態にする。
- 半透明マテリアルが深度を書き込んで後続の顔パーツを不正に遮蔽しないようにする。
- 既存の不透明描画結果を変えない。
- 既存の白テクスチャ/黒テクスチャのフォールバック挙動を壊さない。

### Phase 2: 汎用FrontComposite

- HakuやPMXという名前をレンダラーへハードコードしない。
- `.mskin`のサブメッシュへ、通常、合成ソース、合成時に奥へ緩和する遮蔽物の役割を保存できるようにする。
- 変換元ファイルの隣に置く任意のインポート設定ファイルから、合成ソースと遮蔽物のサブメッシュ番号、Opacity、MaxDistanceを読み込む。
- 設定ファイルが無いモデルは従来どおり通常描画する。
- 変換時に設定のサブメッシュ番号が範囲外なら、変換を失敗させ、対象ファイルと番号をエラーへ含める。
- `.mskin`フォーマットを後方互換なしでバージョンアップし、旧バージョンを読み込まない。
- サブメッシュ役割は共有`.mmat`へ入れない。役割はモデル内のサブメッシュに属するためである。
- FrontComposite有効時は、次の描画を行う。
  1. 通常のモデルをシーンへ描画する。
  2. 透明なオフスクリーンRenderTargetと深度バッファを用意する。
  3. 合成ソース以外の通常パーツを遮蔽用深度として描画する。
  4. 緩和対象の髪パーツだけを、合成用パスでMaxDistance分だけカメラから遠ざけて深度へ描画する。
  5. 合成ソースを深度テスト付きで描画し、顔や体には遮蔽され、髪だけには遮蔽されない状態を作る。
  6. オフスクリーン結果をシーンへOpacity付きで合成する。
- オフスクリーンリソースはActorまたはRendererの所有オブジェクトとしてRAII管理し、毎フレームGPU使用中に破棄・再作成しない。
- ウィンドウ/SceneViewのサイズ変更時にオフスクリーンサイズを安全に追従させる。最初の実装では現在の描画対象サイズを使用し、0サイズは作成しない。
- 既存の直接描画とAnimationEditorのSceneView描画の両方で、同じ描画結果になるようにする。

## 入力設定

インポート設定ファイル名は、入力モデルの拡張子を除いた名前に`.mmdl.json`を付ける。
例: `SakurabaEma_ByPOWER.mmdl.json`。

形式は次のとおりとする。

```json
{
  "frontComposite": {
    "sourceSubmeshes": [3],
    "relaxedOccluders": [4, 10, 11],
    "opacity": 0.45,
    "maxDistance": 1.0
  }
}
```

- `frontComposite`が無い場合は無効。
- `sourceSubmeshes`と`relaxedOccluders`は整数配列で、空配列は禁止。
- 同じ番号が両方に存在する場合はエラー。
- `opacity`は0以上1以下、`maxDistance`は0以上でなければエラー。
- JSONパーサーは既存の`Data/Library/json/json.hpp`を使い、新しい外部依存を追加しない。
- Haku用設定ファイルは、既存の`PostAlphaEye/DrawEye.fx`にある`EYE_SUBSET=3`、`HAIR_SUBSET=4,10,11`に合わせる。

## 想定変更箇所

- `SourceCode/10_Ggraphic/RuntimeFormat/RuntimeFormat.h/.cpp`または対応するIO実装: MSKNバージョン更新、サブメッシュ役割、FrontComposite設定の保存・読み込み。
- `SourceCode/10_Ggraphic/RuntimeFormat/RuntimeConverter.cpp/.h`: `.mmdl.json`の読み込み、妥当性検証、PMX/X変換結果への役割付与。
- `SourceCode/10_Ggraphic/RuntimeFormat/RuntimeConverterMain.cpp`: 既存の自動変換フローで設定ファイルを同じ入力モデルから解決できるようにする。
- `SourceCode/10_Ggraphic/X/XActor.cpp/.h`: MSKNから新しい役割と設定を読み、合成描画を実行する。
- `SourceCode/10_Ggraphic/PMX/PMXRenderer.cpp`または共有描画パイプライン: 不透明/半透明/合成用パイプラインを追加または安全に分離する。
- `Data/Shader/PMX/Pixel.hlsl`および必要な頂点シェーダー: Diffuse Alpha、合成用変位、オフスクリーン合成を実装する。
- `Data/Model/PMX/haku/SakurabaEma_ByPOWER.mmdl.json`: Hakuの検証用設定を追加する。入力モデルの実際のファイル名を確認して一致させる。
- `DESIGN.md`: `.mskin`の新バージョンとFrontCompositeの設計、今回実装しないMME機能を追記する。

## スコープ外

- PMX/Xを実行時に直接読む方式への復帰。
- SDEFの本格実装。
- MME.fxの自動解析。
- すべてのPMX標準描画機能の完全再現。
- 既存の未関連なカメラ、ImGui、アニメーション、ファイル削除、コミット。
- Git commit。

## 受け入れ基準

1. Hakuの`Face_Ex`のようにDiffuse Alphaが0のマテリアルが黒い板として表示されない。
2. Alphaが0.5程度の顔パーツが、テクスチャAlphaと合わせて正しく半透明表示される。
3. FrontComposite設定が無いPMX/Xは、変換・ロード・描画が従来どおり成功する。
4. Haku設定を付けたモデルでは、目・眉・まつ毛が髪の領域だけを通過し、顔や体の背後まで貫通しない。
5. 設定ファイルの不正な番号・値は、曖昧に無視せず変換エラーとして報告される。
6. `.mskin`の旧バージョンは明示的に拒否される。
7. Debug/x64とRelease/x64のビルドが警告0で成功する。
8. 既存のXモデル、PMXモデル、VMDなしPMXが起動時にクラッシュしない。
9. 変更した`.cpp`/`.h`にUTF-8 BOMがあり、`using`宣言を追加していない。
10. Codex完了後、変更ファイル、実装内容、ビルド結果、未解決点を`tasks/.codex-last-report.json`で確認できる。

## Codexへの実装指示

この仕様書の範囲だけを実装すること。D3D12リソース状態遷移、RenderTarget、深度バッファ、フレーム中のリソース寿命について既存実装を確認し、推測で新しい所有モデルを作らないこと。
要件を満たすために大規模な既存API変更が必要になった場合、独断で拡張せず、完了レポートの未解決点へ記載して停止すること。
実装後、Debugビルドを行い、可能ならReleaseビルドも行うこと。テスト用Haku設定が見つからない場合は、設定ファイルを作成する前に入力モデルの実ファイル名を確認すること。
