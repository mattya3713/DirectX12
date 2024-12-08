/****************************
*	ファイルパスのあれそれ.
*   担当:淵脇 未来
****/
#pragma once
#include <string>   // string, wstring
#include <utility>  // pair

namespace MyFilePath {
	// 定数としてセパレーターを定義
	constexpr char DEFAULT_SPLITTER = '*';

	/*******************************************
	* テクスチャのパスをセパレータ文字で分離する.
	*　@param	Path	: 対象のパス文字列.
	*　@param	Splitter: 区切り文字.
	*　@return			: 分離前後の文字列ペア.
	*******************************************/
	static inline std::pair<std::string, std::string> SplitFileName(const std::string& Path, const char Splitter = DEFAULT_SPLITTER);

	/*******************************************
	* ファイル名から拡張子を取得する.
	* @param	Path	: 対象のパス文字列.
	* @return			: 拡張子.
	*******************************************/
	static inline std::string GetExtension(const std::string& Path);
	static inline std::wstring GetExtension(const std::wstring& Path);

	/*******************************************
	* @brief			: モデルからの相対テクスチャパスをアプリから見たパスをに変換,取得する.
	* @param ModelPath	: アプリから見たpmdモデルのパス.
	* @param TexPath	: PMDモデルから見たテクスチャのパス.
	* @return			: アプリから見たテクスチャのパス.
	*******************************************/
	static inline std::string GetTexPath(const std::string& ModelPath, const char* TexPath);
	
	/*******************************************
	* @brief			: ファイルパスなどの/を\\に変換.
	* @param Path		: アプリから見たpmdモデルのパス.
	*******************************************/
	static inline void ReplaceSlashWithBackslash(std::string* Path);

}

#include "FilePath.inl" // インライン実装を含むファイルをインクルード.