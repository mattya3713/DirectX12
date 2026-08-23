#pragma once

//警告についてのコード分析を無効にする.4005:再定義.
#pragma warning(disable:4005)

//ヘッダ読込.
#include <cstdint>
#include <d3d12.h>
#include "d3dx12.h" // /IのData\Library\DirectXTex\Commonを参照(worktree等どんなチェックアウトでも解決する形式).
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
		float padding;//パディング(eyeとのアラインメント維持)
		DirectX::XMMATRIX lightView;//光源視点ビュー行列(シャドウマッピング用)
		DirectX::XMMATRIX lightProj;//光源視点正射影行列(シャドウマッピング用)
		DirectX::XMFLOAT4 lightDirection;//ライト方向(xyz:正規化済み進行方向, w:シャドウバイアス)
		DirectX::XMFLOAT4 lightColor;//ライト色(rgb:色, a:影適用フラグ(1.0で有効))
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

	// 平行光源を設定する(呼び出し側でDirectionLightから取得して渡す. ShadowEnableは影サンプリングのON/OFF).
	void SetLight(
		const DirectX::XMMATRIX& LightView,
		const DirectX::XMMATRIX& LightProj,
		const DirectX::XMFLOAT4& Direction,
		const DirectX::XMFLOAT4& Color);

	// UseOffscreenSceneがtrueならシーンカラーバッファへ、falseならバックバッファへ直接描画する.
	void BeginDraw(bool UseOffscreenScene);
	void EndDraw();

	// BeginDraw()で設定したレンダーターゲット・ビューポート・シザーを再設定する
	// (シャドウ深度パス等でOM/RS設定を上書きした後にメインパスへ復帰させる用途).
	void RestoreMainRenderTargets();

	// 3Dシーンをオフスクリーンへ描き終えた後、ImGui(Scene Viewパネル含む)を実際の
	// バックバッファへ描くための準備をする(SceneManager::Draw()の後、ImGuiManager::Render()の前に呼ぶ).
	void PrepareUIRenderTarget();

	// オフスクリーンのシーンカラーバッファを作成する(ImGuiManager::Init()成功後に1度だけ呼ぶ.
	// ImGuiのSRVヒープへ直接SRVを作成するため、初期化済みのImGuiManagerが必要).
	void CreateSceneColorTarget(ImGuiManager& ImGuiMgr);

	// Scene Viewパネルの現在サイズをリクエストする. 実際のリサイズはBeginDraw()の先頭で行う.
	void RequestSceneColorResize(UINT Width, UINT Height) noexcept;

	// 実ウィンドウのリサイズ時にスワップチェーンと関連する描画資源を作り直す.
	void OnWindowResize(UINT Width, UINT Height);

	// ===== 敗北時巻き戻り演出(FrameRewindへ委譲. 詳細はFrameRewind.h参照) =====

	// 毎フレームの描画結果を縮小リングバッファへ1枚保存する(MainScene::Drawの描画完了位置で呼ぶ).
	void CaptureForRewind();

	// 巻き戻り逆再生を開始する(保存フレームが無い場合はfalseで何もしない).
	bool StartRewindPlayback();

	// 逆再生を1フレーム分描画し、再生位置を古い方向へ進める(完了でIsRewindActive()==falseになる).
	void DrawRewindFrame();

	// 巻き戻り逆再生中か(呼び出し側は通常のシーン描画を止める).
	bool IsRewindActive() const;

	// リングバッファ保存・逆再生の実体.
	std::unique_ptr<class FrameRewind> m_upFrameRewind;

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

	// 現在描画中フレーム(m_FrameIndex)のバックバッファとインデックスを返す(巻き戻り演出等の特殊パス用).
	ID3D12Resource* GetBackBuffer(UINT Index) const
	{
		return (Index < m_pBackBuffer.size()) ? m_pBackBuffer[Index].Get() : nullptr;
	}
	UINT GetFrameIndex() const noexcept { return m_FrameIndex; }

	// バックバッファの解像度を返す(Sprite2D等の画面座標→NDC変換用).
	UINT GetBackBufferWidth() const noexcept { return m_SwapChainDesc.Width; }
	UINT GetBackBufferHeight() const noexcept { return m_SwapChainDesc.Height; }

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

	// 指定サイズでシーンカラーバッファとビューを再作成する.
	void ResizeSceneColorTarget(UINT Width, UINT Height);



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
	bool									m_bUseOffscreenScene = false; // BeginDraw()に渡された描画先モード(RestoreMainRenderTargets()用).

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
	ImGuiManager*							m_pImGuiManagerForSceneSrv;	// シーンテクスチャSRVの再作成先.
	UINT								m_SceneColorWidth;
	UINT								m_SceneColorHeight;
	bool								m_SceneColorResizeRequested;
	UINT								m_SceneColorRequestedWidth;
	UINT								m_SceneColorRequestedHeight;

	// シーンを構成するバッファまわり
	MyComPtr<ID3D12Resource>				m_pSceneConstBuff;		// シーン定数バッファのリソース
	SceneData*								m_pMappedSceneData;		// シーン定数バッファのCPU側マップ済みポインタ.

	// GPUタイムスタンプクエリ(簡易プロファイラ用).
	static constexpr UINT MaxGpuTimestamps = 16; // 1フレームあたりのタイムスタンプ数(開始/終了の組8個分).

public:
	// GPUタイムスタンプ計測が利用可能か(初期化成功後はtrue).
	bool IsGpuProfilingAvailable() const noexcept { return m_pGpuQueryHeap != nullptr; }

	// タイムスタンプの記録を積む(コマンドリスト記録中に呼ぶ. IndexInFrameは0〜MaxGpuTimestamps-1).
	void WriteGpuTimestamp(UINT IndexInFrame);

	// 記録したクエリ結果を読み取りバッファへ解決する(EndDraw内でClose前に呼ぶ).
	void ResolveGpuQueries();

	// 完了済みフレームのタイムスタンプ差分をミリ秒で取得する(開始/終了インデックスを指定).
	float ReadGpuMilliseconds(UINT StartIndexInFrame, UINT EndIndexInFrame) const;

private:
	// GPUタイムスタンプクエリ用のヒープと読み取りバッファを作成する.
	void CreateGpuQueryResources();

	// SetCamera()で設定される現在のカメラ行列.
	DirectX::XMMATRIX						m_ViewMatrix;
	DirectX::XMMATRIX						m_ProjMatrix;
	DirectX::XMFLOAT3						m_EyePosition;

	// SetLight()で設定される平行光源(UpdateSceneBuffer()でSceneDataへ書き込む).
	DirectX::XMMATRIX						m_LightViewMatrix;
	DirectX::XMMATRIX						m_LightProjMatrix;
	DirectX::XMFLOAT4						m_LightDirection;
	DirectX::XMFLOAT4						m_LightColor;

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
	std::unique_ptr<D3D12_VIEWPORT>			m_pSceneColorViewport;		// オフスクリーン用ビューポート.
	std::unique_ptr<D3D12_RECT>				m_pSceneColorScissorRect;		// オフスクリーン用シザー矩形.

	using LoadLambda_t = std::function<HRESULT(const std::wstring& Path, DirectX::TexMetadata*, DirectX::ScratchImage&)>;
	std::map<std::string, LoadLambda_t>		m_LoadLambdaTable;

	// ファイル名パスとリソースのマップテーブル.
	std::map<std::string, MyComPtr<ID3D12Resource>>	m_ResourceTable;

	// GPUタイムスタンプクエリ(簡易プロファイラ用).
	MyComPtr<ID3D12QueryHeap>	m_pGpuQueryHeap;     // タイムスタンプクエリヒープ(バックバッファ数xスロット分).
	MyComPtr<ID3D12Resource>	m_pGpuQueryReadback; // 解決結果の読み取りバッファ(マップ済み).
	std::uint64_t*				m_pMappedGpuQueries = nullptr; // マップ済み読み取りポインタ.
	std::uint64_t				m_GpuTimestampFrequency = 0;   // キューのタイムスタンプ周波数(Hz).

};
