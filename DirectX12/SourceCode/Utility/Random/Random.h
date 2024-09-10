#pragma once
#include <random>
namespace MyRand
{
    inline int GetRandomPercentage(int Min,int Max)
    {
        static bool isInitialized = false;
        static std::random_device dev{};
        static std::mt19937 rnd{ dev() };
        static std::uniform_int_distribution dis{ Min, Max };

        if (isInitialized == false)
        {
            rnd.seed(static_cast<unsigned>(
                std::chrono::steady_clock::now().time_since_epoch().count()));
            isInitialized = true;
        }
        return dis(rnd);
    }

    inline float GetRandomPercentage(float Min, float Max)
    {
        static bool isInitialized = false;
        static std::random_device dev{};
        static std::mt19937 rnd{ dev() };
        static std::uniform_real_distribution<float> dis{ Min, Max };

        if (isInitialized == false)
        {
            rnd.seed(static_cast<unsigned>(
                std::chrono::steady_clock::now().time_since_epoch().count()));
            isInitialized = true;
        }
        return dis(rnd);
    }
}