#pragma once

#include <stdexcept>
#include <type_traits>
#include <sstream>
#include <string>
#include <tuple>
#include <locale>
#include <windows.h>	 // HRESULT と Windows API の定義が含まれる

#include <d3dcompiler.h> // Blobの定義が含まれる.

#include "Utility/String/String.h"

namespace MyAssert {

	// HRESULTを日本語として返す.
	static inline std::string HResultToJapanese(HRESULT Result)
	{
		switch (Result) {
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

	static inline std::string FormatErrorMessage(const std::wstring& ErrorMsg, HRESULT Result) {
		std::string NarrowErrorMsg = MyString::WStringToString(ErrorMsg);
		std::string HResultMsg = HResultToJapanese(Result);
		std::stringstream ss;
		ss << NarrowErrorMsg << " に失敗,\n 原因 : " << std::hex << HResultMsg;
		return ss.str();
	}
#else
	using runtime_error_msg = LPCSTR;

	static inline std::wstring NarrowToWide(const std::string& NarrowStr) {
		int Size = MultiByteToWideChar(CP_UTF8, 0, NarrowStr.c_str(), -1, nullptr, 0);
		if (Size <= 0) {
			throw std::runtime_error("NarrowToWide conversion failed.");
		}
		std::wstring WideStr;
		WideStr.resize(Size - 1); // 終端のnull文字を除く
		MultiByteToWideChar(CP_UTF8, 0, NarrowStr.c_str(), -1, &WideStr[0], Size);
		return WideStr;
	}

	static inline std::wstring FormatErrorMessage(const std::string& ErrorMsg, HRESULT Result) {
		std::wstring WideErrorMsg = NarrowToWide(ErrorMsg);
		std::wstringstream ss;
		ss << WideErrorMsg << L" に失敗, 原因 : 0x" << std::hex << Result;
		return ss.str();
	}
#endif
	template<typename Func, typename ...Args,
		std::enable_if_t<
		std::is_same_v<std::invoke_result_t<Func, Args...>, bool> ||
		std::is_same_v<std::invoke_result_t<Func, Args...>, HRESULT>, int>>
		static inline bool IsFailed(
			runtime_error_msg ErrorMsg,
			Func&& func,
			Args&&... args) noexcept(false)
	{
		HRESULT Result = S_OK;
		std::tuple<Args...> Tup(std::forward<Args>(args)...);

		Result = std::apply(std::forward<Func>(func), std::move(Tup));

		if (FAILED(Result)) {
			std::string FormattedMessage;

#if UNICODE
			// LPCWSTR を std::wstring に変換.
			std::wstring WideErrorMsg(ErrorMsg);
			FormattedMessage = FormatErrorMessage(WideErrorMsg, Result);
#else
			// LPCSTR を std::string に変換.
			FormattedMessage = FormatErrorMessage(std::string(ErrorMsg), Result);
#endif

			throw std::runtime_error(FormattedMessage);
			return false;
		}
		return true;
	}

	/*******************************************
	* @brief	ErroeBlobに入ったエラーを出力.
	* @param	成功かどうか.
	* @param	ErroeBlob.
	*******************************************/
	static void ErrorBlob(const HRESULT& Result, ID3DBlob* ErrorMsg)
	{
		// 成功なら処理をしない.
		if (SUCCEEDED(Result)) { return; }

		std::wstring ErrStr;

		if (Result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
			ErrStr = L"ファイルが見当たりません";
		}
		else {
			if (ErrorMsg) {
				// ErrorMsg があるの場合.
				ErrStr.resize(ErrorMsg->GetBufferSize());
				std::copy_n(static_cast<const char*>(ErrorMsg->GetBufferPointer()), ErrorMsg->GetBufferSize(), ErrStr.begin());
				ErrStr += L"\n";
			}
			else {
				// ErrorMsg がないの場合.
				ErrStr = L"ErrorMsg is null";
			}
		}
		if (ErrorMsg) {
			ErrorMsg->Release();  // メモリ解放
		}

		std::wstring WideErrorMsg(ErrStr);
		std::string FormattedMessage = MyAssert::FormatErrorMessage(WideErrorMsg, Result);
		throw std::runtime_error(FormattedMessage);
	}
}#pragma once
#include "Utility\StringConverter\StringConverter.h"
class MyAssert
{
public:
	/************************************************************
	* @brief boolとHRESULTを判定して例外があるなら落とす.
	* @param msg    ：表示メッセージ.
	* @param func    ：実行関数.
	* @param args    ：実行関数の引数.
	************************************************************/
	template<typename Func, typename ...Args,
		std::enable_if_t<
		std::is_same_v<std::invoke_result_t<Func, Args...>, bool> ||
		std::is_same_v<std::invoke_result_t<Func, Args...>, HRESULT>, int> = 0>
	static void Assert(const std::string msg, Func && func, Args&&... args);
private:
	/************************************************************
	* @brief HRESULTのエラー内容を文字列で返す.
	* @param hr：エラーコード.
	************************************************************/
	static const std::string GetErrorCode(const HRESULT hr);


	/************************************************************
	* @brief エラーハンドリングを行う.
	* @param msg    ：エラーメッセージ.
	* @param result    ：エラー結果.
	************************************************************/
	template <typename Result>
	static void HandleErrorResult(const std::string& msg, Result result);
};

//------------------------------------------------------------------------------------------.

template<typename Func, typename ...Args,
	std::enable_if_t<
	std::is_same_v<std::invoke_result_t<Func, Args...>, bool> ||
	std::is_same_v<std::invoke_result_t<Func, Args...>, HRESULT>, int>>
	inline void MyAssert::Assert(const std::string msg, Func&& func, Args && ...args)
{
	std::tuple<Args...> tup(std::forward<Args>(args)...);

	// 判定.
	auto result = std::apply(std::forward<Func>(func), std::move(tup));

	// エラーハンドリング.
	HandleErrorResult(msg, result);
}

//------------------------------------------------------------------------------------------.

template<typename Result>
inline void MyAssert::HandleErrorResult(const std::string& msg, Result result)
{
	// HRESULTの場合.
	if constexpr (std::is_same_v<Result, HRESULT>)
	{
		std::string errorMsg = msg + "：" + GetErrorCode(result);
		_ASSERT_EXPR(SUCCEEDED(result), StringConverter::StringToWStirng(errorMsg).c_str());
	}
	// boolの場合.
	else if constexpr (std::is_same_v<Result, bool>)
	{
		_ASSERT_EXPR(SUCCEEDED(result), StringConverter::StringToWStirng(msg).c_str());
	}
}｝｝｝｝｝｝