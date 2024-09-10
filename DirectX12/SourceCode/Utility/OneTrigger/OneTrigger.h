#pragma once

#include <functional>

/****************************
*	一回だけ処理を通すクラス.
*   担当:淵脇未来.
****/


class SingleTrigger
{
private:
    bool _triggered;

public:
    SingleTrigger() : _triggered(false) {}

    // Action.
    template<typename Once, typename ResetFunc>
    void CheckAndTrigger(Once OnceFunc, ResetFunc Reset)
    {
        if (!_triggered)
        {
            // 条件を満たしたときに実行する処理.
            _triggered = true;
            OnceFunc();
        }
        else if (_triggered == true && Reset())
        {
            // 条件がリセットされたら、再度実行可能にする.
            _triggered = false;
        }
    }
};