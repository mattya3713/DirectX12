#pragma once

#include <stdexcept>
#include <type_traits>
#include <sstream>
#include <string>
#include <tuple>
#include <locale>
#include <windows.h>	 // HRESULT �� Windows API �̒�`���܂܂��

#include <d3dcompiler.h> // Blob�̒�`���܂܂��.

#include "Utility/String/String.h"

namespace MyAssert {

	// HRESULT����{��Ƃ��ĕԂ�.
	static inline std::string HResultToJapanese(HRESULT Result)
	{
		switch (Result) {
		case E_ABORT: return "����͒��~����܂���";
		case E_ACCESSDENIED: return "��ʓI�ȃA�N�Z�X���ۃG���[���������܂���";
		case E_FAIL: return "�s����̃G���[";
		case E_HANDLE: return "�����ȃn���h��";
		case E_INVALIDARG: return "1 �ȏ�̈����������ł�";
		case E_NOINTERFACE: return "���̂悤�ȃC���^�[�t�F�C�X�̓T�|�[�g����Ă��܂���B";
		case E_NOTIMPL: return "������";
		case E_OUTOFMEMORY: return "�K�v�ȃ������̊��蓖�ĂɎ��s���܂���";
		case E_POINTER: return "�����ȃ|�C���^�[";
		case E_UNEXPECTED: return "�\�����Ȃ��G���[";
		default: return "���m�̃G���[";
		};

	}

#if UNICODE
	using runtime_error_msg = LPCWSTR;

	static inline std::string FormatErrorMessage(const std::wstring& ErrorMsg, HRESULT Result) {
		std::string NarrowErrorMsg = MyString::WStringToString(ErrorMsg);
		std::string HResultMsg = HResultToJapanese(Result);
		std::stringstream ss;
		ss << NarrowErrorMsg << " �Ɏ��s,\n ���� : " << std::hex << HResultMsg;
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
		WideStr.resize(Size - 1); // �I�[��null����������
		MultiByteToWideChar(CP_UTF8, 0, NarrowStr.c_str(), -1, &WideStr[0], Size);
		return WideStr;
	}

	static inline std::wstring FormatErrorMessage(const std::string& ErrorMsg, HRESULT Result) {
		std::wstring WideErrorMsg = NarrowToWide(ErrorMsg);
		std::wstringstream ss;
		ss << WideErrorMsg << L" �Ɏ��s, ���� : 0x" << std::hex << Result;
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
			// LPCWSTR �� std::wstring �ɕϊ�.
			std::wstring WideErrorMsg(ErrorMsg);
			FormattedMessage = FormatErrorMessage(WideErrorMsg, Result);
#else
			// LPCSTR �� std::string �ɕϊ�.
			FormattedMessage = FormatErrorMessage(std::string(ErrorMsg), Result);
#endif

			throw std::runtime_error(FormattedMessage);
			return false;
		}
		return true;
	}

	/*******************************************
	* @brief	ErroeBlob�ɓ������G���[���o��.
	* @param	�������ǂ���.
	* @param	ErroeBlob.
	*******************************************/
	static void ErrorBlob(const HRESULT& Result, ID3DBlob* ErrorMsg)
	{
		// �����Ȃ珈�������Ȃ�.
		if (SUCCEEDED(Result)) { return; }

		std::wstring ErrStr;

		if (Result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
			ErrStr = L"�t�@�C������������܂���";
		}
		else {
			if (ErrorMsg) {
				// ErrorMsg ������̏ꍇ.
				ErrStr.resize(ErrorMsg->GetBufferSize());
				std::copy_n(static_cast<const char*>(ErrorMsg->GetBufferPointer()), ErrorMsg->GetBufferSize(), ErrStr.begin());
				ErrStr += L"\n";
			}
			else {
				// ErrorMsg ���Ȃ��̏ꍇ.
				ErrStr = L"ErrorMsg is null";
			}
		}
		if (ErrorMsg) {
			ErrorMsg->Release();  // ���������
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
	* @brief bool��HRESULT�𔻒肵�ė�O������Ȃ痎�Ƃ�.
	* @param msg    �F�\�����b�Z�[�W.
	* @param func    �F���s�֐�.
	* @param args    �F���s�֐��̈���.
	************************************************************/
	template<typename Func, typename ...Args,
		std::enable_if_t<
		std::is_same_v<std::invoke_result_t<Func, Args...>, bool> ||
		std::is_same_v<std::invoke_result_t<Func, Args...>, HRESULT>, int> = 0>
	static void Assert(const std::string msg, Func && func, Args&&... args);
private:
	/************************************************************
	* @brief HRESULT�̃G���[���e�𕶎���ŕԂ�.
	* @param hr�F�G���[�R�[�h.
	************************************************************/
	static const std::string GetErrorCode(const HRESULT hr);


	/************************************************************
	* @brief �G���[�n���h�����O���s��.
	* @param msg    �F�G���[���b�Z�[�W.
	* @param result    �F�G���[����.
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

	// ����.
	auto result = std::apply(std::forward<Func>(func), std::move(tup));

	// �G���[�n���h�����O.
	HandleErrorResult(msg, result);
}

//------------------------------------------------------------------------------------------.

template<typename Result>
inline void MyAssert::HandleErrorResult(const std::string& msg, Result result)
{
	// HRESULT�̏ꍇ.
	if constexpr (std::is_same_v<Result, HRESULT>)
	{
		std::string errorMsg = msg + "�F" + GetErrorCode(result);
		_ASSERT_EXPR(SUCCEEDED(result), StringConverter::StringToWStirng(errorMsg).c_str());
	}
	// bool�̏ꍇ.
	else if constexpr (std::is_same_v<Result, bool>)
	{
		_ASSERT_EXPR(SUCCEEDED(result), StringConverter::StringToWStirng(msg).c_str());
	}
}�p�p�p�p�p�p