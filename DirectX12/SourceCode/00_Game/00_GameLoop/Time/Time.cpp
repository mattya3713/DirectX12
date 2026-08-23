#include "Time.h"
#include "99_Utility/ServiceLocator/ServiceLocator.h"
#include <thread>

constexpr float TAEGET_FPS = 60.0f;//目標フレーム.

GameTime::GameTime()
    : m_PreviousTime    {}
    , m_TargetFrameTime {}
    , m_DeltaTime       {}
    , m_IsPaused        {}
{
    m_TargetFrameTime   = 1.0f / TAEGET_FPS; // 目標フレームを計算.
    m_PreviousTime      = std::chrono::high_resolution_clock::now();//初期を取得.
}

GameTime::~GameTime()
{
}

// フレーム間の経過時間を更新.
void GameTime::Update()
{
    // インスタンスを取得.
    GameTime* pI = ServiceLocator::Get<GameTime>();

    // 現在の時間を取得.
    auto currentTime = std::chrono::high_resolution_clock::now();

    // 前回からの経過時間を計算.
    std::chrono::duration<float> elapsed = currentTime - pI->m_PreviousTime;

    // 経過時間を秒単位で保持.
    pI->m_DeltaTime = elapsed.count();

    // 時間スケールの残り時間を実時間で減算し、期限切れなら通常速度へ戻す.
    if (pI->m_TimeScaleRemaining > 0.0f) {
        pI->m_TimeScaleRemaining -= pI->m_DeltaTime;
        if (pI->m_TimeScaleRemaining <= 0.0f) {
            pI->m_TimeScaleRemaining = 0.0f;
            pI->m_TimeScale          = 1.0f;
        }
    }

    // 次のフレームのために更新.
    pI->m_PreviousTime = currentTime;
}

// FPSを維持するための処理.
void GameTime::MaintainFPS()
{
    // インスタンスを取得.
    GameTime* pI = ServiceLocator::Get<GameTime>();

    if (pI->m_DeltaTime < pI->m_TargetFrameTime) {
        pI->m_DeltaTime = pI->m_TargetFrameTime;
       /* std::this_thread::sleep_for(
            std::chrono::duration<float>(pI->m_TargetFrameTime - pI->m_DeltaTime));*/
    }
}

// デルタタイムを取得.
const float GameTime::GetDeltaTime()
{
    // 時間スケールを乗算して返す(ヒットストップ/スローモーション等の演出用).
    // MaintainFPSによるフレームペーシングは実時間側で行われるため干渉しない.
    GameTime* pI = ServiceLocator::Get<GameTime>();
    return pI->m_DeltaTime * pI->m_TimeScale;
}

// 一時的な時間スケールを設定する.
void GameTime::SetTimeScale(const float Scale, const float Duration)
{
    GameTime* pI = ServiceLocator::Get<GameTime>();

    if (Scale > 0.0f) { pI->m_TimeScale = Scale; }
    pI->m_TimeScaleRemaining = (Duration > 0.0f) ? Duration : 0.0f;
    if (pI->m_TimeScaleRemaining <= 0.0f) { pI->m_TimeScale = 1.0f; }
}

// 現在の時間スケールを取得する.
const float GameTime::GetTimeScale()
{
    return ServiceLocator::Get<GameTime>()->m_TimeScale;
}

// 一時停止状態を設定する.
void GameTime::SetPaused(const bool IsPaused)
{
    ServiceLocator::Get<GameTime>()->m_IsPaused = IsPaused;
}

// 一時停止中かを取得する.
const bool GameTime::IsPaused()
{
    return ServiceLocator::Get<GameTime>()->m_IsPaused;
}
