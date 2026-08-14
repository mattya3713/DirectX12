#pragma once

#include <d3d12.h>
#include <DirectXMath.h>

#include "99_Utility/ComPtr/ComPtr.h"

class DirectX12;

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/14.
* @brief     : DCCツール風の参照用グリッドを描画する_DEBUG専用クラス.
**********************************************************************************/

class DebugGrid final
{
public:
	explicit DebugGrid(DirectX12& Dx12);
	~DebugGrid();

	DebugGrid(const DebugGrid&)            = delete;
	DebugGrid& operator=(const DebugGrid&) = delete;
	DebugGrid(DebugGrid&&)                 = delete;
	DebugGrid& operator=(DebugGrid&&)      = delete;

	// 現在のアクティブカメラのView-Projectionでグリッドを描画する.
	void Draw();

private:
	void CreatePipeline();
	void CreateGridVertices();

private:
	DirectX12& m_Dx12;

	MyComPtr<ID3D12RootSignature> m_pRootSignature;
	MyComPtr<ID3D12PipelineState> m_pPipelineState;

	MyComPtr<ID3D12Resource> m_pVertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView{};
	UINT m_VertexCount = 0;

	MyComPtr<ID3D12Resource> m_pConstantBuffer;
	DirectX::XMMATRIX* m_pMappedConstantBuffer = nullptr;
};
