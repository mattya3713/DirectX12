#pragma once

#include <d3d12.h>
#include <DirectXMath.h>
#include <map>
#include <memory>

#include "..\\..\\..\\Data\\Library\\DirectXTex\\Common\\d3dx12.h"

// 前方宣言.
class DirectX12;

/**********************************************************************************
* @date      : 2026-08-23.
* @brief     : Sprite2D(画面固定UI用)/Sprite3D(カメラ向きビルボード)描画基盤.
*            : 既存のMmdlRenderer等とは独立したPSO/ルートシグネチャを持つ。
*            : 呼び出しはMainScene::Draw内(BeginDraw〜EndDraw間)を想定し、
*            : 各Draw*()呼び出しがその場でコマンドを積む即時描画方式。
*            : 頂点はフレームごとのアップロードバッファ(FrameBufferCount分)へ書く.
**********************************************************************************/

class SpriteRenderer final
{
public:
	explicit SpriteRenderer(DirectX12& Dx12);
	~SpriteRenderer();

	SpriteRenderer(const SpriteRenderer&)            = delete;
	SpriteRenderer& operator=(const SpriteRenderer&) = delete;

	// 画面座標(ピクセル)にテクスチャ矩形を描画する(UI用. 深度テスト無し).
	void DrawSprite2D(
		ID3D12Resource* pTexture,
		float PosX, float PosY, float Width, float Height,
		const DirectX::XMFLOAT4& Color = { 1.0f, 1.0f, 1.0f, 1.0f });

	// ワールド座標にカメラ向きのビルボード矩形を描画する(深度テスト有り/書き込み無し).
	void DrawSprite3D(
		ID3D12Resource* pTexture,
		const DirectX::XMFLOAT3& Center, float Width, float Height,
		const DirectX::XMFLOAT4& Color = { 1.0f, 1.0f, 1.0f, 1.0f });

private:
	// 共通リソース(ルートシグネチャ・ヒープ・インデックスバッファ)の生成.
	void CreateCommonResources();
	// 種別ごとのパイプラインと頂点バッファを生成する.
	void CreatePipeline(bool Is3D);

	// テクスチャをSRVヒープへ登録し、GPUハンドルを返す(未登録のもののみ新規作成).
	D3D12_GPU_DESCRIPTOR_HANDLE RegisterTexture(ID3D12Resource* pTexture);

	// 種別共通の1枚分の描画コマンドを積む.
	void DrawQuad(bool Is3D, const void* pVertices, UINT VertexSize,
		ID3D12Resource* pTexture);

private:
	static constexpr UINT MAX_SPRITES = 256; // 1フレームあたりの最大スプライト数(種別ごと).

	DirectX12& m_Dx12;

	MyComPtr<ID3D12RootSignature>	m_pRootSignature;
	MyComPtr<ID3D12PipelineState>	m_pPipeline2D; // 深度テスト無し(UI用).
	MyComPtr<ID3D12PipelineState>	m_pPipeline3D; // 深度テスト有り/書き込み無し(ビルボード用).

	// フレームごとに二重化した動的頂点バッファ(GPU読み書き競合の回避).
	struct SpriteBuffer
	{
		MyComPtr<ID3D12Resource>          pVertexBuffer;
		D3D12_VERTEX_BUFFER_VIEW          View{};
		void*                             pMapped = nullptr;
	};
	SpriteBuffer	m_Buffer2D[2]; // FrameBufferCount分.
	SpriteBuffer	m_Buffer3D[2];

	MyComPtr<ID3D12Resource>			m_pIndexBuffer;   // 全スプライト共用の四角形インデックス.
	D3D12_INDEX_BUFFER_VIEW				m_IndexBufferView{};

	MyComPtr<ID3D12DescriptorHeap>		m_pSrvHeap;       // 先頭=SceneCBV(3D用), 以降=テクスチャSRV.
	UINT								m_SrvDescriptorSize = 0;
	UINT								m_NextSrvSlot       = 1;
	std::map<ID3D12Resource*, D3D12_GPU_DESCRIPTOR_HANDLE> m_TextureTable;
};
