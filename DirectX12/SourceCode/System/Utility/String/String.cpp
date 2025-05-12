#include "String.h"
namespace MyString 
{
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
	std::string ExtractAmount(const std::string& str)
	{
		std::istringstream iss(str);
		std::string valueStr;
		std::string typeStr;

		// ','までの値部分とその後の型部分を分割.
		if (std::getline(iss, valueStr, ',') && std::getline(iss, typeStr, ';')) {
			// 改行を取り除く必要がある場合はここで行う.
			if (typeStr.back() == '\n') {
				typeStr.pop_back();
			}

			// 型がfloatの場合、小数点第一位までを文字列として返す.
			if (typeStr == "float") {
				char* end;
				float value = std::strtof(valueStr.c_str(), &end);

				std::ostringstream oss;
				oss << std::fixed << std::setprecision(1) << value;
				return oss.str();
			}

			// 型がboolの場合真偽値を文字列として返す.
			if (typeStr == "bool") {
				// "true"または"1"の場合にtrueと判断
				if (valueStr == "true" || valueStr == "1") {
					return "true";
				}
				else {
					return "false";
				}
			}
			// 値部分を返す.
			return valueStr;
		}

		return "";
	}

	// 特定の行を取り出す.
	std::string ExtractLine(const std::string& str, int Line)
	{
		// UI情報を文字列としてistringstreamに読み込む.
		std::istringstream iss(str);

		// 指定された行まで読み飛ばす.
		std::string line;
		for (int i = 0; i <= Line; ++i)
		{
			if (!std::getline(iss, line))
			{
				return ""; // 行が見つからない場合は空文字列を返す.
			}
		}

		return line;
	}

	// 文字列をfloatへ変換.
	float Stof(std::string str)
	{
		try {
			// std::stof を使って文字列から浮動小数点数に変換.
			return std::stof(str); 
		}
	
		// TODO : 例外処理を作る.
		catch (const std::invalid_argument& e) {
			// 無効な引数が渡された場合の例外処理.
			std::cerr << "引数無効だぜ: " << e.what() << std::endl;
		}
		catch (const std::out_of_range& e) {
			std::cerr << "数値が範囲外だぜ: " << e.what() << std::endl;
		}
		// エラーが発生した場合はデフォルトで 0.0 を返す.
		return 0.0f;
	}

	// 文字列をfloatへ変換.
	bool Stob(std::string str)
	{
		try {
			// 文字列がtrueか1ならtrueを返す.
			if (str == "true" || str == "1") {
				return true;
			}
			else {
				return false;
			}
		}

		// TODO : 例外処理を作る.
		catch (const std::invalid_argument& e) {
			// 無効な引数が渡された場合の例外処理.
			std::cerr << "引数無効だぜ: " << e.what() << std::endl;
		}
		catch (const std::out_of_range& e) {
			std::cerr << "数値が範囲外だぜ: " << e.what() << std::endl;
		}
		// エラーが発生した場合はデフォルトで 0.0 を返す.
		return 0.0f;
	}

	// ワイド文字をマルチバイトに変換.
	std::string WStringToString(const std::wstring& WideStr)
	{
		// 変換後のバッファサイズを取得.
		int Size = WideCharToMultiByte(CP_THREAD_ACP, 0, WideStr.c_str(), -1, (char*)  nullptr, 0, nullptr, nullptr);
		// 変換結果を格納するバッファを用意.
		std::string StrBuf(Size - 1, 0); // 終端の NULL 文字を含めないようにする.
		WideCharToMultiByte(CP_THREAD_ACP, 0, WideStr.c_str(), -1, &StrBuf[0], Size, nullptr, nullptr);
		return StrBuf;
	}

	std::wstring StringToWString(const std::string& Str)
	{
		// 変換後のバッファサイズを取得.
		int Size = MultiByteToWideChar(CP_THREAD_ACP, 0, Str.c_str(), -1, nullptr, 0);
		// 変換結果を格納するバッファを用意.
		std::wstring WideStrBuf(Size - 1, 0); // 終端の NULL 文字を含めないようにする.
		MultiByteToWideChar(CP_THREAD_ACP, 0, Str.c_str(), -1, &WideStrBuf[0], Size);
		return WideStrBuf;
	}

	std::string UTF16ToUTF8(const std::u16string& UTF16)
	{
		// 変換後のバッファサイズを取得.
		int Size = WideCharToMultiByte(CP_THREAD_ACP, 0, reinterpret_cast<const wchar_t*>(UTF16.data()),
			static_cast<int>(UTF16.size()), nullptr, 0, nullptr, nullptr);

		if (Size == 0) {
			// 変換に失敗した場合のエラーハンドリング.
			DWORD error = GetLastError();
			std::cerr << "WideCharToMultiByte failed with error: " << error << std::endl;

			// 失敗した場合は空の文字列を返す.
			return std::string();
		}

		// 変換結果を格納するバッファを用意.
		std::string UTF8Buf(Size, 0);

		// 実際に変換を行う.
		if (WideCharToMultiByte(CP_THREAD_ACP, 0, reinterpret_cast<const wchar_t*>(UTF16.data()),
			static_cast<int>(UTF16.size()), &UTF8Buf[0], Size, nullptr, nullptr) == 0) {

			// 再度エラーハンドリング.
			DWORD error = GetLastError();
			std::cerr << "WideCharToMultiByte failed with error: " << error << std::endl;

			// 失敗した場合は空の文字列を返す.
			return std::string();
		}

		return UTF8Buf;
	}

	std::u16string UTF8ToUTF16(const std::string& UTF8)
	{

		// 変換後のバッファサイズを取得.
		int Size = MultiByteToWideChar(CP_THREAD_ACP, 0, UTF8.c_str(), static_cast<int>(UTF8.size()), nullptr, 0);

		if (Size == 0) {
			// 変換に失敗した場合のエラーハンドリング.
			DWORD error = GetLastError();
			std::cerr << "MultiByteToWideChar failed with error: " << error << std::endl;

			// 失敗した場合は空の文字列を返す.
			return std::u16string();
		}

		// 変換結果を格納するバッファを用意.
		std::u16string UTF16Buf(Size, 0);

		// 実際に変換を行う.
		int result = MultiByteToWideChar(CP_THREAD_ACP, 0, UTF8.c_str(), static_cast<int>(UTF8.size()),
			reinterpret_cast<wchar_t*>(&UTF16Buf[0]), Size);

		if (result == 0) {
			// 再度エラーハンドリング.
			DWORD error = GetLastError();
			std::cerr << "MultiByteToWideChar failed with error: " << error << std::endl;

			// 失敗した場合は空の文字列を返す.
			return std::u16string();
		}

		return UTF16Buf;
	}
}