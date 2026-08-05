#pragma once

#include<string>
#include"ModelData.h"

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/03.
* @brief     : モデルファイルを読み込み、共通データ(Model::ModelData)へ変換する責務を持つインターフェイス.
*              新しいモデルフォーマットに対応する場合は、このインターフェイスを実装したパーサーを追加する.
**********************************************************************************/

class IModelParser
{
public:
	virtual ~IModelParser() = default;

	// FilePathのモデルファイルを読み込み、OutDataへ変換結果を格納する.
	// 成功時はtrue、失敗時はfalseを返す.
	virtual bool Load(const std::string& FilePath, Model::ModelData& OutData) = 0;
};
