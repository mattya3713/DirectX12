#include "Easing.h"
#pragma once

namespace MyEasing {
    template<typename T>
    void UpdateEasing(Type Type, float Time, float MaxTime, T Start, T End, T& Out)
    {
       switch (Type)
        {
            case MyEasing::Type::Liner:         Liner       (Time, MaxTime, Start, End, Out); break;
            case MyEasing::Type::InSine:        InSine      (Time, MaxTime, Start, End, Out); break;
            case MyEasing::Type::OutSine:       OutSine     (Time, MaxTime, Start, End, Out); break;
            case MyEasing::Type::InOutSine:     InOutSine   (Time, MaxTime, Start, End, Out); break;
            case MyEasing::Type::InQuad:        InQuad      (Time, MaxTime, Start, End, Out); break;
            case MyEasing::Type::OutQuad:       OutQuad     (Time, MaxTime, Start, End, Out); break;
            case MyEasing::Type::InOutQuad:     InOutQuad   (Time, MaxTime, Start, End, Out); break;
            case MyEasing::Type::InCubic:       InCubic     (Time, MaxTime, Start, End, Out); break;
            case MyEasing::Type::OutCubic:      OutCubic    (Time, MaxTime, Start, End, Out); break;
            case MyEasing::Type::InOutCubic:    InOutCubic  (Time, MaxTime, Start, End, Out); break;
            case MyEasing::Type::InQuart:       InQuart     (Time, MaxTime, Start, End, Out); break;
            case MyEasing::Type::OutQuart:      OutQuart    (Time, MaxTime, Start, End, Out); break;
            case MyEasing::Type::InOutQuart:    InOutQuart  (Time, MaxTime, Start, End, Out); break;
            case MyEasing::Type::InQuint:       InQuint     (Time, MaxTime, Start, End, Out); break;
            case MyEasing::Type::OutQuint:      OutQuint    (Time, MaxTime, Start, End, Out); break;
            case MyEasing::Type::InOutQuint:    InOutQuint  (Time, MaxTime, Start, End, Out); break;
            case MyEasing::Type::InExpo:        InExpo      (Time, MaxTime, Start, End, Out); break;
            case MyEasing::Type::OutExpo:       OutExpo     (Time, MaxTime, Start, End, Out); break;
            case MyEasing::Type::InOutExpo:     InOutExpo   (Time, MaxTime, Start, End, Out); break;
            case MyEasing::Type::InCirc:        InCirc      (Time, MaxTime, Start, End, Out); break;
            case MyEasing::Type::OutCirc:       OutCirc     (Time, MaxTime, Start, End, Out); break;
            case MyEasing::Type::InOutCirc:     InOutCirc   (Time, MaxTime, Start, End, Out); break;
            case MyEasing::Type::InBack:        InBack      (Time, MaxTime, Start, End, Out); break;
            case MyEasing::Type::OutBack:       OutBack     (Time, MaxTime, Start, End, Out); break;
            case MyEasing::Type::InOutBack:     InOutBack   (Time, MaxTime, Start, End, Out); break;
            case MyEasing::Type::InElastic:     InElastic   (Time, MaxTime, Start, End, Out); break;
            case MyEasing::Type::OutElastic:    OutElastic  (Time, MaxTime, Start, End, Out); break;
            case MyEasing::Type::InOutElastic:  InOutElastic(Time, MaxTime, Start, End, Out); break;
            case MyEasing::Type::InBounce:      InBounce    (Time, MaxTime, Start, End, Out); break;
            case MyEasing::Type::OutBounce:     OutBounce   (Time, MaxTime, Start, End, Out); break;
            case MyEasing::Type::InOutBounce:   InOutBounce (Time, MaxTime, Start, End, Out); break;
            default:                              Out = Start; // —áŠO‚Í“®‚©‚³‚È‚¢.
        }
    }

    const char* GetEasingTypeName(MyEasing::Type type)
    {
        switch (type)
        {
        case MyEasing::Type::Liner:         return "Liner";
        case MyEasing::Type::InSine:        return "InSine";
        case MyEasing::Type::OutSine:       return "OutSine";
        case MyEasing::Type::InOutSine:     return "InOutSine";
        case MyEasing::Type::InQuad:        return "InQuad";
        case MyEasing::Type::OutQuad:       return "OutQuad";
        case MyEasing::Type::InOutQuad:     return "InOutQuad";
        case MyEasing::Type::InCubic:       return "InCubic";
        case MyEasing::Type::OutCubic:      return "OutCubic";
        case MyEasing::Type::InOutCubic:    return "InOutCubic";
        case MyEasing::Type::InQuart:       return "InQuart";
        case MyEasing::Type::OutQuart:      return "OutQuart";
        case MyEasing::Type::InOutQuart:    return "InOutQuart";
        case MyEasing::Type::InQuint:       return "InQuint";
        case MyEasing::Type::OutQuint:      return "OutQuint";
        case MyEasing::Type::InOutQuint:    return "InOutQuint";
        case MyEasing::Type::InExpo:        return "InExpo";
        case MyEasing::Type::OutExpo:       return "OutExpo";
        case MyEasing::Type::InOutExpo:     return "InOutExpo";
        case MyEasing::Type::InCirc:        return "InCirc";
        case MyEasing::Type::OutCirc:       return "OutCirc";
        case MyEasing::Type::InOutCirc:     return "InOutCirc";
        case MyEasing::Type::InBack:        return "InBack";
        case MyEasing::Type::OutBack:       return "OutBack";
        case MyEasing::Type::InOutBack:     return "InOutBack";
        case MyEasing::Type::InElastic:     return "InElastic";
        case MyEasing::Type::OutElastic:    return "OutElastic";
        case MyEasing::Type::InOutElastic:  return "InOutElastic";
        case MyEasing::Type::InBounce:      return "InBounce";
        case MyEasing::Type::OutBounce:     return "OutBounce";
        case MyEasing::Type::InOutBounce:   return "InOutBounce";
        default:                            return "Unknown";
        }
    }

    template<typename T>
    void Liner(float Time, float MaxTime, T Start, T End, T& Out)
    {
        float t = Time / MaxTime; 
        Out = Start + (End - Start) * t;
    }

    template<typename T>
    void InSine(float Time, float MaxTime, T Start, T End, T& Out) {
        float t = Time / MaxTime;
        Out = Start + (End - Start) * (1 - std::cos(t * (M_PI * 0.5f)));
    }

    template<typename T>
    void OutSine(float Time, float MaxTime, T Start, T End, T& Out)
    {
        float t = Time / MaxTime;
        Out = Start + (End - Start) * std::sin(t * (M_PI * 0.5f));
    }

    template<typename T>
    void InOutSine(float Time, float MaxTime, T Start, T End, T& Out)
    {
        float t = Time / MaxTime;
        Out = Start + (End - Start) * (-(std::cos(M_PI * t) - 1) * 0.5f);
    }

    template<typename T>
    void InQuad(float Time, float MaxTime, T Start, T End, T& Out)
    {
        float t = Time / MaxTime;
        Out = Start + (End - Start) * t * t;
    }

    template<typename T>
    void OutQuad(float Time, float MaxTime, T Start, T End, T& Out)
    {
        float t = Time / MaxTime;
        Out = Start + (End - Start) * t * (2 - t);
    }

    template<typename T>
    void InOutQuad(float Time, float MaxTime, T Start, T End, T& Out)
    {
        float t = Time / MaxTime;
        if (t < 0.5f) Out = Start + (End - Start) * 2 * t * t;
        Out = Start + (End - Start) * (-1 + (4 - 2 * t) * t);
    }

    template<typename T>
    void InCubic(float Time, float MaxTime, T Start, T End, T& Out)
    {
        float t = Time / MaxTime;
        Out = Start + (End - Start) * t * t * t;
    }

    template<typename T>
    void OutCubic(float Time, float MaxTime, T Start, T End, T& Out)
    {
        float t = Time / MaxTime;
        t--;
        Out = Start + (End - Start) * (t * t * t + 1);
    }

    template<typename T>
    void InOutCubic(float Time, float MaxTime, T Start, T End, T& Out)
    {
        float t = Time / MaxTime;
        if (t < 0.5f) Out = Start + (End - Start) * 4 * t * t * t;
        t -= 1;
        Out = Start + (End - Start) * (t * t * t * 4 + 1);
    }

    template<typename T>
    void InQuart(float Time, float MaxTime, T Start, T End, T& Out)
    {
        float t = Time / MaxTime;
        Out = Start + (End - Start) * t * t * t * t;
    }

    template<typename T>
    void OutQuart(float Time, float MaxTime, T Start, T End, T& Out)
    {
        float t = Time / MaxTime;
        t--;
        Out = Start + (End - Start) * (1 - t * t * t * t);
    }

    template<typename T>
    void InOutQuart(float Time, float MaxTime, T Start, T End, T& Out)
    {
        float t = Time / MaxTime;
        if (t < 0.5f) Out = Start + (End - Start) * 8 * t * t * t * t;
        t -= 1;
        Out = Start + (End - Start) * (1 - t * t * t * t);
    }

    template<typename T>
    void InQuint(float Time, float MaxTime, T Start, T End, T& Out)
    {
        float t = Time / MaxTime;
        Out = Start + (End - Start) * t * t * t * t * t;
    }

    template<typename T>
    void OutQuint(float Time, float MaxTime, T Start, T End, T& Out)
    {
        float t = Time / MaxTime;
        t--;
        Out = Start + (End - Start) * (t * t * t * t * t + 1);
    }

    template<typename T>
    void InOutQuint(float Time, float MaxTime, T Start, T End, T& Out)
    {
        float t = Time / MaxTime;
        if (t < 0.5f) Out = Start + (End - Start) * 16 * t * t * t * t * t;
        t -= 1;
        Out = Start + (End - Start) * (t * t * t * t * t * 16 + 1);
    }

    template<typename T>
    void InExpo(float Time, float MaxTime, T Start, T End, T& Out)
    {
        float t = Time / MaxTime;
        Out = Start + (End - Start) * (t == 0 ? 0 : std::pow(2, 10 * (t - 1)));
    }

    template<typename T>
    void OutExpo(float Time, float MaxTime, T Start, T End, T& Out)
    {
        float t = Time / MaxTime;
        Out = Start + (End - Start) * (t == 1 ? 1 : 1 - std::pow(2, -10 * t));
    }

    template<typename T>
    void InOutExpo(float Time, float MaxTime, T Start, T End, T& Out)
    {
        float t = Time / MaxTime;
        if (t == 0) Out = Start;
        if (t == 1) Out = End;
        if (t < 0.5f) Out = Start + (End - Start) * 0.5f * std::pow(2, 10 * (2 * t - 1));
        Out = Start + (End - Start) * 0.5f * (2 - std::pow(2, -10 * (2 * t - 1)));
    }

    template<typename T>
    void InCirc(float Time, float MaxTime, T Start, T End, T& Out)
    {
        float t = Time / MaxTime;
        Out = Start + (End - Start) * (1 - std::sqrt(1 - t * t));
    }

    template<typename T>
    void OutCirc(float Time, float MaxTime, T Start, T End, T& Out)
    {
        float t = Time / MaxTime;
        t--;
        Out = Start + (End - Start) * std::sqrt(1 - t * t);
    }

    template<typename T>
    void InOutCirc(float Time, float MaxTime, T Start, T End, T& Out)
    {
        float t = Time / MaxTime;
        if (t < 0.5f) Out = Start + (End - Start) * 0.5f * (1 - std::sqrt(1 - 4 * t * t));
        t = t * 2 - 1;
        Out = Start + (End - Start) * 0.5f * (std::sqrt(1 - t * t) + 1);
    }

    template<typename T>
    void InBack(float Time, float MaxTime, T Start, T End, T& Out)
    {
        const float s = 1.70158f;
        float t = Time / MaxTime;
        Out = Start + (End - Start) * t * t * ((s + 1) * t - s);
    }

    template<typename T>
    void OutBack(float Time, float MaxTime, T Start, T End, T& Out)
    {
        const float s = 1.70158f;
        float t = Time / MaxTime;
        t--;
        Out = Start + (End - Start) * (t * t * ((s + 1) * t + s) + 1);
    }

    template<typename T>
    void InOutBack(float Time, float MaxTime, T Start, T End, T& Out)
    {
        const float s = 1.70158f * 1.525f;
        float t = Time / MaxTime;
        if (t < 0.5f) Out = Start + (End - Start) * 0.5f * (t * t * ((s + 1) * 2 * t - s));
        t = 2 * t - 1;
        Out = Start + (End - Start) * 0.5f * (t * t * ((s + 1) * t + s) + 1);
    }

    template<typename T>
    void InElastic(float Time, float MaxTime, T Start, T End, T& Out)
    {
        float t = Time / MaxTime;
        if (t == 0) Out = Start;
        if (t == 1) Out = End;
        Out = Start - (End - Start) * std::pow(2, 10 * (t - 1)) * std::sin((t - 1.1f) * 5 * M_PI);
    }

    template<typename T>
    void OutElastic(float Time, float MaxTime, T Start, T End, T& Out)
    {
        float t = Time / MaxTime;
        if (t == 0) Out = Start;
        if (t == 1) Out = End;
        Out = Start + (End - Start) * std::pow(2, -10 * t) * std::sin((t - 0.1f) * 5 * M_PI) + (End - Start);
    }

    template<typename T>
    void InOutElastic(float Time, float MaxTime, T Start, T End, T& Out)
    {
        float t = Time / MaxTime;
        if (t == 0) Out = Start;
        if (t == 1) Out = End;
        if (t < 0.5f) {
            t *= 2;
            Out = Start - (End - Start) * 0.5f * std::pow(2, 10 * (t - 1)) * std::sin((t - 1.1f) * 5 * M_PI);
        }
        else
        {
            t = t * 2 - 1;
            Out = Start + (End - Start) * 0.5f * std::pow(2, -10 * t) * std::sin((t - 0.1f) * 5 * M_PI);
        }
    }

    template<typename T>
    void InBounce(float Time, float MaxTime, T Start, T End, T& Out)
    {
        T Emp;
        OutBounce(MaxTime - Time, MaxTime, Start, End, Emp);
        Out = Start + Emp;
    }

    template<typename T>
    void OutBounce(float Time, float MaxTime, T Start, T End, T& Out)
    {
        float t = Time / MaxTime;
        if (t < 1 / 2.75f) {
            Out = Start + (End - Start) * (7.5625f * t * t);
        }
        else if (t < 2 / 2.75f) {
            t -= 1.5f / 2.75f;
            Out = Start + (End - Start) * (7.5625f * t * t + 0.75f);
        }
        else if (t < 2.5f / 2.75f) {
            t -= 2.25f / 2.75f;
            Out = Start + (End - Start) * (7.5625f * t * t + 0.9375f);
        }
        else {
            t -= 2.625f / 2.75f;
            Out = Start + (End - Start) * (7.5625f * t * t + 0.984375f);
        }
    }

    template<typename T>
    void InOutBounce(float Time, float MaxTime, T Start, T End, T& Out)
    {
        if (Time < MaxTime * 0.5f) {
            T HalfOut;
            InBounce(Time * 2, MaxTime, Start, End * 0.5f, HalfOut);
            Out = HalfOut;
        }
        else {
            T HalfOut;
            OutBounce(Time * 2 - MaxTime, MaxTime, Start + End * 0.5f, End * 0.5f, HalfOut);
            Out = HalfOut;
        }
    }
}