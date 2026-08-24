// Async Compute(コンピュートキュー/フェンス同期)単体テスト(スタンドアロン. ゲーム本体には含まれない).
// 実D3D12デバイス(NVIDIA優先→HW→WARPフォールバック)でウィンドウ無しに以下を検証する:
// 1. コンピュートキュー(D3D12_COMMAND_LIST_TYPE_COMPUTE)+専用フェンスの作成とCPUイベント到達
// 2. Data/Shader/Compute/TestCompute.hlslのDispatch結果が期待式(1+i*2)と256要素すべて一致
// 3. コンピュートSignal→グラフィックス(DIRECT)キューWait の順序保証付き同期読み取り
//    (本番DirectX12::GraphicsWaitComputeFenceと同じパターン)
// 4. 逆方向(グラフィックスSignal→コンピュートキューWait)も同様に完走する
// 5. 重いコンピュート実行中にDIRECT側ワークが先に完了する並行性プローブ(判定はせず計測ログのみ)

#include <windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h>

#include <chrono>
#include <cstdint>
#include <iostream>
#include <string>
#include <thread>

#include "../SourceCode/99_Utility/ComPtr/ComPtr.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

namespace {

	constexpr UINT kElementCount     = 256;
	constexpr UINT kThreadsPerGroup  = 64;
	constexpr UINT kDispatchGroups   = kElementCount / kThreadsPerGroup;
	constexpr UINT kHeavyDispatches  = 20000; // 並行性プローブ用のディスパッチ連投数.

	// TestCompute.hlslと同じ期待値式.
	float ExpectedValue(UINT Index)
	{
		return 1.0f + static_cast<float>(Index) * 2.0f;
	}

	std::string AdapterNameToAscii(const std::wstring& Name)
	{
		std::string ascii;
		for (wchar_t ch : Name)
		{
			ascii.push_back((ch >= 0x20 && ch < 0x7F) ? static_cast<char>(ch) : '?');
		}
		return ascii;
	}

	// フェンス値への到達をイベントで待つ(本番の待ちパターンと同じ).
	void WaitFence(ID3D12Fence* Fence, HANDLE Event, std::uint64_t Value)
	{
		if (Fence->GetCompletedValue() < Value)
		{
			Fence->SetEventOnCompletion(Value, Event);
			WaitForSingleObject(Event, INFINITE);
		}
	}

	ID3D12CommandQueue* CreateQueue(ID3D12Device* Device, D3D12_COMMAND_LIST_TYPE Type)
	{
		D3D12_COMMAND_QUEUE_DESC desc{};
		desc.Type     = Type;
		desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		desc.Flags    = D3D12_COMMAND_QUEUE_FLAG_NONE;
		desc.NodeMask = 0;

		MyComPtr<ID3D12CommandQueue> queue(nullptr);
		if (FAILED(Device->CreateCommandQueue(&desc, IID_PPV_ARGS(queue.ReleaseAndGetAddressOf()))))
		{
			return nullptr;
		}
		return queue.Detach();
	}

}

int main()
{
	int failures = 0;
	const auto check = [&failures](bool Condition, const char* pLabel) {
		std::cout << (Condition ? "[PASS] " : "[FAIL] ") << pLabel << std::endl;
		if (!Condition) { ++failures; }
	};

	using Clock = std::chrono::steady_clock;

	// ---- 0: デバッグレイヤー/ファクトリ/デバイス ----

	UINT dxgi_flags = 0;
	{
		MyComPtr<ID3D12Debug> debug(nullptr);
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(debug.ReleaseAndGetAddressOf()))))
		{
			debug.Get()->EnableDebugLayer();
			dxgi_flags = DXGI_CREATE_FACTORY_DEBUG;
		}
	}

	MyComPtr<IDXGIFactory4> factory(nullptr);
	if (FAILED(CreateDXGIFactory2(dxgi_flags, IID_PPV_ARGS(factory.ReleaseAndGetAddressOf()))))
	{
		std::cout << "[FAIL] 0. IDXGIFactory4生成に失敗(テスト続行不可)" << std::endl;
		return 1;
	}

	// 本番(FindAdapter("NVIDIA"))に合わせHWアダプタを選ぶ. 無ければWARPへフォールバック.
	MyComPtr<IDXGIAdapter1> chosen(nullptr);
	bool chosen_is_nvidia = false;
	for (UINT i = 0; factory->EnumAdapters1(i, chosen.ReleaseAndGetAddressOf()) != DXGI_ERROR_NOT_FOUND; ++i)
	{
		DXGI_ADAPTER_DESC1 desc{};
		if (FAILED(chosen.Get()->GetDesc1(&desc))) { continue; }
		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) { continue; }

		const std::wstring name = desc.Description;
		if (name.find(L"NVIDIA") != std::wstring::npos)
		{
			chosen_is_nvidia = true;
			break;
		}
	}

	const bool use_warp = (chosen.Get() == nullptr);
	if (use_warp)
	{
		factory->EnumWarpAdapter(IID_PPV_ARGS(chosen.ReleaseAndGetAddressOf()));
	}

	MyComPtr<ID3D12Device> device(nullptr);
	const HRESULT device_hr = D3D12CreateDevice(use_warp ? nullptr : chosen.Get(),
		D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(device.ReleaseAndGetAddressOf()));

	check(SUCCEEDED(device_hr) && device.Get() != nullptr, "0a. D3D12デバイス生成(NVIDIA優先/HW/WARPフォールバック)");

	if (FAILED(device_hr))
	{
		std::cout << "TESTS FAILED" << std::endl;
		return 1;
	}

	DXGI_ADAPTER_DESC1 chosen_desc{};
	if (!use_warp && SUCCEEDED(chosen.Get()->GetDesc1(&chosen_desc)))
	{
		std::cout << "[INFO] adapter=" << AdapterNameToAscii(chosen_desc.Description)
			<< " (WARP=" << (use_warp ? "yes" : "no") << ")" << std::endl;
	}
	else
	{
		std::cout << "[INFO] adapter=WARP software rasterizer" << std::endl;
	}
	std::cout << "[INFO] debug layer=" << ((dxgi_flags != 0) ? "on" : "off") << std::endl;

	HANDLE fence_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	check(fence_event != nullptr, "0b. 待ち受けイベント生成");

	// ---- 1: コンピュートキュー+専用フェンスの基本動作 ----

	MyComPtr<ID3D12CommandQueue> compute_queue(nullptr);
	MyComPtr<ID3D12CommandQueue> direct_queue(nullptr);
	MyComPtr<ID3D12Fence>        compute_fence(nullptr);
	MyComPtr<ID3D12Fence>        direct_fence(nullptr);

	compute_queue.Attach(CreateQueue(device.Get(), D3D12_COMMAND_LIST_TYPE_COMPUTE));
	direct_queue.Attach(CreateQueue(device.Get(), D3D12_COMMAND_LIST_TYPE_DIRECT));

	check(compute_queue.Get() != nullptr && direct_queue.Get() != nullptr,
		"1a. コンピュートキューとグラフィックスキューの作成");

	check(SUCCEEDED(device.Get()->CreateFence(0, D3D12_FENCE_FLAG_NONE,
			IID_PPV_ARGS(compute_fence.ReleaseAndGetAddressOf()))) &&
		SUCCEEDED(device.Get()->CreateFence(0, D3D12_FENCE_FLAG_NONE,
			IID_PPV_ARGS(direct_fence.ReleaseAndGetAddressOf()))),
		"1b. 専用フェンス2本(コンピュート用/グラフィックス用)の作成");

	{
		MyComPtr<ID3D12CommandAllocator> alloc(nullptr);
		MyComPtr<ID3D12GraphicsCommandList> list(nullptr);
		device.Get()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE,
			IID_PPV_ARGS(alloc.ReleaseAndGetAddressOf()));
		device.Get()->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COMPUTE, alloc.Get(), nullptr,
			IID_PPV_ARGS(list.ReleaseAndGetAddressOf()));
		list.Get()->Close();

		ID3D12CommandList* lists[] = { list.Get() };
		compute_queue.Get()->ExecuteCommandLists(1, lists);
		compute_fence.Get()->SetEventOnCompletion(1, fence_event);
		compute_queue.Get()->Signal(compute_fence.Get(), 1);
		WaitForSingleObject(fence_event, INFINITE);

		check(compute_fence.Get()->GetCompletedValue() >= 1,
			"1c. コンピュートキューのシグナルがCPUイベントで観測される");
	}

	// ---- 2/3: TestCompute.hlslの実行とDIRECTキューWait経由の読み取り ----

	MyComPtr<ID3DBlob> cs_blob(nullptr);
	constexpr UINT kCompileFlags =
#if defined(_DEBUG)
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
		0;
#endif
	check(SUCCEEDED(D3DCompileFromFile(L"Data/Shader/Compute/TestCompute.hlsl", nullptr,
			D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "cs_5_0", kCompileFlags, 0,
			cs_blob.GetAddressOf(), nullptr)),
		"2a. TestCompute.hlslのコンパイル(cs_5_0)");

	MyComPtr<ID3D12RootSignature> root_sig(nullptr);
	{
		D3D12_ROOT_PARAMETER root_param{};
		root_param.ParameterType             = D3D12_ROOT_PARAMETER_TYPE_UAV;
		root_param.Descriptor.ShaderRegister = 0;

		D3D12_ROOT_SIGNATURE_DESC root_desc{};
		root_desc.NumParameters = 1;
		root_desc.pParameters   = &root_param;
		root_desc.Flags         = D3D12_ROOT_SIGNATURE_FLAG_NONE;

		MyComPtr<ID3DBlob> sig_blob(nullptr);
		if (SUCCEEDED(D3D12SerializeRootSignature(&root_desc, D3D_ROOT_SIGNATURE_VERSION_1,
				sig_blob.GetAddressOf(), nullptr)))
		{
			device.Get()->CreateRootSignature(0, sig_blob.Get()->GetBufferPointer(),
				sig_blob.Get()->GetBufferSize(), IID_PPV_ARGS(root_sig.ReleaseAndGetAddressOf()));
		}
	}
	check(root_sig.Get() != nullptr, "2b. 空ルートシグネチャ(UAV0)の作成");

	MyComPtr<ID3D12PipelineState> pso(nullptr);
	{
		D3D12_COMPUTE_PIPELINE_STATE_DESC pso_desc{};
		pso_desc.pRootSignature = root_sig.Get();
		pso_desc.CS             = { cs_blob.Get()->GetBufferPointer(), cs_blob.Get()->GetBufferSize() };
		device.Get()->CreateComputePipelineState(&pso_desc, IID_PPV_ARGS(pso.ReleaseAndGetAddressOf()));
	}
	check(pso.Get() != nullptr, "2c. コンピュートPSOの作成");

	MyComPtr<ID3D12Resource> output_buffer(nullptr);
	MyComPtr<ID3D12Resource> readback_buffer(nullptr);
	{
		D3D12_HEAP_PROPERTIES props{};
		props.Type = D3D12_HEAP_TYPE_DEFAULT;

		D3D12_RESOURCE_DESC desc{};
		desc.Dimension       = D3D12_RESOURCE_DIMENSION_BUFFER;
		desc.Width           = kElementCount * sizeof(float);
		desc.Height          = 1;
		desc.DepthOrArraySize = 1;
		desc.MipLevels       = 1;
		desc.SampleDesc.Count = 1;
		desc.Layout          = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		desc.Flags           = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

		check(SUCCEEDED(device.Get()->CreateCommittedResource(&props, D3D12_HEAP_FLAG_NONE, &desc,
				D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(output_buffer.ReleaseAndGetAddressOf()))),
			"2d. 出力UAVバッファの作成");

		desc.Flags = D3D12_RESOURCE_FLAG_NONE;
		props.Type = D3D12_HEAP_TYPE_READBACK;

		check(SUCCEEDED(device.Get()->CreateCommittedResource(&props, D3D12_HEAP_FLAG_NONE, &desc,
				D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(readback_buffer.ReleaseAndGetAddressOf()))),
			"2e. 読み戻しバッファの作成");
	}

	{
		MyComPtr<ID3D12CommandAllocator> alloc(nullptr);
		MyComPtr<ID3D12GraphicsCommandList> list(nullptr);
		check(SUCCEEDED(device.Get()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE,
				IID_PPV_ARGS(alloc.ReleaseAndGetAddressOf()))) &&
			SUCCEEDED(device.Get()->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COMPUTE,
				alloc.Get(), pso.Get(), IID_PPV_ARGS(list.ReleaseAndGetAddressOf()))),
			"2f. コンピュート専用アロケータ/コマンドリストの作成");

		D3D12_RESOURCE_BARRIER to_uav = {};
		to_uav.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		to_uav.Transition.pResource   = output_buffer.Get();
		to_uav.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
		to_uav.Transition.StateAfter  = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

		D3D12_RESOURCE_BARRIER uav_barrier = {};
		uav_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;

		D3D12_RESOURCE_BARRIER to_copy_src = {};
		to_copy_src.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		to_copy_src.Transition.pResource   = output_buffer.Get();
		to_copy_src.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		to_copy_src.Transition.StateAfter  = D3D12_RESOURCE_STATE_COPY_SOURCE;

		list.Get()->SetComputeRootSignature(root_sig.Get());
		list.Get()->SetPipelineState(pso.Get());
		list.Get()->SetComputeRootUnorderedAccessView(0, output_buffer.Get()->GetGPUVirtualAddress());
		list.Get()->ResourceBarrier(1, &to_uav);
		list.Get()->Dispatch(kDispatchGroups, 1, 1);
		list.Get()->ResourceBarrier(1, &uav_barrier);
		list.Get()->ResourceBarrier(1, &to_copy_src);
		list.Get()->CopyResource(readback_buffer.Get(), output_buffer.Get());
		check(SUCCEEDED(list.Get()->Close()), "3a. コンピュートコマンドの記録とClose");

		ID3D12CommandList* lists[] = { list.Get() };
		compute_queue.Get()->ExecuteCommandLists(1, lists);
		check(SUCCEEDED(compute_queue.Get()->Signal(compute_fence.Get(), 2)), "3b. コンピュート完了フェンスのシグナル");

		// 本番DirectX12::GraphicsWaitComputeFenceと同じ: DIRECTキューにWaitを挿入してから実行する.
		direct_queue.Get()->Wait(compute_fence.Get(), 2);
		{
			MyComPtr<ID3D12CommandAllocator> d_alloc(nullptr);
			MyComPtr<ID3D12GraphicsCommandList> d_list(nullptr);
			device.Get()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
				IID_PPV_ARGS(d_alloc.ReleaseAndGetAddressOf()));
			device.Get()->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, d_alloc.Get(), nullptr,
				IID_PPV_ARGS(d_list.ReleaseAndGetAddressOf()));
			d_list.Get()->Close();

			ID3D12CommandList* d_lists[] = { d_list.Get() };
			direct_queue.Get()->ExecuteCommandLists(1, d_lists);
			direct_queue.Get()->Signal(direct_fence.Get(), 1);
		}
		WaitFence(direct_fence.Get(), fence_event, 1);
		check(direct_fence.Get()->GetCompletedValue() >= 1,
			"3c. DIRECTキューのWait(コンピュート完了待ち)挿入下でワークが完走");

		float* p_mapped = nullptr;
		int correct = -1;
		if (SUCCEEDED(readback_buffer.Get()->Map(0, nullptr, reinterpret_cast<void**>(&p_mapped))) && p_mapped)
		{
			correct = 0;
			for (UINT i = 0; i < kElementCount; ++i)
			{
				if (p_mapped[i] == ExpectedValue(i)) { ++correct; }
			}
			readback_buffer.Get()->Unmap(0, nullptr);
		}
		check(correct == static_cast<int>(kElementCount),
			"3d. Dispatch結果が期待式どおり(256/256一致. フェンス同期後に読むため競合なし)");
	}

	// ---- 4: 逆方向(グラフィックスSignal→コンピュートWait) ----

	{
		MyComPtr<ID3D12CommandAllocator> alloc(nullptr);
		MyComPtr<ID3D12GraphicsCommandList> list(nullptr);
		device.Get()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE,
			IID_PPV_ARGS(alloc.ReleaseAndGetAddressOf()));
		device.Get()->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COMPUTE, alloc.Get(), pso.Get(),
			IID_PPV_ARGS(list.ReleaseAndGetAddressOf()));
		list.Get()->Close();

		direct_fence.Get()->SetEventOnCompletion(2, fence_event);
		direct_queue.Get()->Signal(direct_fence.Get(), 2);
		WaitForSingleObject(fence_event, INFINITE);

		compute_queue.Get()->Wait(direct_fence.Get(), 2); // グラフィックス成果をコンピュート側が参照する前の待ち.
		ID3D12CommandList* lists[] = { list.Get() };
		compute_queue.Get()->ExecuteCommandLists(1, lists);
		compute_queue.Get()->Signal(compute_fence.Get(), 3);
		WaitFence(compute_fence.Get(), fence_event, 3);

		check(compute_fence.Get()->GetCompletedValue() >= 3,
			"4a. 逆方向Wait(グラフィックスSignal→コンピュート側待ち)でもECLが完走する");
	}

	// ---- 5: 並行性プローブ(重いコンピュート中にDIRECT側が先に完了するか. 判定はせずログのみ) ----

	{
		// 前節のコンピュートワーク完了(fence_c>=3)済みなのでアロケータを安全に再利用できる.
		MyComPtr<ID3D12CommandAllocator> alloc(nullptr);
		MyComPtr<ID3D12GraphicsCommandList> list(nullptr);
		device.Get()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE,
			IID_PPV_ARGS(alloc.ReleaseAndGetAddressOf()));
		device.Get()->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COMPUTE, alloc.Get(), pso.Get(),
			IID_PPV_ARGS(list.ReleaseAndGetAddressOf()));

		D3D12_RESOURCE_BARRIER to_uav = {};
		to_uav.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		to_uav.Transition.pResource   = output_buffer.Get();
		to_uav.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
		to_uav.Transition.StateAfter  = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

		list.Get()->SetComputeRootSignature(root_sig.Get());
		list.Get()->SetPipelineState(pso.Get());
		list.Get()->SetComputeRootUnorderedAccessView(0, output_buffer.Get()->GetGPUVirtualAddress());
		list.Get()->ResourceBarrier(1, &to_uav);
		for (UINT i = 0; i < kHeavyDispatches; ++i)
		{
			list.Get()->Dispatch(kDispatchGroups, 1, 1);
		}
		list.Get()->Close();

		MyComPtr<ID3D12CommandAllocator> d_alloc(nullptr);
		MyComPtr<ID3D12GraphicsCommandList> d_list(nullptr);
		device.Get()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
			IID_PPV_ARGS(d_alloc.ReleaseAndGetAddressOf()));
		device.Get()->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, d_alloc.Get(), nullptr,
			IID_PPV_ARGS(d_list.ReleaseAndGetAddressOf()));
		d_list.Get()->Close();

		const auto submit_time = Clock::now();

		ID3D12CommandList* lists[] = { list.Get() };
		compute_queue.Get()->ExecuteCommandLists(1, lists);
		compute_queue.Get()->Signal(compute_fence.Get(), 4);

		ID3D12CommandList* d_lists[] = { d_list.Get() };
		direct_queue.Get()->ExecuteCommandLists(1, d_lists);
		direct_queue.Get()->Signal(direct_fence.Get(), 3);

		double compute_ms = -1.0;
		double direct_ms  = -1.0;
		while ((compute_ms < 0.0 || direct_ms < 0.0))
		{
			if (direct_ms < 0.0 && direct_fence.Get()->GetCompletedValue() >= 3)
			{
				direct_ms = std::chrono::duration<double, std::milli>(Clock::now() - submit_time).count();
			}
			if (compute_ms < 0.0 && compute_fence.Get()->GetCompletedValue() >= 4)
			{
				compute_ms = std::chrono::duration<double, std::milli>(Clock::now() - submit_time).count();
			}
			if (compute_ms < 0.0 || direct_ms < 0.0) { std::this_thread::sleep_for(std::chrono::milliseconds(1)); }
		}

		const bool direct_first = direct_ms < compute_ms;
		std::cout << "[INFO] probe: compute=" << compute_ms << "ms direct=" << direct_ms << "ms"
			<< " direct_finished_while_compute_inflight=" << (direct_first ? "yes" : "no")
			<< " (WARPでは直列化されるためyesはHWアダプタでのみ期待値)" << std::endl;
		check(true, "5a. 並行性プローブ完走(上記INFOの計測値が重複実行の証跡ログ)");
	}

	if (fence_event != nullptr) { CloseHandle(fence_event); }

	std::cout << (failures == 0 ? "ALL TESTS PASSED" : "TESTS FAILED") << std::endl;
	return failures == 0 ? 0 : 1;
}
