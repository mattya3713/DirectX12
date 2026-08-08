#include "MemoryLeakDetector.h"

#include <crtdbg.h>
#include <Windows.h>

namespace {
	_CrtMemState g_StartState = {};
}

namespace Diagnostics {

void BeginMemoryLeakCheck()
{
	_CrtMemCheckpoint(&g_StartState);
}

void EndMemoryLeakCheck()
{
	_CrtMemState end_state = {};
	_CrtMemCheckpoint(&end_state);

	_CrtMemState diff_state = {};
	if (_CrtMemDifference(&diff_state, &g_StartState, &end_state)) {
		// デバッグ出力(VSのデバッガ使用時)とstderr(デバッガ無しの実行時)の両方へ出す.
		_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG | _CRTDBG_MODE_FILE);
		_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);

		OutputDebugStringA("========== [MemoryLeakDetector] メモリリークを検出しました ==========\n");
		_CrtMemDumpStatistics(&diff_state);
		_CrtMemDumpAllObjectsSince(&g_StartState);
	}
}

} // namespace Diagnostics
