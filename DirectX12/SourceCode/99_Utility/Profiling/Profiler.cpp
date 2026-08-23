#include "Profiler.h"

#include <vector>

#include "ImGui/imgui.h"
#include "10_Ggraphic/10_Device/DirectX/DirectX12.h"
#include "99_Utility/ServiceLocator/ServiceLocator.h"

namespace {
	// GPUスコープの割当(ラベル→タイムスタンプ開始インデックス).
	// インデックスは開始/終了の2つ組で割り当てる(0-1, 2-3, ...).
	struct GpuScope
	{
		std::string Label;
		UINT        StartIndex = 0;
		UINT        EndIndex   = 0;
	};

	std::map<std::string, UINT>& GpuSlotTable()
	{
		static std::map<std::string, UINT> table;
		return table;
	}

	std::vector<GpuScope>& GpuScopes()
	{
		static std::vector<GpuScope> scopes;
		return scopes;
	}

	constexpr UINT kMaxGpuPairs = 8; // DirectX12::MaxGpuTimestamps / 2.
}

Profiler& Profiler::Instance()
{
	static Profiler instance;
	return instance;
}

void Profiler::BeginFrame()
{
	m_CpuMilliseconds.clear();
}

void Profiler::RecordCpuMilliseconds(const char* pLabel, float Milliseconds)
{
	if (!pLabel) { return; }
	m_CpuMilliseconds[pLabel] = Milliseconds;
}

void Profiler::GpuBegin(const char* pLabel)
{
	DirectX12* p_dx12 = ServiceLocator::Get<DirectX12>();
	if (!pLabel || !p_dx12 || !p_dx12->IsGpuProfilingAvailable()) { return; }

	auto& table = GpuSlotTable();
	const auto it = table.find(pLabel);
	UINT start_index = 0;
	if (it != table.end())
	{
		start_index = it->second * 2u;
	}
	else
	{
		auto& scopes = GpuScopes();
		if (scopes.size() >= kMaxGpuPairs) { return; } // スロット使い切り.
		start_index = static_cast<UINT>(scopes.size()) * 2u;
		table.emplace(pLabel, static_cast<UINT>(scopes.size()));
		scopes.push_back({ pLabel, start_index, start_index + 1 });
	}

	p_dx12->WriteGpuTimestamp(start_index);
}

void Profiler::GpuEnd(const char* pLabel)
{
	DirectX12* p_dx12 = ServiceLocator::Get<DirectX12>();
	if (!pLabel || !p_dx12 || !p_dx12->IsGpuProfilingAvailable()) { return; }

	const auto& table = GpuSlotTable();
	const auto it = table.find(pLabel);
	if (it == table.end()) { return; }

	p_dx12->WriteGpuTimestamp(it->second * 2u + 1u);
}

void Profiler::DrawImGui()
{
	if (!ImGui::Begin("Profiler")) { ImGui::End(); return; }

	if (ImGui::CollapsingHeader("CPU", ImGuiTreeNodeFlags_DefaultOpen))
	{
		for (const auto& [label, ms] : m_CpuMilliseconds)
		{
			ImGui::Text("%-16s %8.3f ms", label.c_str(), ms);
		}
	}

	DirectX12* p_dx12 = ServiceLocator::Get<DirectX12>();
	if (p_dx12 && p_dx12->IsGpuProfilingAvailable() && ImGui::CollapsingHeader("GPU", ImGuiTreeNodeFlags_DefaultOpen))
	{
		for (const GpuScope& scope : GpuScopes())
		{
			const float gpu_ms = p_dx12->ReadGpuMilliseconds(scope.StartIndex, scope.EndIndex);
			if (gpu_ms >= 0.0f) {
				ImGui::Text("%-16s %8.3f ms", scope.Label.c_str(), gpu_ms);
			}
			else {
				ImGui::Text("%-16s %s", scope.Label.c_str(), "(計測中)");
			}
		}
	}

	ImGui::End();
}

// ===== ScopedTimer =====

Profiler::ScopedTimer::ScopedTimer(const char* pLabel)
	: m_pLabel(pLabel)
	, m_Start(std::chrono::high_resolution_clock::now())
{
}

Profiler::ScopedTimer::~ScopedTimer()
{
	const auto end = std::chrono::high_resolution_clock::now();
	const float milliseconds = std::chrono::duration<float, std::milli>(end - m_Start).count();
	Instance().RecordCpuMilliseconds(m_pLabel, milliseconds);
}
