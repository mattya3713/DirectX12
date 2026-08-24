// Standalone verification for RenderBatchOrder (multithread command recording acceptance criteria).
#include <cassert>
#include <cstdio>
#include <vector>

#include "10_Ggraphic/10_Device/DirectX/RenderBatchOrder.h"

namespace {

	constexpr uint32_t MAIN_FIRST    = RenderBatchOrder::MainFirst;
	constexpr uint32_t MAIN_DEFERRED = RenderBatchOrder::MainDeferred;
	constexpr uint32_t MAIN_FINAL    = RenderBatchOrder::MainFinal;
	constexpr uint32_t PARALLEL_BASE = RenderBatchOrder::ParallelBase;

}

int main()
{
	int passed = 0;
	int failed = 0;

	const auto expect = [&](bool Condition, const char* pWhat)
	{
		if (Condition) {
			++passed;
			std::printf("PASS: %s\n", pWhat);
		}
		else {
			++failed;
			std::printf("FAIL: %s\n", pWhat);
		}
	};

	const auto ids_equal = [](const std::vector<uint32_t>& Order, std::initializer_list<uint32_t> Expected) -> bool
	{
		if (Order.size() != Expected.size()) { return false; }
		size_t i = 0;
		for (uint32_t id : Expected) {
			if (Order[i] != id) { return false; }
			++i;
		}
		return true;
	};

	// --- 全スロットClose済み(通常のDebugフレーム): 前半→Player→Boss→後半→コライダー→終端 ---
	{
		const bool closed[3] = { true, true, true };
		const std::vector<uint32_t> order = RenderBatchOrder::Build(closed, 3);
		expect(ids_equal(order, { MAIN_FIRST, PARALLEL_BASE + 0, PARALLEL_BASE + 1, MAIN_DEFERRED, PARALLEL_BASE + 2, MAIN_FINAL }),
			"all slots closed yields interleaved order with last slot before final list");
	}

	// --- 誰も並列記録していないフレーム: メイン3本のみ ---
	{
		const bool closed[3] = { false, false, false };
		const std::vector<uint32_t> order = RenderBatchOrder::Build(closed, 3);
		expect(ids_equal(order, { MAIN_FIRST, MAIN_DEFERRED, MAIN_FINAL }),
			"no closed slots yields main-only batch");
	}

	// --- スロット数が2でも最終スロットは後半リストの後ろへ配置される ---
	{
		const bool closed[2] = { true, true };
		const std::vector<uint32_t> order = RenderBatchOrder::Build(closed, 2);
		expect(ids_equal(order, { MAIN_FIRST, PARALLEL_BASE + 0, MAIN_DEFERRED, PARALLEL_BASE + 1, MAIN_FINAL }),
			"with two configured slots the last one still executes after the deferred list");
	}

	// --- 最終スロットだけClose済み(コライダーのみ描画されたフレーム) ---
	{
		const bool closed[3] = { false, false, true };
		const std::vector<uint32_t> order = RenderBatchOrder::Build(closed, 3);
		expect(ids_equal(order, { MAIN_FIRST, MAIN_DEFERRED, PARALLEL_BASE + 2, MAIN_FINAL }),
			"only trailing slot executes between deferred and final lists");
	}

	// --- 前半側スロットだけClose済み(Player/Bossのみ) ---
	{
		const bool closed[3] = { true, true, false };
		const std::vector<uint32_t> order = RenderBatchOrder::Build(closed, 3);
		expect(ids_equal(order, { MAIN_FIRST, PARALLEL_BASE + 0, PARALLEL_BASE + 1, MAIN_DEFERRED, MAIN_FINAL }),
			"leading slots execute between first and deferred lists");
	}

	// --- フラグ配列がnullptrでもメイン3本は必ず実行対象になる ---
	{
		const std::vector<uint32_t> order = RenderBatchOrder::Build(nullptr, 3);
		expect(ids_equal(order, { MAIN_FIRST, MAIN_DEFERRED, MAIN_FINAL }),
			"null flags still schedules the three main lists");
	}

	// --- 不変条件: 先頭=前半/終端=最後/後半は並列スロットより後ろ(全256パターンを総当たり) ---
	{
		bool invariant_ok = true;
		for (uint32_t pattern = 0; pattern < 8u * 8u * 8u; ++pattern)
		{
			bool closed[3];
			uint32_t slot_count = 1 + (((pattern >> 6) & 0x7u) % 3u); // 1〜3を網羅.
			for (uint32_t s = 0; s < slot_count; ++s) { closed[s] = ((pattern >> (s * 2)) & 0x1u) != 0; }

			const std::vector<uint32_t> order = RenderBatchOrder::Build(closed, slot_count);
			if (order.size() < 3) { invariant_ok = false; break; }
			if (order.front() != MAIN_FIRST || order.back() != MAIN_FINAL) { invariant_ok = false; break; }
			if (order[1] != MAIN_DEFERRED && order[1] < PARALLEL_BASE) { invariant_ok = false; break; }

			bool seen_deferred = false;
			for (size_t i = 0; i < order.size(); ++i)
			{
				if (order[i] == MAIN_DEFERRED) {
					seen_deferred = true;
					continue;
				}
				if (seen_deferred && order[i] >= PARALLEL_BASE && order[i] != MAIN_FINAL) {
					// 後半リストより後に来るのは最終スロットだけ.
					const bool is_last_slot = (order[i] - PARALLEL_BASE) == (slot_count - 1);
					if (!is_last_slot) { invariant_ok = false; }
				}
				if (!seen_deferred && order[i] == MAIN_FINAL) { invariant_ok = false; }
			}
			if (!invariant_ok) { break; }
		}
		expect(invariant_ok, "invariants hold across flag combinations");
	}

	std::printf("\n%d passed, %d failed\n", passed, failed);
	return (failed == 0) ? 0 : 1;
}
