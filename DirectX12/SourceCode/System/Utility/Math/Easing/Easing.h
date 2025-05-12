/****************************
* イージング関数をまとめた系.h.
* 担当:淵脇 未来
****/
#pragma once

#include <cmath>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace MyEasing {
    //-----------------
    // https://easings.net/ja
    // チートシート.

    enum class Type
    {
        Liner,
        InSine,
        OutSine,
        InOutSine,
        InQuad,
        OutQuad,
        InOutQuad,
        InCubic,
        OutCubic,
        InOutCubic,
        InQuart,
        OutQuart,
        InOutQuart,
        InQuint,
        OutQuint,
        InOutQuint,
        InExpo,
        OutExpo,
        InOutExpo,
        InCirc,
        OutCirc,
        InOutCirc,
        InBack,
        OutBack,
        InOutBack,
        InElastic,
        OutElastic,
        InOutElastic,
        InBounce,
        OutBounce,
        InOutBounce

    };

    /*******************************************
    * @brief  対応したEasing関数を実行.
    * @param  Type     : Easingのタイプ.
    * @param  Time     : 経過時間(フレーム).
    * @param  MaxTime  : アニメーションにかかる時間(フレーム).
    * @param  Start    : 始まりの値.
    * @param  End      : 終わりの値.
    * @return          : 更新した値.
    *******************************************/
    template<typename T>
    inline void UpdateEasing(Type Type, float Time, float MaxTime, T Start, T End, T& Out);
    /*******************************************
    * @brief  対応したEasingを文字列として変換.
    * @return          : Easingの名前.
    *******************************************/
    inline const char* GetEasingTypeName(MyEasing::Type type);

    /*******************************************
    * @brief  等速直線運動.
    * @param  Time     : 経過時間(フレーム).
    * @param  MaxTime  : アニメーションにかかる時間(フレーム).
    * @param  Start    : 始まりの値.
    * @param  End      : 終わりの値.
    * @param  Out       : 求まった値.
    *******************************************/
    template<typename T>
    inline void Liner(float Time, float MaxTime, T Start, T End, T& Out);

    /*******************************************
    * @brief  最初はゆっくり、後半は速く加速する.
    * @param  Time     : 経過時間(フレーム).
    * @param  MaxTime  : アニメーションにかかる時間(フレーム).
    * @param  Start    : 始まりの値.
    * @param  End      : 終わりの値.
    * @param  Out       : 求まった値.
    *******************************************/
    template<typename T>
    inline void InSine(float Time, float MaxTime, T Start, T End, T& Out);

    /*******************************************
    * @brief  最初は速く、徐々に減速し終わりがゆっくりになる.
    * @param  Time     : 経過時間(フレーム).
    * @param  MaxTime  : アニメーションにかかる時間(フレーム).
    * @param  Start    : 始まりの値.
    * @param  End      : 終わりの値.
    * @param  Out       : 求まった値.
    *******************************************/
    template<typename T>
    inline void OutSine(float Time, float MaxTime, T Start, T End, T& Out);

    /*******************************************
    * @brief  始めと終わりがゆっくりで、中間が速い動き.
    * @param  Time     : 経過時間(フレーム).
    * @param  MaxTime  : アニメーションにかかる時間(フレーム).
    * @param  Start    : 始まりの値.
    * @param  End      : 終わりの値.
    * @param  Out       : 求まった値.
    *******************************************/
    template<typename T>
    inline void InOutSine(float Time, float MaxTime, T Start, T End, T& Out);

    /*******************************************
    * @brief  加速しながら動く、最初は遅く後半で急激に速くなる.
    * @param  Time     : 経過時間(フレーム).
    * @param  MaxTime  : アニメーションにかかる時間(フレーム).
    * @param  Start    : 始まりの値.
    * @param  End      : 終わりの値.
    * @param  Out       : 求まった値.
    *******************************************/
    template<typename T>
    inline void InQuad(float Time, float MaxTime, T Start, T End, T& Out);

    /*******************************************
    * @brief  最初は速く、後半で急激に減速する.
    * @param  Time     : 経過時間(フレーム).
    * @param  MaxTime  : アニメーションにかかる時間(フレーム).
    * @param  Start    : 始まりの値.
    * @param  End      : 終わりの値.
    * @param  Out       : 求まった値.
    *******************************************/
    template<typename T>
    inline void OutQuad(float Time, float MaxTime, T Start, T End, T& Out);

    /*******************************************
    * @brief  最初と最後でゆっくりし、中間で急速に動く.
    * @param  Time     : 経過時間(フレーム).
    * @param  MaxTime  : アニメーションにかかる時間(フレーム).
    * @param  Start    : 始まりの値.
    * @param  End      : 終わりの値.
    * @param  Out       : 求まった値.
    *******************************************/
    template<typename T>
    inline void InOutQuad(float Time, float MaxTime, T Start, T End, T& Out);

    /*******************************************
    * @brief  最初は非常にゆっくり、後半で急速に動く.
    * @param  Time     : 経過時間(フレーム).
    * @param  MaxTime  : アニメーションにかかる時間(フレーム).
    * @param  Start    : 始まりの値.
    * @param  End      : 終わりの値.
    * @param  Out       : 求まった値.
    *******************************************/
    template<typename T>
    inline void InCubic(float Time, float MaxTime, T Start, T End, T& Out);

    /*******************************************
    * @brief  最初に急速に動き、後半はゆっくり減速する.
    * @param  Time     : 経過時間(フレーム).
    * @param  MaxTime  : アニメーションにかかる時間(フレーム).
    * @param  Start    : 始まりの値.
    * @param  End      : 終わりの値.
    * @param  Out       : 求まった値.
    *******************************************/
    template<typename T>
    inline void OutCubic(float Time, float MaxTime, T Start, T End, T& Out);

    /*******************************************
    * @brief  始めと終わりはゆっくりで、中間は急速に動く.
    * @param  Time     : 経過時間(フレーム).
    * @param  MaxTime  : アニメーションにかかる時間(フレーム).
    * @param  Start    : 始まりの値.
    * @param  End      : 終わりの値.
    * @param  Out       : 求まった値.
    *******************************************/
    template<typename T>
    inline void InOutCubic(float Time, float MaxTime, T Start, T End, T& Out);

    /*******************************************
    * @brief  徐々に加速し、終わり際に急速に動く.
    * @param  Time     : 経過時間(フレーム).
    * @param  MaxTime  : アニメーションにかかる時間(フレーム).
    * @param  Start    : 始まりの値.
    * @param  End      : 終わりの値.
    * @param  Out       : 求まった値.
    *******************************************/
    template<typename T>
    inline void InQuart(float Time, float MaxTime, T Start, T End, T& Out);

    /*******************************************
    * @brief  最初に急速に動き、後半は徐々に減速する.
    * @param  Time     : 経過時間(フレーム).
    * @param  MaxTime  : アニメーションにかかる時間(フレーム).
    * @param  Start    : 始まりの値.
    * @param  End      : 終わりの値.
    * @param  Out       : 求まった値.
    *******************************************/
    template<typename T>
    inline void OutQuart(float Time, float MaxTime, T Start, T End, T& Out);

    /*******************************************
    * @brief  最初と最後がゆっくりで、中間が非常に速くなる.
    * @param  Time     : 経過時間(フレーム).
    * @param  MaxTime  : アニメーションにかかる時間(フレーム).
    * @param  Start    : 始まりの値.
    * @param  End      : 終わりの値.
    * @param  Out       : 求まった値.
    *******************************************/
    template<typename T>
    inline void InOutQuart(float Time, float MaxTime, T Start, T End, T& Out);

    /*******************************************
    * @brief  最初は非常にゆっくり、後半で急激に加速する.
    * @param  Time     : 経過時間(フレーム).
    * @param  MaxTime  : アニメーションにかかる時間(フレーム).
    * @param  Start    : 始まりの値.
    * @param  End      : 終わりの値.
    * @param  Out       : 求まった値.
    *******************************************/
    template<typename T>
    inline void InQuint(float Time, float MaxTime, T Start, T End, T& Out);

    /*******************************************
    * @brief  最初に急激に動き、後半は非常にゆっくりになる.
    * @param  Time     : 経過時間(フレーム).
    * @param  MaxTime  : アニメーションにかかる時間(フレーム).
    * @param  Start    : 始まりの値.
    * @param  End      : 終わりの値.
    * @param  Out       : 求まった値.
    *******************************************/
    template<typename T>
    inline void OutQuint(float Time, float MaxTime, T Start, T End, T& Out);

    /*******************************************
    * @brief  始めと終わりはゆっくりで、中間は急激に動く.
    * @param  Time     : 経過時間(フレーム).
    * @param  MaxTime  : アニメーションにかかる時間(フレーム).
    * @param  Start    : 始まりの値.
    * @param  End      : 終わりの値.
    * @param  Out       : 求まった値.
    *******************************************/
    template<typename T>
    inline void InOutQuint(float Time, float MaxTime, T Start, T End, T& Out);

    /*******************************************
    * @brief  最初はほとんど動かず、後半で急速に動き始める.
    * @param  Time     : 経過時間(フレーム).
    * @param  MaxTime  : アニメーションにかかる時間(フレーム).
    * @param  Start    : 始まりの値.
    * @param  End      : 終わりの値.
    * @param  Out       : 求まった値.
    *******************************************/
    template<typename T>
    inline void InExpo(float Time, float MaxTime, T Start, T End, T& Out);

    /*******************************************
    * @brief  最初に急速に動き、後半は急激に減速する.
    * @param  Time     : 経過時間(フレーム).
    * @param  MaxTime  : アニメーションにかかる時間(フレーム).
    * @param  Start    : 始まりの値.
    * @param  End      : 終わりの値.
    * @param  Out       : 求まった値.
    *******************************************/
    template<typename T>
    inline void OutExpo(float Time, float MaxTime, T Start, T End, T& Out);

    /*******************************************
    * @brief  始めと終わりがゆっくりで、中間は急激に動く.
    * @param  Time     : 経過時間(フレーム).
    * @param  MaxTime  : アニメーションにかかる時間(フレーム).
    * @param  Start    : 始まりの値.
    * @param  End      : 終わりの値.
    * @param  Out       : 求まった値.
    *******************************************/
    template<typename T>
    inline void InOutExpo(float Time, float MaxTime, T Start, T End, T& Out);

    /*******************************************
    * @brief  最初はゆっくり動き、後半は急激に加速し、最後に減速する.
    * @param  Time     : 経過時間(フレーム).
    * @param  MaxTime  : アニメーションにかかる時間(フレーム).
    * @param  Start    : 始まりの値.
    * @param  End      : 終わりの値.
    * @param  Out       : 求まった値.
    *******************************************/
    template<typename T>
    inline void InCirc(float Time, float MaxTime, T Start, T End, T& Out);

    /*******************************************
    * @brief  最初に急速に動き、後半でゆっくり動く.
    * @param  Time     : 経過時間(フレーム).
    * @param  MaxTime  : アニメーションにかかる時間(フレーム).
    * @param  Start    : 始まりの値.
    * @param  End      : 終わりの値.
    * @param  Out       : 求まった値.
    *******************************************/
    template<typename T>
    inline void OutCirc(float Time, float MaxTime, T Start, T End, T& Out);

    /*******************************************
    * @brief  始めはゆっくり、後半は急速に加速し、最後に再び減速する.
    * @param  Time     : 経過時間(フレーム).
    * @param  MaxTime  : アニメーションにかかる時間(フレーム).
    * @param  Start    : 始まりの値.
    * @param  End      : 終わりの値.
    * @param  Out       : 求まった値.
    *******************************************/
    template<typename T>
    inline void InOutCirc(float Time, float MaxTime, T Start, T End, T& Out);

    /*******************************************
    * @brief  始めに動きが少なく、後半で急速に動き始める、再び減速する.
    * @param  Time     : 経過時間(フレーム).
    * @param  MaxTime  : アニメーションにかかる時間(フレーム).
    * @param  Start    : 始まりの値.
    * @param  End      : 終わりの値.
    * @param  Out       : 求まった値.
    *******************************************/
    template<typename T>
    inline void InBack(float Time, float MaxTime, T Start, T End, T& Out);

    /*******************************************
    * @brief  始めは急速に動き、後半で動きが減速する.
    * @param  Time     : 経過時間(フレーム).
    * @param  MaxTime  : アニメーションにかかる時間(フレーム).
    * @param  Start    : 始まりの値.
    * @param  End      : 終わりの値.
    * @param  Out       : 求まった値.
    *******************************************/
    template<typename T>
    inline void OutBack(float Time, float MaxTime, T Start, T End, T& Out);

    /*******************************************
    * @brief  始めと終わりがゆっくりで、中間が急激に加速し、最後に再び減速する.
    * @param  Time     : 経過時間(フレーム).
    * @param  MaxTime  : アニメーションにかかる時間(フレーム).
    * @param  Start    : 始まりの値.
    * @param  End      : 終わりの値.
    * @param  Out       : 求まった値.
    *******************************************/
    template<typename T>
    inline void InOutBack(float Time, float MaxTime, T Start, T End, T& Out);

    /*******************************************
    * @brief  最初は少し動き、後半で弾むような動きになる.
    * @param  Time     : 経過時間(フレーム).
    * @param  MaxTime  : アニメーションにかかる時間(フレーム).
    * @param  Start    : 始まりの値.
    * @param  End      : 終わりの値.
    * @param  Out       : 求まった値.
    *******************************************/
    template<typename T>
    inline void InElastic(float Time, float MaxTime, T Start, T End, T& Out);

    /*******************************************
    * @brief  最初は急速に動き、後半で弾むような動きになる.
    * @param  Time     : 経過時間(フレーム).
    * @param  MaxTime  : アニメーションにかかる時間(フレーム).
    * @param  Start    : 始まりの値.
    * @param  End      : 終わりの値.
    * @param  Out       : 求まった値.
    *******************************************/
    template<typename T>
    inline void OutElastic(float Time, float MaxTime, T Start, T End, T& Out);

    /*******************************************
    * @brief  始めはゆっくりで、中間で弾むような動きになり、最後に再びゆっくり減速する.
    * @param  Time     : 経過時間(フレーム).
    * @param  MaxTime  : アニメーションにかかる時間(フレーム).
    * @param  Start    : 始まりの値.
    * @param  End      : 終わりの値.
    * @param  Out       : 求まった値.
    *******************************************/
    template<typename T>
    inline void InOutElastic(float Time, float MaxTime, T Start, T End, T& Out);

    /*******************************************
    * @brief  最初はほとんど動かず、後半で跳ねるような動きになる.
    * @param  Time     : 経過時間(フレーム).
    * @param  MaxTime  : アニメーションにかかる時間(フレーム).
    * @param  Start    : 始まりの値.
    * @param  End      : 終わりの値.
    * @param  Out       : 求まった値.
    *******************************************/
    template<typename T>
    inline void InBounce(float Time, float MaxTime, T Start, T End, T& Out);

    /*******************************************
    * @brief  最初に急速に動き、後半で跳ねるような動きになる.
    * @param  Time     : 経過時間(フレーム).
    * @param  MaxTime  : アニメーションにかかる時間(フレーム).
    * @param  Start    : 始まりの値.
    * @param  End      : 終わりの値.
    * @param  Out       : 求まった値.
    *******************************************/
    template<typename T>
    inline void OutBounce(float Time, float MaxTime, T Start, T End, T& Out);

    /*******************************************
    * @brief  始めと終わりがゆっくりで、中間が跳ねるような動きになる.
    * @param  Time     : 経過時間(フレーム).
    * @param  MaxTime  : アニメーションにかかる時間(フレーム).
    * @param  Start    : 始まりの値.
    * @param  End      : 終わりの値.
    * @param  Out       : 求まった値.
    *******************************************/
    template<typename T>
    inline void InOutBounce(float Time, float MaxTime, T Start, T End, T& Out);

}
#include "Easing.inl" 