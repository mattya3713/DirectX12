/****************************
*	痒い所に手を届かせる系の計算.h.
*   担当:淵脇 未来
****/
#pragma once
#include <cmath>

namespace MyMath {
    //-----------------
    // クランプ : 値を範囲に収める.
    template <typename T>
    T Clamp(const T& Value, const T& Min, const T& Max);

    // 絶対値クランプ.
    template <typename T>
    T Clamp(const T& Value, const T& MaxAbs);

    //-----------------
    // 値を減少(増加)させる.
    template<typename T>
    void DecreaseToValue(T& Value, T Median, T Amount);
}

#include "Math.inl" // インライン実装を含むファイルをインクルード.