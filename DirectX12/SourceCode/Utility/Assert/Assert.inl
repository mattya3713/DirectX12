#pragma once

#include <stdexcept>
#include <type_traits>
#include <sstream>
#include <string>
#include <tuple>
#include <locale>
#include <windows.h> // HRESULT と Windows API の定義が含まれる

#include "Utility/String/String.h"

namespace MyAssert {

	// HRESULTを日本語として返す.
	static inline std::string HRESULTToJapanese(HRESULT result)
	{
		switch (result) {
		case E_ABORT: return "操作は中止されました";
		case E_ACCESSDENIED: return "一般的なアクセス拒否エラーが発生しました";
		case E_FAIL: return "不特定のエラー";
		case E_HANDLE: return "無効なハンドル";
		case E_INVALIDARG: return "1 つ以上の引数が無効です";
		case E_NOINTERFACE: return "そのようなインターフェイスはサポートされていません。";
		case E_NOTIMPL: return "未実装";
		case E_OUTOFMEMORY: return "必要なメモリの割り当てに失敗しました";
		case E_POINTER: return "無効なポインター";
		case E_UNEXPECTED: return "予期しないエラー";
		default: return "未知のエラー";
		};

	}

#if UNICODE
	using runtime_error_msg = LPCWSTR;

	static inline std::string FormatErrorMessage(const std::wstring& errorMsg, HRESULT result) {
		std::string narrowErrorMsg = MyString::WStringToString(errorMsg);
		std::string HresultMsg = HRESULTToJapanese(result);
		std::stringstream ss;
		ss << narrowErrorMsg << " に失敗,\n 原因 : " << std::hex << HresultMsg;
		return ss.str();
	}
#else
	using runtime_error_msg = LPCSTR;

	static inline std::wstring NarrowToWide(const std::string& narrowStr) {
		int size = MultiByteToWideChar(CP_UTF8, 0, narrowStr.c_str(), -1, nullptr, 0);
		if (size <= 0) {
			throw std::runtime_error("NarrowToWide conversion failed.");
		}
		std::wstring wideStr;
		wideStr.resize(size - 1); // 終端のnull文字を除く
		MultiByteToWideChar(CP_UTF8, 0, narrowStr.c_str(), -1, &wideStr[0], size);
		return wideStr;
	}

	static inline std::wstring FormatErrorMessage(const std::string& errorMsg, HRESULT result) {
		std::wstring wideErrorMsg = NarrowToWide(errorMsg);
		std::wstringstream ss;
		ss << wideErrorMsg << L" に失敗, 原因 : 0x" << std::hex << result;
		return ss.str();
	}
#endif

	template<class Func, class... Args,
		std::enable_if_t<std::is_same_v<std::invoke_result_t<Func, Args...>, HRESULT>, int> = 0>
	static inline bool IsFailed(runtime_error_msg errorMsg, Func&& func, Args&&... args) noexcept(false) {
		HRESULT result = S_OK;
		std::tuple<Args...> tup(std::forward<Args>(args)...);

		result = std::apply(std::forward<Func>(func), std::move(tup));

		if (FAILED(result)) {
			std::string formattedMessage;

#if UNICODE
			// LPCWSTR を std::wstring に変換
			std::wstring wideErrorMsg(errorMsg);
			formattedMessage = FormatErrorMessage(wideErrorMsg, result);
#else
			// LPCSTR を std::string に変換
			formattedMessage = FormatErrorMessage(std::string(errorMsg), result);
#endif

			throw std::runtime_error(formattedMessage);
			return false;
		}
		return true;
	}
}