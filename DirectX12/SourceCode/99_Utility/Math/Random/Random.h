#pragma once
#include <random>
#include <vector>

namespace MyRand
{
    namespace detail
    {
        // 乱数エンジンはプログラム全体で1つを使い回す(inline関数のstaticのため複数TUでも共有).
        inline std::mt19937& Engine()
        {
            static std::mt19937 engine{ std::random_device{}() };
            return engine;
        }
    }

    inline int GetRandomPercentage(int Min,int Max)
    {
        // distributionは呼び出しごとにMin/Maxで構築し直す(staticにすると初回の範囲で固定されるため).
        std::uniform_int_distribution<> dis{ Min, Max };
        return dis(detail::Engine());
    }

    inline float GetRandomPercentage(float Min, float Max)
    {
        // distributionは呼び出しごとにMin/Maxで構築し直す(staticにすると初回の範囲で固定されるため).
        std::uniform_real_distribution<float> dis{ Min, Max };
        return dis(detail::Engine());
    }

    // 指定した値をランダムで返す.
    inline int GetRandomValue(const std::vector<int> values) {

        if (values.empty()) { return 0; } // 空ベクタのインデックス参照を防止.

        // ランダムなインデックスを作成(エンジンは共有. 毎回random_deviceを叩かない).
        std::uniform_int_distribution<> distrib(0, static_cast<int>(values.size()) - 1);

        return values[distrib(detail::Engine())];
    }
}
