#pragma once

#include <d3d12.h>
#include <DirectXMath.h>

#include "99_Utility/ComPtr/ComPtr.h"

class DirectX12;

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/22.
* @brief     : カプセル型当たり判定をワイヤーフレームで描画する_DEBUG専用クラス.
*            : コライダー数が少ないため、DebugGridと違いバッチングはせず
*            : DrawCapsule()呼び出しごとに1回Draw Callを発行する.
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

	// カプセルコライダー1本分をワイヤーフレームで即座に描画する(SegStart/SegEndはワールド座標、Colorは0〜1).
	void DrawCapsule(const DirectX::XMFLOAT3& SegStart, const DirectX::XMFLOAT3& SegEnd, float Radius, const DirectX::XMFLOAT3& Color);

private:
	void CreatePipeline();

private:
	DirectX12& m_Dx12;

	MyComPtr<ID3D12RootSignature> m_pRootSignature;
	MyComPtr<ID3D12PipelineState> m_pPipelineState;

	MyComPtr<ID3D12Resource> m_pVertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView{};
	void* m_pMappedVertexBuffer = nullptr;

	MyComPtr<ID3D12Resource> m_pConstantBuffer;
	DirectX::XMMATRIX* m_pMappedConstantBuffer = nullptr;
};
