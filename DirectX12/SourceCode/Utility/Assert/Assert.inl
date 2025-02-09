#pragma once

#include <stdexcept>
#include <type_traits>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <locale>
#include <windows.h>        // HRESULT と Windows API の定義が含まれる
#include <d3dcompiler.h>    // Blobの定義が含まれる.

#include "Utility/String/String.h"

namespace MyAssert {

    // HRESULTを日本語として返す.
    static inline std::string_view HResultToJapanese(const HRESULT& Result)
    {
        switch (Result) {
        case E_ABORT:           return "操作は中止されました";
        case E_ACCESSDENIED:    return "一般的なアクセス拒否エラーが発生しました";
        case E_FAIL:            return "不特定のエラー";
        case E_HANDLE:          return "無効なハンドル";
        case E_INVALIDARG:      return "1 つ以上の引数が無効です";
        case E_NOINTERFACE:     return "そのようなインターフェイスはサポートされていません。";
        case E_NOTIMPL:         return "未実装";
        case E_OUTOFMEMORY:     return "必要なメモリの割り当てに失敗しました";
        case E_POINTER:         return "無効なポインター";
        case E_UNEXPECTED:      return "予期しないエラー";
        default:                return "未知のエラー";
        }
    }

#if UNICODE
    using RuntimeErrorMsg = std::wstring_view;

    static inline std::string FormatErrorMessage(RuntimeErrorMsg ErrorMsg, const HRESULT& Result) {
        std::string NarrowErrorMsg = MyString::WStringToString(std::wstring(ErrorMsg));
        std::string_view HResultMsg = HResultToJapanese(Result);
        std::ostringstream ss;
        ss << NarrowErrorMsg << " に失敗,\n 原因 : " << HResultMsg;
        return ss.str();
    }
#else
    using RuntimeErrorMsg = std::string_view;

    static inline std::string FormatErrorMessage(RuntimeErrorMsg ErrorMsg, HRESULT Result) {
        std::string_view HResultMsg = HResultToJapanese(Result);
        std::ostringstream ss;
        ss << ErrorMsg << " に失敗,\n 原因 : " << HResultMsg;
        return ss.str();
    }
#endif

    /*******************************************
    * @brief    HRESULT,boolが戻り値の関数エラーをthrow.
    * @param    実行したい関数ポインタ.
    * @param    必要な引数.
    *******************************************/
    template<typename FunctionPtr, typename ...FuncArgs,
        std::enable_if_t<
        std::is_same_v<std::invoke_result_t<FunctionPtr, FuncArgs...>, bool> ||
        std::is_same_v<std::invoke_result_t<FunctionPtr, FuncArgs...>, HRESULT>, int> = 0 >
    static inline bool IsFailed(
        RuntimeErrorMsg ErrorMsg,
        FunctionPtr && Func,
        FuncArgs&&... Args) noexcept(false)
    {
        // 関数の戻り値型を取得.
        using ResultType = std::invoke_result_t<FunctionPtr, FuncArgs...>;

        // 関数を実行.
        ResultType Result = std::invoke(std::forward<FunctionPtr>(Func), std::forward<FuncArgs>(Args)...);

        // エラー判定(HRISULT).
        if constexpr (std::is_same_v<ResultType, HRESULT>) {
            if (FAILED(Result)) {
                throw std::runtime_error(FormatErrorMessage(ErrorMsg, Result));
                return false;
            }
        }
        // エラー判定(bool).
        else if constexpr (std::is_same_v<ResultType, bool>) {
            if (!Result) {
                throw std::runtime_error(std::string(ErrorMsg));
                return false;
            }
        }

        return true;
    }

    /*******************************************
    * @brief    ErrorBlobに入ったエラーを出力.
    * @param    成功かどうか.
    * @param    ErrorBlob.
    *******************************************/
    static inline void ErrorBlob(const HRESULT& Result, ID3DBlob* ErrorMsg)
    {
        if (SUCCEEDED(Result)) {
            return;
        }

        std::wstring ErrStr;

        if (Result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
            ErrStr = L"ファイルが見当たりません";
        }
        else if (ErrorMsg) {
            ErrStr.assign(static_cast<const wchar_t*>(ErrorMsg->GetBufferPointer()), ErrorMsg->GetBufferSize() / sizeof(wchar_t));
        }
        else {
            ErrStr = L"ErrorMsg is null";
        }

        if (ErrorMsg) {
            ErrorMsg->Release();
        }

        throw std::runtime_error(FormatErrorMessage(ErrStr, Result));
    }

} // namespace MyAssert
