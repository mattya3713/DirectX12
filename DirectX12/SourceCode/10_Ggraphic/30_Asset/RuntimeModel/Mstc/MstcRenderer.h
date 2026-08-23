#pragma once

#include<d3d12.h>
#include<vector>

// 前方宣言.
class DirectX12;

/**************************************************
*	Mstc(静的メッシュ)用描画パイプラインクラス.
*	ボーン無し・スキニング無しの単純な頂点シェーダーで、
*	ベースカラー+オブジェクト空間法線マップを扱う.
*	MmdlRendererと同じくScene CB(b0)はDirectX12の共有バッファを参照する.
**/

class MstcRenderer
{
	friend class MstcActor;
public:
	explicit MstcRenderer(DirectX12& dx12);
	~MstcRenderer();

	// 描画開始時にPSOとルートシグネチャをセットする.
	void BeforDraw();

	// デフォルトの白テクスチャを取得(テクスチャ欠損時のフォールバック用).
	MyComPtr<ID3D12Resource>& GetWhiteTex() { return m_pWhiteTex; }

private:
	// パイプライン初期化.
	void CreateGraphicsPipeline();
	// ルートシグネチャ初期化.
	void CreateRootSignature();

	// 白テクスチャの生成(法線マップ・ベースカラーのフォールバック用).
	ID3D12Resource* CreateWhiteTexture();

	/*******************************************
	* @brief	シェーダーのコンパイル(Debug用. .hlslソースを実行時コンパイル).
	*******************************************/
	HRESULT CompileShaderFromFile(
		const std::wstring& FilePath,
		LPCSTR EntryPoint,
		LPCSTR Target,
		ID3DBlob** ShaderBlob);

	/*******************************************
	* @brief	事前コンパイル済みシェーダー(.cso)の読み込み(Releaseビルド用).
	*******************************************/
	HRESULT LoadCompiledShader(
		const std::wstring& FilePath,
		ID3DBlob** ShaderBlob);

private:
	DirectX12& m_Dx12;

	MyComPtr<ID3D12PipelineState>	m_pPipelineState;		// パイプライン(不透明のみ).
	MyComPtr<ID3D12RootSignature>	m_pRootSignature;		// ルートシグネチャ.

	MyComPtr<ID3D12Resource>		m_pWhiteTex;			// デフォルトの白テクスチャ.
};
