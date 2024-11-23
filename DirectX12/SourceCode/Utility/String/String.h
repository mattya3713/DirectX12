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
	// 値を文字列に変換する.
	template<typename T>
	std::string ToString(const T& value)
	{
		std::stringstream ss;
		ss << value << "," << typeid(T).name() << ";";
		return ss.str();
	}

	// 文字列から値を戻す.
	template<typename T>
	T FromString(const std::string& str)
	{
		std::stringstream ss(str);
		T value;
		ss >> value;
		return value;
	}

	// 特定の行の値を取り出す.
	std::string ExtractAmount(const std::string& str);

	// 特定の行を取り出す.
	std::string ExtractLine(const std::string& str, int Line);

	// 文字列をfloatへ変換.
	float Stof(std::string str);
	// 文字列をboolへ変換.
	bool Stob(std::string str);

	// EnumをStringへ変換.
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

	// StringをEnumへ変換.
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

	// ワイド文字をマルチバイトに変換.
	std::string WStringToString(const std::wstring& wideStr);

	// マルチバイトをワイド文字に変換.
	std::wstring StringToWString(const std::string& Str);

	// UTF-16からUTF-8へ変換.
	std::string UTF16ToUTF8(const std::u16string& utf16); 

	// UTF-8からUTF-16へ変換.
	std::u16string UTF8ToUTF16(const std::string& utf8); 
}