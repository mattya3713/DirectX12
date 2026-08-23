#pragma once

#include <chrono>
#include <map>
#include <string>

/**********************************************************************************
* @author    : Coder(閃斬 Production Loop).
* @date      : 2026/08/23.
* @brief     : CPU/GPUフレームタイム計測(簡易プロファイラ).
*            : CPUはRAIIのScopedTimer、GPUはDirectX12が持つタイムスタンプクエリを
*            : 利用する。計測結果はProfiler::DrawImGui()の一覧パネルで確認できる.
**********************************************************************************/

class Profiler final
{
public:
	// CPU計測用のRAIIタイマ(スコープを抜けたら結果を記録する).
	class ScopedTimer final
	{
	public:
		explicit ScopedTimer(const char* pLabel);
		~ScopedTimer();

		ScopedTimer(const ScopedTimer&)            = delete;
		ScopedTimer& operator=(const ScopedTimer&) = delete;

	private:
		const char* m_pLabel;
		std::chrono::high_resolution_clock::time_point m_Start;
	};

	// シングルトン取得(ServiceLocatorには登録せず、計測専用として直接参照する).
	static Profiler& Instance();

	// フレームの先頭で呼ぶ(CPU計測結果をクリアし、GPUスロット割当をリセットする).
	void BeginFrame();

	// CPU計測結果を記録する(ScopedTimerから呼ぶ).
	void RecordCpuMilliseconds(const char* pLabel, float Milliseconds);

	// GPUスコープの開始/終了(DirectX12のタイムスタンプクエリを積む).
	// 同じラベルは同じスロットに対応づけられる.
	void GpuBegin(const char* pLabel);
	void GpuEnd(const char* pLabel);

	// 計測結果のImGui表示("Profiler"ウィンドウ).
	void DrawImGui();

private:
	Profiler() = default;
	~Profiler() = default;

	Profiler(const Profiler&)            = delete;
	Profiler& operator=(const Profiler&) = delete;
	Profiler(Profiler&&)                 = delete;
	Profiler& operator=(Profiler&&)      = delete;

private:
	std::map<std::string, float> m_CpuMilliseconds; // CPU計測結果(ラベル→ms).
};
