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

namespace MyString {
	// 値を文字列に変換する.
	template<typename T>
	std::string ToString(const T& value);
	// 文字列から値を戻す.
	template<typename T>
	T FromString(const std::string& str);

	// 特定の行の値を取り出す.
	std::string ExtractAmount(const std::string& str);

	// 特定の行を取り出す.
	std::string ExtractLine(const std::string& str, int Line);

	// 文字列をfloatへ変換.
	float Stof(std::string str);
	// 文字列をboolへ変換.
	bool Stob(std::string str);

	// ワイド文字をマルチバイトに変換.
	std::string WStringToString(const std::wstring& wideStr);

	// マルチバイトをワイド文字に変換.
	std::wstring StringToWString(const std::string& Str);

	// UTF-16からUTF-8へ変換.
	std::string UTF16ToUTF8(const std::u16string& utf16); 

	// UTF-8からUTF-16へ変換.
	std::u16string UTF8ToUTF16(const std::string& utf8); 


}