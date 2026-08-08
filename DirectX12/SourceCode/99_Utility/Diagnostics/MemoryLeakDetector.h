#pragma once

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/09.
* @brief     : CRTヒープのメモリリーク検知(Create〜Releaseの間で増えた分だけを検出).
**********************************************************************************/

namespace Diagnostics {

	// リーク検知の開始地点を記録する(Main::Create()の最初で呼ぶ想定).
	void BeginMemoryLeakCheck();

	// 開始地点からのリークを検知し、あればデバッグ出力とstderrへレポートする(Main::Release()の最後で呼ぶ想定).
	void EndMemoryLeakCheck();

} // namespace Diagnostics
