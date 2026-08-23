#pragma once

#include <d3d12.h>

/**********************************************************************************
* @author    : 朱雀 (Suzaku / 閃斬 Production Loop Coder).
* @date      : 2026/08/23.
* @brief     : ポストプロセスパイプライン(Bloom).
*            : オフスクリーンのシーンカラー(BeginDraw(true)で描画される
*            : シーンカラーバッファ)に対し、輝度抽出→ガウシアンブラー→
*            : 合成の3段構成でBloomをかけ、バックバッファへ出力する.
*            : Main::Draw()のシーン描画後にApply()を1度呼ぶだけで動く.
**********************************************************************************/

class DirectX12;

class PostProcessPipeline final
{
public:
	PostProcessPipeline() = default;
	~PostProcessPipeline() = default;

	PostProcessPipeline(const PostProcessPipeline&)            = delete;
	PostProcessPipeline& operator=(const PostProcessPipeline&) = delete;

	// ルートシグネチャ・PSO・既定テクスチャを作成する(Main::Createから1度呼ぶ).
	bool Create(DirectX12& Dx12);

	// シーン描画完了後に呼ぶ. Bloomを適用してバックバッファへ出力する.
	void Apply();

private:
	// ルートシグネチャとPSO群を作成する.
	void CreateRootSignatureAndPso();

	// 中間ターゲット(輝度/ブラー用の半分解像度テクスチャ3枚)を作成する.
	void CreateIntermediateTargets(UINT Width, UINT Height);

	// 未使用SRV用の白テクスチャを作成する.
	ID3D12Resource* CreateWhiteTexture();

	// シェーダーのコンパイル(Debug: .hlslを実行時コンパイル. Release: .csoを読む).
	HRESULT CompileShaderFromFile(const std::wstring& FilePath, LPCSTR EntryPoint, LPCSTR Target, ID3DBlob** ShaderBlob);
	HRESULT LoadCompiledShader(const std::wstring& FilePath, ID3DBlob** ShaderBlob);

private:
	DirectX12* m_pDx12 = nullptr; // Create()で設定される.

	MyComPtr<ID3D12RootSignature>	m_pRootSignature;	// ルートシグネチャ(t0/t1+RootConstants+静的サンプラー).
	MyComPtr<ID3D12PipelineState>	m_pBrightExtractPso;	// 輝度抽出パス.
	MyComPtr<ID3D12PipelineState>	m_pBlurPso;				// ガウシアンブラーパス(H/V共用).
	MyComPtr<ID3D12PipelineState>	m_pCompositePso;		// 合成パス.

	MyComPtr<ID3D12Resource>	m_pWhiteTex;				// 未使用SRV用の白テクスチャ.

	// Bloom中間ターゲット(バックバッファの半分解像度).
	MyComPtr<ID3D12Resource>	m_pBrightTexture;			// 輝度抽出結果.
	MyComPtr<ID3D12Resource>	m_pBlurTempA;				// ブラーピンポンバッファA(水平ブラー出力).
	MyComPtr<ID3D12Resource>	m_pBlurTempB;				// ブラーピンポンバッファB(垂直ブラー出力).
	MyComPtr<ID3D12DescriptorHeap>	m_pRtvHeap;				// 中間ターゲット3枚分のRTV.
	MyComPtr<ID3D12DescriptorHeap>	m_pSrvHeap;				// SRVヒープ(scene/bright/A/Bの4個).

	UINT m_TargetWidth  = 0; // 現在の中間ターゲット幅(バックバッファの1/2).
	UINT m_TargetHeight = 0; // 現在の中間ターゲット高さ.

	float m_Threshold  = 0.6f; // 輝度しきい値(仮値. Playtestで調整).
	float m_Intensity  = 0.8f; // Bloom強度(仮値).
};
