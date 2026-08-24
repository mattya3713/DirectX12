#pragma once

//ヘッダ読込.
#include <cstdint>
#include <vector>

/**********************************************************
* @author      : 白虎(びゃっこ).
* @date        : 2026/08/24.
* @brief       : マルチスレッド記録したコマンドリストの実行順を組み立てる純粋関数群.
*              : GPU依存が無いためDirectX12本体と単体テストの両方から使える.
**********************************************************/

class RenderBatchOrder final
{
public:
	RenderBatchOrder() = delete;

	// リスト種別ID(メイン0=前半/1=後半/2=終端. 並列スロットはParallelBase+スロット番号).
	static constexpr uint32_t MainFirst    = 0;	// 前半(シャドウパス〜セットアップ).
	static constexpr uint32_t MainDeferred = 1;	// 後半(スプライト/テキスト/静的レベル/パーティクル).
	static constexpr uint32_t MainFinal    = 2;	// 終端(PRESENT遷移とクエリ解決専用).
	static constexpr uint32_t ParallelBase = 3;	// 並列スロットIDの起点.

	// 並列スロットのClose済みフラグ配列から実行順ID列を返す.
	// 順序規則: 前半 → 並列スロット0〜N-2 → 後半 → 最終スロット(N-1) → 終端.
	// (最終スロットだけ後半より後ろに置くことで、コライダー等の追加描画が
	//  終端リストのPRESENT遷移より前に描かれることを保証する).
	static std::vector<uint32_t> Build(const bool* pParallelSlotClosed, uint32_t SlotCount)
	{
		std::vector<uint32_t> Order;
		Order.reserve(3 + SlotCount);

		Order.push_back(MainFirst);

		if (pParallelSlotClosed != nullptr)
		{
			for (uint32_t Slot = 0; Slot + 1 < SlotCount; ++Slot)
			{
				if (pParallelSlotClosed[Slot]) { Order.push_back(ParallelBase + Slot); }
			}
		}

		Order.push_back(MainDeferred);

		if (pParallelSlotClosed != nullptr && SlotCount > 0 && pParallelSlotClosed[SlotCount - 1])
		{
			Order.push_back(ParallelBase + (SlotCount - 1));
		}

		Order.push_back(MainFinal);
		return Order;
	}
};
