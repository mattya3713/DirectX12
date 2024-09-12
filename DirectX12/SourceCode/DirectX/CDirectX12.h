#pragma once

//警告についてのコード分析を無効にする.4005:再定義.
#pragma warning(disable:4005)

//ヘッダ読込.
#include <D3D12.h>
#include <d3dx12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>

// XXX : 昔のヘッダーがincludeされてしまうため直パス.
#include "C:\Program Files (x86)\Windows Kits\10\Include\10.0.22000.0\um\d3dcompiler.h"	

//ライブラリ読み込み.
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")



using namespace DirectX;

/**************************************************
*	DirectX12セットアップ.
**/
class CDirectX12
{
public:

	struct VerTex
	{
		XMFLOAT3 Pos;	// xyz座標.
		XMFLOAT2 uv;	// xuv座標.
	};

public:
	CDirectX12();
	~CDirectX12();

	//DirectX12構築.
	bool Create(HWND hWnd);
	void UpDate();

	//デバイスを取得.
	ID3D12Device* GetDevice() const { return m_pDevice12; }
	//デバイスコンテキストを取得.
	//ID3D11DeviceContext* GetContext() const { return m_pContext11; }

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
	* @param	検索する文字列.
	*******************************************/
	void ShaderCompileError(const HRESULT& Result, ID3DBlob* ErrorMsg);

private:
	ID3D12Device*		m_pDevice12;	// DirectX12のデバイスコンテキスト.
	IDXGIFactory6*		m_pDxgiFactory;	// ディスプレイに出力するためのAPI.
	IDXGISwapChain4*	m_pSwapChain;	// スワップチェーン.

	ID3D12CommandAllocator*		m_pCmdAllocator;// コマンドアロケータ(命令をためておくメモリ領域).	
	ID3D12GraphicsCommandList*	m_pCmdList;		// コマンドリスト.
	ID3D12CommandQueue*			m_pCmdQueue;	// コマンドキュー.

	XMFLOAT3					m_Vertex[3];		// 頂点.
};