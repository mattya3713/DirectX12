#pragma once
#include "Math.h"

namespace MyMath {
    //-----------------
    // クランプ : 値を範囲に収める.

    // 通常クランプ.
    template <typename T>
    static inline T Clamp(const T& Value, const T& Min, const T& Max) {
        return std::max(Min, std::min(Value, Max));
    }

    // 絶対値クランプ.
    template <typename T>
    static inline T Clamp(const T& Value, const T& MaxAbs) {
        return std::max(MaxAbs * -1.f , std::min(Value, MaxAbs));
    }

    //-----------------
    // 値を減少(増加)させる.
    template<typename T>
    static inline void DecreaseToValue(T& Value, T Median, T Amount)
    {
        if (std::abs(Value - Median) <= Amount) 
        {
            Value = Median;
            return;
        }

        if (Value >= Median) 
        {
            Value -= Amount;
        }
        else 
        {
            Value += Amount;
        }
    }
}