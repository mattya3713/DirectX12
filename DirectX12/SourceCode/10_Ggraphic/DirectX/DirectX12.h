#pragma once

//警告についてのコード分析を無効にする.4005:再定義.
#pragma warning(disable:4005)

//ヘッダ読込.
#include <d3d12.h>
#include "..\\..\\..\\Data\\Library\\DirectXTex\\Common\\d3dx12.h"
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include "..\\..\\..\\Data\\Library\\DirectXTex\\DirectXTex\\DirectXTex.h"

#include <d3dcompiler.h>

//ライブラリ読み込み.
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "windowscodecs.lib") 
#pragma comment(lib, "DirectXTex.lib") 
#pragma comment(lib, "dxguid.lib") 

// モデルの頂点サイズ.
constexpr size_t PmdVertexSize = 38;

class ImGuiManager;

/**********************************************************
* @author      : 淵脇未来.
* @date        : 2025/02/18.
* @brief       : DirectX12セットアップ.
**********************************************************/

class DirectX12
{
public:

	// シェーダ側に投げられるマテリアルデータ.
	struct MaterialForHlsl {
		DirectX::XMFLOAT3	Diffuse;	// ディフューズ色.		
		float				Alpha;		// α値.		
		DirectX::XMFLOAT3	Specular;	// スペキュラの強.		
		float				Specularity;// スペキュラ色.		
		DirectX::XMFLOAT3	Ambient;	// アンビエント色.		

		MaterialForHlsl()
			: Diffuse		(0.0f, 0.0f, 0.0f)
			, Alpha			(0.0f)
			, Specular		(0.0f, 0.0f, 0.0f)
			, Specularity	(0.0f)
			, Ambient		(0.0f, 0.0f, 0.0f)
		{}
	};

	// それ以外のマテリアルデータ.
	struct AdditionalMaterial {
		std::string TexPath;	// テクスチャファイルパス.
		int			ToonIdx;	// トゥーン番号.
		bool		EdgeFlg;	// マテリアル毎の輪郭線フラグ.

		AdditionalMaterial()
			: TexPath		{}
			, ToonIdx		(0)
			, EdgeFlg		(false)
		{}
	};

	// まとめたもの.
	struct Material {
		unsigned int IndicesNum;		// インデックス数.
		MaterialForHlsl Materialhlsl;	// シェーダ側に投げられるマテリアルデータ.
		AdditionalMaterial Additional;	// それ以外のマテリアルデータ.
		
		Material()
			: IndicesNum	(0)
			, Materialhlsl	{}
			, Additional	{}
		{}
	};

	// TODO : 仮シーンデータ.
	struct SceneData {
		DirectX::XMMATRIX view;//ビュー行列
		DirectX::XMMATRIX proj;//プロジェクション行列
		DirectX::XMFLOAT3 eye;//視点座標
	};

public:
	DirectX12();
	~DirectX12();

	//DirectX12構築.
	bool Create(HWND hWnd);
	void Update();
	void UpdateSceneBuffer();

	// カメラ行列を設定する(呼び出し側でCameraBase派生クラスから取得して渡す).
	void SetCamera(const DirectX::XMMATRIX& View, const DirectX::XMMATRIX& Proj, const DirectX::XMFLOAT3& Eye);

	void BeginDraw();
	void EndDraw();

	// 3Dシーンをオフスクリーンへ描き終えた後、ImGui(Scene Viewパネル含む)を実際の
	// バックバッファへ描くための準備をする(SceneManager::Draw()の後、ImGuiManager::Render()の前に呼ぶ).
	void PrepareUIRenderTarget();

	// オフスクリーンのシーンカラーバッファを作成する(ImGuiManager::Init()成功後に1度だけ呼ぶ.
	// ImGuiのSRVヒープへ直接SRVを作成するため、初期化済みのImGuiManagerが必要).
	void CreateSceneColorTarget(ImGuiManager& ImGuiMgr);

	// スワップチェーン取得.
	const MyComPtr<IDXGISwapChain4> GetSwapChain();

	// DirextX12デバイス取得.
	const MyComPtr<ID3D12Device> GetDevice();

	// コマンドリスト取得.
	const MyComPtr<ID3D12GraphicsCommandList> GetCommandList();

	// テクスチャを取得.
	MyComPtr<ID3D12Resource> GetTextureByPath(const char* texpath);

	ID3D12Resource* GetSceneConstantBuffer() const { return m_pSceneConstBuff.Get(); }
	SceneData* GetMappedSceneData() const { return m_pMappedSceneData; }

	// GPUの完了待ち(全キューをflushする. 終了処理など、確実に同期したい箇所専用).
	void WaitForGPU();

private:// 作っていくんだよねぇ~.

	// バックバッファの数(スワップチェーンのBufferCountと一致させる).
	// コマンドアロケータをこの数だけ用意し、フレームごとに使い回すことで、
	// Present直後に毎回GPU完了を待つ必要をなくす(CPU/GPUのパイプライニング).
	static constexpr UINT FrameBufferCount = 2;

	// DXGIの生成.
	void CreateDXGIFactory(MyComPtr<IDXGIFactory6>& DxgiFactory);

	// コマンド類の生成.
	void CreateCommandObject(
		MyComPtr<ID3D12CommandAllocator>	(&CmdAllocators)[FrameBufferCount],
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
		MyComPtr<ID3D12DescriptorHeap>&	DepthHeap,
		MyComPtr<ID3D12DescriptorHeap>&	DepthSRVHeap);

	// シーンビューの作成.
	void CreateSceneDesc();

	// フェンスの作成.
	void CreateFance(MyComPtr<ID3D12Fence>& Fence);



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
	* @brief	テクスチャ名からテクスチャバッファ作成、中身をコピーする.
	* @param	ファイルパス.
	* @param	リソースのポインタを返す.
	*******************************************/
	MyComPtr<ID3D12Resource> CreateTextureFromFile(const char* Texpath);

	/*******************************************
	* @brief	 テクスチャロードテーブルの作成.
	*******************************************/
	void CreateTextureLoadTable();


private:
	HWND m_hWnd;	// ウィンドウハンドル.

	// DXGI.
	MyComPtr<IDXGIFactory6>					m_pDxgiFactory;			// ディスプレイに出力するためのAPI.
	MyComPtr<IDXGISwapChain4>				m_pSwapChain;			// スワップチェーン.
	DXGI_SWAP_CHAIN_DESC1                   m_SwapChainDesc;		// スワップチェーンのディスクリプション.

	// DirectX12.
	MyComPtr<ID3D12Device>					m_pDevice12;			// DirectX12のデバイスコンテキスト.
	MyComPtr<ID3D12CommandAllocator>		m_pCmdAllocators[FrameBufferCount]; // コマンドアロケータ(バックバッファごとに1つ. 命令をためておくメモリ領域).
	MyComPtr<ID3D12GraphicsCommandList>		m_pCmdList;				// コマンドリスト.
	MyComPtr<ID3D12CommandQueue>			m_pCmdQueue;			// コマンドキュー.
	UINT									m_FrameIndex;			// 現在描画中のバックバッファのインデックス(BeginDraw()で設定).

	// レンダーターゲット.
	MyComPtr<ID3D12DescriptorHeap>			m_pRenderTargetViewHeap;// レンダーターゲットビュー.
	std::vector<MyComPtr<ID3D12Resource>>	m_pBackBuffer;			// バックバッファ.

	// 深度バッファ.
	MyComPtr<ID3D12Resource>				m_pDepthBuffer;			// 深度バッファ.
	MyComPtr<ID3D12DescriptorHeap>			m_pDepthHeap;			// 深度ステンシルビュー.
	MyComPtr<ID3D12DescriptorHeap>			m_pDepthSRVHeap;		// 深度ステンシルビューのデスクリプタヒープ.
	D3D12_CLEAR_VALUE						m_DepthClearValue;		// 深度のクリア値.

	// オフスクリーンのシーンカラーバッファ(Scene Viewパネル表示用. 3DシーンはここへBeginDraw()で描く).
	MyComPtr<ID3D12Resource>				m_pSceneColorBuffer;
	MyComPtr<ID3D12DescriptorHeap>			m_pSceneColorRTVHeap;	// ↑専用のRTVヒープ(1ディスクリプタ).

	//シーンを構成するバッファまわり
	MyComPtr<ID3D12Resource>				m_pSceneConstBuff;		// シーン定数バッファのリソース
	SceneData*								m_pMappedSceneData;		// シーン定数バッファのCPU側マップ済みポインタ.

	// SetCamera()で設定される現在のカメラ行列.
	DirectX::XMMATRIX						m_ViewMatrix;
	DirectX::XMMATRIX						m_ProjMatrix;
	DirectX::XMFLOAT3						m_EyePosition;

	// フェンス類.
	MyComPtr<ID3D12Fence>					m_pFence;				// 処理待ち柵.
	UINT64									m_FenceValue;			// 処理カウンター.
	HANDLE									m_hFenceEvent;			// フェンスイベントハンドル.
	UINT64									m_FrameFenceValues[FrameBufferCount] {}; // 各アロケータスロットについて「ここまで完了していれば再利用してよい」フェンス値.

	// 描画周りの設定.
	MyComPtr<ID3D12PipelineState>			m_pPipelineState;		// パイプライン.
	MyComPtr<ID3D12RootSignature>			m_pRootSignature;		// ルートシグネチャ.
	std::unique_ptr<D3D12_VIEWPORT>			m_pViewport;			// ビューポート.
	std::unique_ptr<D3D12_RECT>				m_pScissorRect;			// シザー矩形.

	using LoadLambda_t = std::function<HRESULT(const std::wstring& Path, DirectX::TexMetadata*, DirectX::ScratchImage&)>;
	std::map<std::string, LoadLambda_t>		m_LoadLambdaTable;

	// ファイル名パスとリソースのマップテーブル.
	std::map<std::string, MyComPtr<ID3D12Resource>>	m_ResourceTable;

};
