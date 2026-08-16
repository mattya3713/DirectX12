#pragma once

#include<d3d12.h>
#include<vector>
#include<memory>

// 前方宣言.
class DirectX12;
class PMDActor;

class PMDRenderer
{
	friend PMDActor;
public:
	PMDRenderer(DirectX12& dx12);
	~PMDRenderer();
	void Update();
	void Draw();

	// PMD用のパイプラインステートを取得.
	ID3D12PipelineState* GetPipelineState();

	// PMD用のルート署名を取得.
	ID3D12RootSignature* GetRootSignature();

private:
	// テクスチャの汎用素材を作成.
	ID3D12Resource* CreateDefaultTexture(size_t Width, size_t Height);
	// 白テクスチャの生成.
	ID3D12Resource* CreateWhiteTexture();
	// 黒テクスチャの生成.
	ID3D12Resource* CreateBlackTexture();
	// グレーテクスチャの生成.
	ID3D12Resource* CreateGrayGradationTexture();

	// パイプライン初期化.
	void CreateGraphicsPipelineForPMD();
	// ルートシグネチャ初期化.
	void CreateRootSignature();

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

private:
	DirectX12& m_pDx12;

	MyComPtr<ID3D12PipelineState>	m_pPipelineState;		// パイプライン.
	MyComPtr<ID3D12RootSignature>	m_pRootSignature;		// ルートシグネチャ.

	//PMD用共通テクスチャ.
	MyComPtr<ID3D12Resource>		m_pWhiteTex;			// 白色のテクスチャ.
	MyComPtr<ID3D12Resource>		m_pBlackTex;			// 黒色のテクスチャ.
	MyComPtr<ID3D12Resource>		m_pGradTex;				// 白<->黒グラデーションのテクスチャ.

};

