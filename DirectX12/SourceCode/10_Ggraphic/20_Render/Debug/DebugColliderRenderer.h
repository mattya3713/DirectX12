#pragma once

#include <d3d12.h>
#include <DirectXMath.h>
#include <vector>

#include "99_Utility/ComPtr/ComPtr.h"

class DirectX12;

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/22.
* @brief     : カプセル型当たり判定をワイヤーフレームで描画する_DEBUG専用クラス.
*            : 各オブジェクトはRegisterCapsule()でコライダー情報を登録するだけ.
*            : 実際の描画(RootSignature/PSO切替・DrawInstanced)はDraw()が
*            : フレーム末尾の1箇所でまとめて行う(登録キュー方式).
*            : 頂点バッファは全カプセル分を1枚にまとめて積むため、
*            : 呼び出しごとのバッファ上書きで描画が消える問題も構造的に起きない.
**********************************************************************************/

class DebugColliderRenderer final
{
public:
	explicit DebugColliderRenderer(DirectX12& Dx12);
	~DebugColliderRenderer();

	DebugColliderRenderer(const DebugColliderRenderer&)            = delete;
	DebugColliderRenderer& operator=(const DebugColliderRenderer&) = delete;
	DebugColliderRenderer(DebugColliderRenderer&&)                 = delete;
	DebugColliderRenderer& operator=(DebugColliderRenderer&&)      = delete;

	// カプセルコライダー1本分を描画キューへ登録する(SegStart/SegEndはワールド座標、Colorは0〜1).
	// 実際の描画は行わない(フレーム末尾のDraw()でまとめて描かれる).
	void RegisterCapsule(const DirectX::XMFLOAT3& SegStart, const DirectX::XMFLOAT3& SegEnd, float Radius, const DirectX::XMFLOAT3& Color);

	// 登録済みの全カプセルをまとめて描画し、キューをクリアする(各シーンのDraw末尾で1回呼ぶ).
	void Draw();

private:
	void CreatePipeline();

private:
	// 描画待ちカプセル1本分の情報(RegisterCapsule()の引数をそのまま保持する).
	struct CapsuleRequest
	{
		DirectX::XMFLOAT3 SegStart;
		DirectX::XMFLOAT3 SegEnd;
		float             Radius;
		DirectX::XMFLOAT3 Color;
	};

	DirectX12& m_Dx12;

	MyComPtr<ID3D12RootSignature> m_pRootSignature;
	MyComPtr<ID3D12PipelineState> m_pPipelineState;

	std::vector<CapsuleRequest> m_PendingCapsules; // 今フレームに登録されたカプセル(Draw()で消費してクリア).

	MyComPtr<ID3D12Resource> m_pVertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView{};
	void* m_pMappedVertexBuffer = nullptr;

	MyComPtr<ID3D12Resource> m_pConstantBuffer;
	DirectX::XMMATRIX* m_pMappedConstantBuffer = nullptr;
};
