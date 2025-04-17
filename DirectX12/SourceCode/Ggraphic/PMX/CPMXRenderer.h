﻿#pragma once

#include<d3d12.h>
#include<vector>
#include<memory>

// 前方宣言.
class CDirectX12;
class CPMXActor;

/**************************************************
*	PMX用描画パイプラインクラス.
*	担当：淵脇 未来
**/

class CPMXRenderer
{
	friend CPMXActor;
public:
	CPMXRenderer(CDirectX12& dx12);
	~CPMXRenderer();
	void Update();
	void BeforDraw();
	void Draw();

	void AddActor(std::shared_ptr<CPMXActor> pActor);

	// PMD用のパイプラインステートを取得.
	ID3D12PipelineState* GetPipelineState();

	// PMD用のルート署名を取得.
	ID3D12RootSignature* GetRootSignature();
	
	// デフォルトの白テクスチャを取得.
	MyComPtr<ID3D12Resource>& GetWhiteTex();
	// デフォルトの黒テクスチャを取得.
	MyComPtr<ID3D12Resource>& GetBlackTex();
	// デフォルトの白<->黒テクスチャを取得.
	MyComPtr<ID3D12Resource>& GetGradTex();

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
	void CreateGraphicsPipelineForPMX();
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
	CDirectX12& m_pDx12;

	MyComPtr<ID3D12PipelineState>	m_pPipelineState;		// パイプライン.
	MyComPtr<ID3D12RootSignature>	m_pRootSignature;		// ルートシグネチャ.

	//PMX用共通テクスチャ.
	MyComPtr<ID3D12Resource>		m_pWhiteTex;			// 白色のテクスチャ.
	MyComPtr<ID3D12Resource>		m_pBlackTex;			// 黒色のテクスチャ.
	MyComPtr<ID3D12Resource>		m_pGradTex;				// 白<->黒グラデーションのテクスチャ.

	std::vector<std::shared_ptr<CPMXActor>> m_pActors;		// キャラ.
};

