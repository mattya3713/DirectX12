#include "stdafx.h"
#include "AsyncComputeDemo.h"


#include <d3dcompiler.h>

#include "10_Device/DirectX/DirectX12.h"
#include "99_Utility/ComPtr/ComPtr.h"
#include "99_Utility/Debug/Log/DebugLog.h"
#include "99_Utility/ServiceLocator/ServiceLocator.h"

#include "..\..\..\..\Data\Library\DirectXTex\Common\d3dx12.h"

namespace {

	constexpr UINT kElementCount    = 256;   // 検証するfloat要素数(64スレッドx4ディスパッチ).
	constexpr UINT kThreadsPerGroup = 64;
	constexpr UINT kDispatchGroups  = kElementCount / kThreadsPerGroup;
	constexpr UINT kIntervalFrames  = 180;  // 実行間隔(フレーム).

	const char* StateToText(bool InFlight)
	{
		return InFlight ? "in flight" : "idle";
	}

}

// DirectX12依存の実体(pimpl).
class AsyncComputeDemo::Impl
{
public:
	bool Initialize(ID3D12Device* pDevice);
	void Tick(DirectX12& Dx12);
	void Verify();

private:
	bool CreatePipeline(ID3D12Device* pDevice);
	bool CreateBuffersAndCommands(ID3D12Device* pDevice);
	bool Execute(DirectX12& Dx12);

	MyComPtr<ID3D12RootSignature>       m_pRootSignature;
	MyComPtr<ID3D12PipelineState>       m_pPipelineState;
	MyComPtr<ID3D12CommandAllocator>    m_pAllocator;
	MyComPtr<ID3D12GraphicsCommandList> m_pCommandList;

	MyComPtr<ID3D12Resource> m_pOutputBuffer;   // コンピュート書き込み先(UAV).
	MyComPtr<ID3D12Resource> m_pReadbackBuffer; // 読み戻し用(READBACK).

	HANDLE m_FenceEvent = nullptr;

	UINT   m_FrameCounter        = 0;
	bool   m_InFlight            = false;
	UINT64 m_PendingFenceValue   = 0;
	int    m_RunCount            = 0;
};

bool AsyncComputeDemo::Impl::Initialize(ID3D12Device* pDevice)
{
	if (!CreatePipeline(pDevice)) { return false; }
	if (!CreateBuffersAndCommands(pDevice)) { return false; }

	m_FenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	return m_FenceEvent != nullptr;
}

// 空ルートシグネチャ(UAV1つ)+コンピュートPSOを作成する.
bool AsyncComputeDemo::Impl::CreatePipeline(ID3D12Device* pDevice)
{
	CD3DX12_ROOT_PARAMETER root_param{};
	root_param.InitAsUnorderedAccessView(0);

	D3D12_ROOT_SIGNATURE_DESC root_desc{};
	root_desc.NumParameters = 1;
	root_desc.pParameters   = &root_param;
	root_desc.Flags         = D3D12_ROOT_SIGNATURE_FLAG_NONE; // コンピュートはIA入力不要.

	MyComPtr<ID3DBlob> sig_blob(nullptr);
	if (FAILED(D3D12SerializeRootSignature(&root_desc, D3D_ROOT_SIGNATURE_VERSION_1, sig_blob.GetAddressOf(), nullptr)))
	{
		return false;
	}
	if (FAILED(pDevice->CreateRootSignature(0, sig_blob->GetBufferPointer(), sig_blob->GetBufferSize(), IID_PPV_ARGS(m_pRootSignature.ReleaseAndGetAddressOf()))))
	{
		return false;
	}

#if defined(_DEBUG)
	constexpr UINT kCompileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	constexpr UINT kCompileFlags = 0;
#endif

	MyComPtr<ID3DBlob> cs_blob(nullptr);
	ID3DBlob* error_blob = nullptr;
	const HRESULT hr = D3DCompileFromFile(L"Data/Shader/Compute/TestCompute.hlsl", nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "cs_5_0", kCompileFlags, 0,
		cs_blob.GetAddressOf(), &error_blob);

	if (FAILED(hr))
	{
		if (DebugLog* p_debug_log = ServiceLocator::Get<DebugLog>()) {
			p_debug_log->LogError(std::string("TestCompute.hlsl compile failed: ")
				+ (error_blob ? static_cast<const char*>(error_blob->GetBufferPointer()) : "unknown"));
		}
		if (error_blob) { error_blob->Release(); }
		return false;
	}

	D3D12_COMPUTE_PIPELINE_STATE_DESC pso_desc{};
	pso_desc.pRootSignature = m_pRootSignature.Get();
	pso_desc.CS             = CD3DX12_SHADER_BYTECODE(cs_blob.Get());

	return SUCCEEDED(pDevice->CreateComputePipelineState(&pso_desc, IID_PPV_ARGS(m_pPipelineState.ReleaseAndGetAddressOf())));
}

// 出力UAV・読み戻しバッファ・COMPUTE型アロケータ/コマンドリストを作成する.
bool AsyncComputeDemo::Impl::CreateBuffersAndCommands(ID3D12Device* pDevice)
{
	const auto uav_props = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	const auto uav_desc  = CD3DX12_RESOURCE_DESC::Buffer(kElementCount * sizeof(float), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	if (FAILED(pDevice->CreateCommittedResource(&uav_props, D3D12_HEAP_FLAG_NONE, &uav_desc,
		D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(m_pOutputBuffer.ReleaseAndGetAddressOf()))))
	{
		return false;
	}

	const auto readback_props = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);
	const auto readback_desc  = CD3DX12_RESOURCE_DESC::Buffer(kElementCount * sizeof(float));
	if (FAILED(pDevice->CreateCommittedResource(&readback_props, D3D12_HEAP_FLAG_NONE, &readback_desc,
		D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(m_pReadbackBuffer.ReleaseAndGetAddressOf()))))
	{
		return false;
	}

	if (FAILED(pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE, IID_PPV_ARGS(m_pAllocator.ReleaseAndGetAddressOf()))))
	{
		return false;
	}

	return SUCCEEDED(pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COMPUTE,
		m_pAllocator.Get(), m_pPipelineState.Get(), IID_PPV_ARGS(m_pCommandList.ReleaseAndGetAddressOf())));
}

// コンピュートキューへ計算を記録・発行する.
bool AsyncComputeDemo::Impl::Execute(DirectX12& Dx12)
{
	ID3D12CommandQueue* p_compute_queue = Dx12.GetComputeQueue();
	ID3D12Fence*        p_fence         = Dx12.GetComputeFence();
	if (!p_compute_queue || !p_fence) { return false; }

	// 前回実行の完了を待ってからアロケータを再利用する(単一アロケータ運用のため).
	const UINT64 completed = p_fence->GetCompletedValue();
	if (m_PendingFenceValue > 0 && completed < m_PendingFenceValue && m_FenceEvent)
	{
		p_fence->SetEventOnCompletion(m_PendingFenceValue, m_FenceEvent);
		WaitForSingleObject(m_FenceEvent, INFINITE);
	}

	if (FAILED(m_pAllocator->Reset())) { return false; }
	if (FAILED(m_pCommandList->Reset(m_pAllocator.Get(), m_pPipelineState.Get()))) { return false; }

	m_pCommandList->SetComputeRootSignature(m_pRootSignature.Get());
	m_pCommandList->SetPipelineState(m_pPipelineState.Get());
	m_pCommandList->SetComputeRootUnorderedAccessView(0, m_pOutputBuffer->GetGPUVirtualAddress());

	// バリア: COMMON → UNORDERED_ACCESS.
	const D3D12_RESOURCE_BARRIER to_uav = CD3DX12_RESOURCE_BARRIER::Transition(
		m_pOutputBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	m_pCommandList->ResourceBarrier(1, &to_uav);

	m_pCommandList->Dispatch(kDispatchGroups, 1, 1);

	// UAVバリア(書き込み完了の保証).
	const D3D12_RESOURCE_BARRIER uav_barrier = CD3DX12_RESOURCE_BARRIER::UAV(m_pOutputBuffer.Get());
	m_pCommandList->ResourceBarrier(1, &uav_barrier);

	// バリア: UNORDERED_ACCESS → COPY_SOURCE(読み戻しのため).
	const D3D12_RESOURCE_BARRIER to_copy_src = CD3DX12_RESOURCE_BARRIER::Transition(
		m_pOutputBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
	m_pCommandList->ResourceBarrier(1, &to_copy_src);

	m_pCommandList->CopyResource(m_pReadbackBuffer.Get(), m_pOutputBuffer.Get());

	if (FAILED(m_pCommandList->Close())) { return false; }

	ID3D12CommandList* lists[] = { m_pCommandList.Get() };
	p_compute_queue->ExecuteCommandLists(1, lists);

	// コンピュートキューからフェンスをシグナルし、グラフィックスキューに完了待ちを挿入する
	// (キュー間同期: グラフィックス側が結果を使う場合はこの待ちの後に参照する).
	m_PendingFenceValue = Dx12.SignalComputeFence();
	Dx12.GraphicsWaitComputeFence(m_PendingFenceValue);

	return true;
}

// フェンス完了後に読み戻しバッファを検証し、結果をログへ出力する.
void AsyncComputeDemo::Impl::Verify()
{
	float* p_mapped = nullptr;
	if (FAILED(m_pReadbackBuffer->Map(0, nullptr, reinterpret_cast<void**>(&p_mapped))) || !p_mapped) { return; }

	int correct = 0;
	for (UINT i = 0; i < kElementCount; ++i)
	{
		const float expected = 1.0f + static_cast<float>(i) * 2.0f;
		if (p_mapped[i] == expected) { ++correct; }
	}

	m_pReadbackBuffer->Unmap(0, nullptr);
	++m_RunCount;

	if (DebugLog* p_debug_log = ServiceLocator::Get<DebugLog>())
	{
		const std::string message = (correct == static_cast<int>(kElementCount))
			? ("AsyncCompute verified: " + std::to_string(correct) + "/" + std::to_string(kElementCount)
				+ " correct (run #" + std::to_string(m_RunCount) + ")")
			: ("AsyncCompute MISMATCH: " + std::to_string(correct) + "/" + std::to_string(kElementCount));
		p_debug_log->LogInfo(message);
	}
}

AsyncComputeDemo::~AsyncComputeDemo() = default;

void AsyncComputeDemo::Impl::Tick(DirectX12& Dx12)
{
	++m_FrameCounter;

	if (m_InFlight)
	{
		// コンピュート完了(フェンス到達)後に読み戻して検証する.
		ID3D12Fence* p_fence = Dx12.GetComputeFence();
		if (!p_fence || m_PendingFenceValue == 0 || p_fence->GetCompletedValue() < m_PendingFenceValue) { return; }

		Verify();
		m_InFlight = false;
		return;
	}

	if (m_FrameCounter % kIntervalFrames != 0) { return; }
	if (!Execute(Dx12)) { return; }

	m_InFlight = true;
}

// ----- 外部インターフェース -----

bool AsyncComputeDemo::Initialize(struct ID3D12Device* pDevice, class DirectX12& Dx12)
{
	if (!m_Impl) { m_Impl = std::make_unique<Impl>(); }
	return m_Impl->Initialize(pDevice);
}

void AsyncComputeDemo::Tick(class DirectX12& Dx12)
{
	if (m_Impl) { m_Impl->Tick(Dx12); }
}
