#pragma once

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/09.
* @brief     : 未処理例外(SEH例外・std::terminate経由の両方)発生時に
*            : クラッシュダンプ(.dmp)を書き出すハンドラー.
**********************************************************************************/

namespace Diagnostics {

	// クラッシュダンプハンドラーを登録する(WinMainの最初で呼ぶ想定).
	void InstallCrashDumpHandler();

} // namespace Diagnostics
