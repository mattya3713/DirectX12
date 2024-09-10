#pragma once

#include <stdexcept>
#include <type_traits>
#include <sstream>
#include <string>
#include <tuple>
#include <locale>
#include <windows.h> // HRESULT �� Windows API �̒�`���܂܂��

#include "Utility/String/String.h"

namespace MyAssert {

	// HRESULT����{��Ƃ��ĕԂ�.
	static inline std::string HRESULTToJapanese(HRESULT result)
	{
		switch (result) {
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

	static inline std::string FormatErrorMessage(const std::wstring& errorMsg, HRESULT result) {
		std::string narrowErrorMsg = MyString::WStringToString(errorMsg);
		std::string HresultMsg = HRESULTToJapanese(result);
		std::stringstream ss;
		ss << narrowErrorMsg << " �Ɏ��s,\n ���� : " << std::hex << HresultMsg;
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
		wideStr.resize(size - 1); // �I�[��null����������
		MultiByteToWideChar(CP_UTF8, 0, narrowStr.c_str(), -1, &wideStr[0], size);
		return wideStr;
	}

	static inline std::wstring FormatErrorMessage(const std::string& errorMsg, HRESULT result) {
		std::wstring wideErrorMsg = NarrowToWide(errorMsg);
		std::wstringstream ss;
		ss << wideErrorMsg << L" �Ɏ��s, ���� : 0x" << std::hex << result;
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
			// LPCWSTR �� std::wstring �ɕϊ�
			std::wstring wideErrorMsg(errorMsg);
			formattedMessage = FormatErrorMessage(wideErrorMsg, result);
#else
			// LPCSTR �� std::string �ɕϊ�
			formattedMessage = FormatErrorMessage(std::string(errorMsg), result);
#endif

			throw std::runtime_error(formattedMessage);
			return false;
		}
		return true;
	}
}