#pragma once

#include "json/json.hpp"

/**********************************************************************************
* @date      : 2026-08-23.
* @brief     : 汎用シリアライズインターフェース.
*            : 「このオブジェクトは自分をJSONへ出し入れできる」ことを表す最小限の
*            : ペアのみを規定する。実装側は自クラスの全永続化対象メンバを
*            : Serialize()で書き出し、Deserialize()で同じキーから読み戻すこと.
**********************************************************************************/

class ISerializable
{
public:
	virtual ~ISerializable() = default;

	// 自分の状態をJSONへ書き出す(キー名は実装クラスが定義する).
	virtual nlohmann::json Serialize() const = 0;

	// JSONから自分の状態を読み戻す(存在しないキーは既定値のままにする等の方針も実装クラスが決める).
	virtual void Deserialize(const nlohmann::json& Data) = 0;
};
