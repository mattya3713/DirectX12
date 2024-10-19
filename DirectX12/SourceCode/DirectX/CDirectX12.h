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

using namespace DirectX;

/**************************************************
*	DirectX12セットアップ.
**/
class CDirectX12
{
public:

	// 頂点構造体.
	struct VerTex
	{
		XMFLOAT3 Pos;	// xyz座標.
		XMFLOAT2 uv;	// uv座標.
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
		XMFLOAT3 Pos;			// 頂点座標		: 12Byte.
		XMFLOAT3 Normal;        // 法線ベクトル	: 12Byte.
		XMFLOAT2 uv;            // uv座標		:  8Byte.
		uint16_t BoneNo[2];		// ボーン番号		:  4Byte.
		uint8_t  BoneWeight;    // ボーン影響度	:  1Byte.
		uint8_t  EdgeFlg;       // 輪郭線フラグ   :  1Byte.
		uint16_t Dummy;			// 
	}; // 38Byte.

public:
	CDirectX12();
	~CDirectX12();

	//DirectX12構築.
	bool Create(HWND hWnd);
	void UpDate();

	//デバイスコンテキストを取得.
	ID3D12Device* GetDevice() const { return m_pDevice12; }
	

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