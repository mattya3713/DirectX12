#pragma once
#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <stdexcept>
#include <unordered_set>
#include <locale>
#include <codecvt>
#include "magic_enum.h"

namespace MyString {
	// �l�𕶎���ɕϊ�����.
	template<typename T>
	std::string ToString(const T& value)
	{
		std::stringstream ss;
		ss << value << "," << typeid(T).name() << ";";
		return ss.str();
	}

	// �����񂩂�l��߂�.
	template<typename T>
	T FromString(const std::string& str)
	{
		std::stringstream ss(str);
		T value;
		ss >> value;
		return value;
	}

	// ����̍s�̒l�����o��.
	std::string ExtractAmount(const std::string& str);

	// ����̍s�����o��.
	std::string ExtractLine(const std::string& str, int Line);

	// �������float�֕ϊ�.
	float Stof(std::string str);
	// �������bool�֕ϊ�.
	bool Stob(std::string str);

	// Enum��String�֕ϊ�.
	template<typename T>
	std::string EnumToString(T Enum)
	{
		return std::string(magic_enum::enum_name<T>(Enum));
	}

	template<typename T>
	std::vector<std::string> EnumToStrings()
	{
		std::vector<std::string> result;
		for (const auto& name : magic_enum::enum_names<T>())
		{

			result.emplace_back(std::string(name));
		}
		return result;
	}
	template<typename T>
	std::vector<std::string> EnumToStrings(std::unordered_set<T> Skip)
	{
		std::vector<std::string> result;
		for (const auto& name : magic_enum::enum_names<T>())
		{
			T value = magic_enum::enum_cast<T>(name).value();
			if (Skip.find(value) == Skip.end())
			{
				result.emplace_back(std::string(name));
			}
		}
		return result;
	}

	// String��Enum�֕ϊ�.
	template<typename T>
	T StringToEnum(std::string& str)
	{
		auto optEnum = magic_enum::enum_cast<T>(str);
		if (optEnum.has_value()) {
			return optEnum.value();
		}
		_ASSERT_EXPR(false, L"Not Error Message");
		return optEnum.value();

	}

	// ���C�h�������}���`�o�C�g�ɕϊ�.
	std::string WStringToString(const std::wstring& wideStr);

	// �}���`�o�C�g�����C�h�����ɕϊ�.
	std::wstring StringToWString(const std::string& Str);

	// UTF-16����UTF-8�֕ϊ�.
	std::string UTF16ToUTF8(const std::u16string& utf16); 

	// UTF-8����UTF-16�֕ϊ�.
	std::u16string UTF8ToUTF16(const std::string& utf8); 
}