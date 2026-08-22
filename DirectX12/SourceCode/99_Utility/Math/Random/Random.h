#pragma once
#include <random>
namespace MyRand
{
    inline int GetRandomPercentage(int Min,int Max)
    {
        // エンジンはプログラム全体で1つを使い回す.
        static std::mt19937 rnd{ std::random_device{}() };

        // distributionは呼び出しごとにMin/Maxで構築し直す(staticにすると初回の範囲で固定されるため).
        std::uniform_int_distribution<> dis{ Min, Max };
        return dis(rnd);
    }

    inline float GetRandomPercentage(float Min, float Max)
    {
        // エンジンはプログラム全体で1つを使い回す.
        static std::mt19937 rnd{ std::random_device{}() };

        // distributionは呼び出しごとにMin/Maxで構築し直す(staticにすると初回の範囲で固定されるため).
        std::uniform_real_distribution<float> dis{ Min, Max };
        return dis(rnd);
    }

    // 指定した値をランダムで返す.
    inline int GetRandomValue(const std::vector<int> values) {

        // 乱数生成器のシードとして乱数デバイスを使う.
        std::random_device rd;
        std::mt19937 gen(rd());

        // ランダムなインデックを作成.
        std::uniform_int_distribution<> distrib(0, static_cast<int>(values.size()) - 1);

        return values[distrib(gen)];
    }
}
