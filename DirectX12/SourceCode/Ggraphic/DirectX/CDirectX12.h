#pragma once

//警告についてのコード分析を無効にする.4005:再定義.
#pragma warning(disable:4005)

//ヘッダ読込.
#include <D3D12.h>
#include <d3d12.h>
#include <d3dx12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include <DirectXTex.h>

// XXX : 昔のヘッダーがincludeされてしまうため直パス.
#include "d3dcompiler.h"	

//ライブラリ読み込み.
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

// モデルの頂点サイズ.
constexpr size_t PmdVertexSize = 38;

/**************************************************
*	DirectX12セットアップ.
**/
class CDirectX12
{
public:

	// 頂点構造体.
	struct VerTex
	{
		DirectX::XMFLOAT3 Pos;	// xyz座標.
		DirectX::XMFLOAT2 uv;	// uv座標.
	};

	// PMDヘッダー構造体.
	struct PMDHeader
	{
		float Version;			// バージョン.
		char ModelName[20];		// モデルの名前.
		char ModelComment[256];	// モデルのコメント.
	};

	// PMD頂点構造体.
	struct PMDVertex
	{
		DirectX::XMFLOAT3 Pos;		// 頂点座標		: 12Byte.
		DirectX::XMFLOAT3 Normal;	// 法線ベクトル	: 12Byte.
		DirectX::XMFLOAT2 uv;		// uv座標		:  8Byte.
		uint16_t BoneNo[2];			// ボーン番号	:  4Byte.
		uint8_t  BoneWeight;		// ボーン影響度	:  1Byte.
		uint8_t  EdgeFlg;			// 輪郭線フラグ :  1Byte.
		uint16_t Padding;			// パディング	:  2Byte.
	};								// 合計         : 40Byte.

	// TODO : パディングの対処法を考える.
	//PMDマテリアル構造体
#pragma pack(1)
	struct PMDMaterial {
		DirectX::XMFLOAT3 Diffuse;  // ディフューズ色			: 12Byte.
		float	 Alpha;				// α値						:  4Byte.
		float    Specularity;		// スペキュラの強さ			:  4Byte.
		DirectX::XMFLOAT3 Specular; // スペキュラ色				: 12Byte.
		DirectX::XMFLOAT3 Ambient;  // アンビエント色			: 12Byte.
		uint8_t  ToonIdx;			// トゥーン番号				:  1Byte.
		uint8_t  EdgeFlg;			// Material毎の輪郭線フラグ	:  1Byte.
//		uint16_t Padding;			// パディング				:  2Byte.
		uint32_t IndicesNum;		// 割り当たるインデックス数	:  4Byte.
		char     TexFilePath[20];	// テクスチャファイル名		: 20Byte.
	};// 70Byte(パディングなし).
#pragma pack()

	// シェーダ側に投げられるマテリアルデータ.
	struct MaterialForHlsl {
		DirectX::XMFLOAT3 Diffuse;		// ディフューズ色.		
		float	 Alpha;					// α値.		
		DirectX::XMFLOAT3 Specular;		// スペキュラの強.		
		float	 Specularity;			// スペキュラ色.		
		DirectX::XMFLOAT3 Ambient;		// アンビエント色.		
	};

	// それ以外のマテリアルデータ.
	struct AdditionalMaterial {
		std::string TexPath;	// テクスチャファイルパス.
		int			ToonIdx;	// トゥーン番号.
		bool		EdgeFlg;	// マテリアル毎の輪郭線フラグ.
	};

	// まとめたもの.
	struct Material {
		unsigned int IndicesNum;	//インデックス数.
		MaterialForHlsl Material;
		AdditionalMaterial Additional;
	};

public:
	CDirectX12();
	~CDirectX12();

	//DirectX12構築.
	bool Create(HWND hWnd);
	void UpDate();

	void BeginDraw();
	void Draw();
	void EndDraw();

	// スワップチェーン取得.
	const MyComPtr<IDXGISwapChain4> GetSwapChain();

	// DirextX12デバイス取得.
	const MyComPtr<ID3D12Device> GetDevice();

	// コマンドリスト取得.
	const MyComPtr<ID3D12GraphicsCommandList> GetCommandList();

	MyComPtr<ID3D12Resource> GetTextureByPath(const char* texpath);

	//デバイスコンテキストを取得.
	//ID3D12Device* GetDevice() const { return m_pDevice12; }


private:// 作っていくんだよねぇ.

	// DXGIの生成.
	void CreateDXGIFactory(MyComPtr<IDXGIFactory6>& DxgiFactory);

	// コマンド類の生成.
	void CreateCommandObject(
		MyComPtr<ID3D12CommandAllocator>&	CmdAllocator,
		MyComPtr<ID3D12GraphicsCommandList>&CmdList,
		MyComPtr<ID3D12CommandQueue>&		CmdQueue);

	// スワップチェーンの作成.
	void CreateSwapChain(MyComPtr<IDXGISwapChain4>& SwapChain);

	// レンダーターゲットの作成.
	void CreateRenderTarget(
		MyComPtr<ID3D12DescriptorHeap>&			RenderTargetViewHeap,
		std::vector<MyComPtr<ID3D12Resource>>&	BackBuffer);

	// 深度バッファの作成.
	void CreateDepthDesc(
		MyComPtr<ID3D12Resource>&		DepthBuffer,
		MyComPtr<ID3D12DescriptorHeap>&	DepthHeap);

	// フェンスの作成.
	void CreateFance(MyComPtr<ID3D12Fence>& Fence);

	// グラフィックパイプラインステートの設定.
	void CreateGraphicPipeline(MyComPtr<ID3D12PipelineState>& GraphicPipelineState);

	//テクスチャローダテーブルの作成.
	void CreateTextureLoadTable();

	// textureの作成.
	ID3D12Resource* CreateGrayGradationTexture();
	ID3D12Resource* CreateWhiteTexture();
	ID3D12Resource* CreateBlackTexture();
	// 読み込み
	ID3D12Resource* LoadTextureFromFile(std::string& texPath);


private:
	/*******************************************
	* @brief	アダプターを見つける.
	* @param	検索する文字列.
	* @return   見つけたアダプターを返す.
	*******************************************/
	IDXGIAdapter* FindAdapter(std::wstring FindWord);

	/*******************************************
	* @brief	デバッグレイヤーを起動.
	*******************************************/
	void EnableDebuglayer();

	/*******************************************
	* @brief	ErroeBlobに入ったエラーを出力.
	* @param	そもそもファイルがあるかどうか.
	* @param	その他のコンパイルエラー内容.
	*******************************************/
	void ShaderCompileError(const HRESULT& Result, ID3DBlob* ErrorMsg);


	/*******************************************
	* @brief	シェーダーのコンパイル.
	* @param	ファイルパス.
	* @param	エントリーポイント.
	* @param	.
	* @param	シェーダーブロブ(巨大バイナリ).
	*******************************************/
	HRESULT CompileShaderFromFile(
		const std::wstring& FilePath,
		LPCSTR EntryPoint,
		LPCSTR Target,
		ID3DBlob** ShaderBlob);

	/*******************************************
	* @brief	テクスチャ名からテクスチャバッファ作成、中身をコピーする.
	* @param	ファイルパス.
	* @param	リソースのポインタを返す.
	*******************************************/
	ID3D12Resource* CreateTextureFromFile(const char* Texpath);


private:
	HWND m_hWnd;	// ウィンドウハンドル.

	// DXGI.
	MyComPtr<IDXGIFactory6>					m_pDxgiFactory;			// ディスプレイに出力するためのAPI.
	MyComPtr<IDXGISwapChain4>				m_pSwapChain;			// スワップチェーン.

	// DirectX12.
	MyComPtr<ID3D12Device>					m_pDevice12;			// DirectX12のデバイスコンテキスト.
	MyComPtr<ID3D12CommandAllocator>		m_pCmdAllocator;		// コマンドアロケータ(命令をためておくメモリ領域).	
	MyComPtr<ID3D12GraphicsCommandList>		m_pCmdList;				// コマンドリスト.
	MyComPtr<ID3D12CommandQueue>			m_pCmdQueue;			// コマンドキュー.

	// レンダーターゲット.
	MyComPtr<ID3D12DescriptorHeap>			m_pRenderTargetViewHeap;// レンダーターゲットビュー.
	std::vector<MyComPtr<ID3D12Resource>>	m_pBackBuffer;			// バックバッファ.

	// 深度バッファ.
	MyComPtr<ID3D12Resource>				m_pDepthBuffer;			// 深度バッファ.
	MyComPtr<ID3D12DescriptorHeap>			m_pDepthHeap;			// 深度ステンシルビューのデスクリプタヒープ. 
	D3D12_CLEAR_VALUE						m_DepthClearValue;		// 深度のクリア値.

	// フェンス類.
	MyComPtr<ID3D12Fence>					m_pFence;				// 処理待ち柵.
	UINT64									m_FenceValue;			// 処理カウンター.

	// 描画周りの設定.
	MyComPtr<ID3D12PipelineState>			m_pPipelineState;		// 描画設定.
	MyComPtr<ID3D12RootSignature>			m_pRootSignature;		// ルートシグネチャ.
	std::unique_ptr<D3D12_VIEWPORT>			m_pViewport;			// ビューポート.
	std::unique_ptr<D3D12_RECT>				m_pScissorRect;			// シザー矩形.

	using LoadLambda_t = std::function<HRESULT(const std::wstring& Path, DirectX::TexMetadata*, DirectX::ScratchImage&)>;
	std::map<std::string, LoadLambda_t>		m_LoadLambdaTable;

	// ファイル名パスとリソースのマップテーブル.
	std::map<std::string, MyComPtr<ID3D12Resource>>	m_ResourceTable;

};
