#pragma once

#include<d3d12.h>
#include<vector>

// 前方宣言.
class DirectX12;
class PMXActor;
class MmdlActor;

/**************************************************
*	PMX用描画パイプラインクラス.
*	担当：淵脇 未来
**/

class MmdlRenderer
{
	friend PMXActor; // Legacy PMXActorが既存のRenderer内部DirectX12を利用するため.
	friend MmdlActor;
public:
	MmdlRenderer(DirectX12& dx12);
	~MmdlRenderer();
	void BeforDraw();

	// PMD用のパイプラインステートを取得.
	ID3D12PipelineState* GetPipelineState();
	// 半透明マテリアル用のパイプラインステートを取得.
	ID3D12PipelineState* GetTransparentPipelineState();

	// PMD用のルート署名を取得.
	ID3D12RootSignature* GetRootSignature();

	// シャドウ深度パスを開始する(光源視点への描画設定+シャドウマップのクリア).
	void BeginShadowPass();
	// シャドウ深度パスを終了する(シャドウマップをSRV状態へ遷移しメインパスのレンダーターゲットを復帰させる).
	void EndShadowPass();
	// シャドウマップテクスチャの取得(Actorがピクセルシェーダー用SRVを作るのに使う. 未初期化時はnullptr).
	ID3D12Resource* GetShadowMapResource() const noexcept { return m_pShadowMap.Get(); }
	
	// デフォルトの透明テクスチャを取得.
	MyComPtr<ID3D12Resource>& GetAlphaTex();
	// デフォルトの白テクスチャを取得.
	MyComPtr<ID3D12Resource>& GetWhiteTex();
	// デフォルトの黒テクスチャを取得.
	MyComPtr<ID3D12Resource>& GetBlackTex();
	// デフォルトの白<->黒テクスチャを取得.
	MyComPtr<ID3D12Resource>& GetGradTex();

private:
	// テクスチャの汎用素材を作成.
	ID3D12Resource* CreateDefaultTexture(size_t Width, size_t Height);
	// 透明テクスチャの生成.
	ID3D12Resource* CreateAlphaTexture();
	// 白テクスチャの生成.
	ID3D12Resource* CreateWhiteTexture();
	// 黒テクスチャの生成.
	ID3D12Resource* CreateBlackTexture();
	// グレーテクスチャの生成.
	ID3D12Resource* CreateGrayGradationTexture();

	// パイプライン初期化.
	void CreateGraphicsPipelineForPMX();
	// ルートシグネチャ初期化.
	void CreateRootSignature();
	// シャドウマップ用の深度テクスチャ・DSV・深度専用パイプラインの初期化.
	void CreateShadowResources();

	/*******************************************
	* @brief	シェーダーのコンパイル.
	* @param	ファイルパス.
	* @param	エントリーポイント.
	* @param	出力形式.
	* @param	シェーダーブロブ(巨大バイナリ).
	*******************************************/
	HRESULT CompileShaderFromFile(
		const std::wstring& FilePath,
		LPCSTR EntryPoint,
		LPCSTR Target,
		ID3DBlob** ShaderBlob);

	/*******************************************
	* @brief	事前コンパイル済みシェーダー(.cso)の読み込み(Releaseビルド用).
	*            : ビルド時にfxc.exeで.hlsl→.csoへ変換したものをそのまま読むだけで、
	*            : 実行時コンパイルもソース同梱も不要にする(配布物にシェーダーコードを含めないため).
	* @param	ファイルパス(.cso).
	* @param	シェーダーブロブ(巨大バイナリ).
	*******************************************/
	HRESULT LoadCompiledShader(
		const std::wstring& FilePath,
		ID3DBlob** ShaderBlob);

private:
	DirectX12& m_pDx12;

	MyComPtr<ID3D12PipelineState>	m_pPipelineState;		// パイプライン.
	MyComPtr<ID3D12PipelineState>	m_pTransparentPipelineState;	// 半透明パイプライン.
	MyComPtr<ID3D12RootSignature>	m_pRootSignature;		// ルートシグネチャ.

	// シャドウマッピング.
	static constexpr UINT SHADOW_MAP_SIZE = 1024;						// シャドウマップの解像度(固定. 品質チューニングは今回のスコープ外).
	MyComPtr<ID3D12Resource>		m_pShadowMap;						// 光源視点深度バッファ(D32_FLOAT).
	MyComPtr<ID3D12DescriptorHeap>	m_pShadowDSVHeap;					// シャドウマップ用DSVヒープ(1ディスクリプタ).
	MyComPtr<ID3D12PipelineState>	m_pShadowPipelineState;				// 深度専用パイプライン(VS_Shadow+PS無し).
	bool							m_ShadowMapInShaderResourceState = false; // シャドウマップの現在リソース状態(DEPTH_WRITE=false).

	//PMX用共通テクスチャ.
	MyComPtr<ID3D12Resource>		m_pAlphaTex;			// 透明のテクスチャ.
	MyComPtr<ID3D12Resource>		m_pWhiteTex;			// 白色のテクスチャ.
	MyComPtr<ID3D12Resource>		m_pBlackTex;			// 黒色のテクスチャ.
	MyComPtr<ID3D12Resource>		m_pGradTex;				// 白<->黒グラデーションのテクスチャ.

};
