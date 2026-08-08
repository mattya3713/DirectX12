#include "CrashDumpHandler.h"

#include <Windows.h>
#include <DbgHelp.h>
#include <exception>
#include <cstdio>
#include <ctime>

#pragma comment(lib, "Dbghelp.lib")

namespace {

	// タイムスタンプ付きのダンプファイルを書き出す(ExceptionPointersはSEH例外時のみ非null).
	void WriteDumpFile(EXCEPTION_POINTERS* ExceptionPointers)
	{
		CreateDirectoryA("Dumps", nullptr);

		time_t now = time(nullptr);
		tm local_time = {};
		localtime_s(&local_time, &now);

		char file_name[256] = {};
		sprintf_s(file_name, "Dumps\\crash_%04d%02d%02d_%02d%02d%02d.dmp",
			local_time.tm_year + 1900, local_time.tm_mon + 1, local_time.tm_mday,
			local_time.tm_hour, local_time.tm_min, local_time.tm_sec);

		HANDLE file = CreateFileA(file_name, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (file == INVALID_HANDLE_VALUE) { return; }

		MINIDUMP_EXCEPTION_INFORMATION dump_info = {};
		dump_info.ThreadId = GetCurrentThreadId();
		dump_info.ExceptionPointers = ExceptionPointers;
		dump_info.ClientPointers = FALSE;

		MiniDumpWriteDump(
			GetCurrentProcess(), GetCurrentProcessId(), file,
			MiniDumpWithDataSegs,
			ExceptionPointers ? &dump_info : nullptr,
			nullptr, nullptr);

		CloseHandle(file);
	}

	// SEH例外(アクセス違反等)の未処理ハンドラー.
	LONG WINAPI UnhandledExceptionHandler(EXCEPTION_POINTERS* ExceptionPointers)
	{
		WriteDumpFile(ExceptionPointers);
		return EXCEPTION_EXECUTE_HANDLER;
	}

	// std::terminate経由(未捕捉のC++例外・abort等)のハンドラー.
	void TerminateHandler()
	{
		WriteDumpFile(nullptr);
		std::abort();
	}

} // namespace

namespace Diagnostics {

void InstallCrashDumpHandler()
{
	SetUnhandledExceptionFilter(UnhandledExceptionHandler);
	std::set_terminate(TerminateHandler);
}

} // namespace Diagnostics
