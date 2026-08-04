#include "Time.h"
#include <thread>

constexpr float TAEGET_FPS = 60.0f;//目標フレーム.

GameTime::GameTime()
    : m_PreviousTime    ()
    , m_TargetFrameTime ()
    , m_DeltaTime       ()
{
    m_TargetFrameTime   = 1.0f / TAEGET_FPS; // 目標フレームを計算.
    m_PreviousTime      = std::chrono::high_resolution_clock::now();//初期を取得.
}

GameTime::~GameTime()
{
}

// インスタンスを取得.
GameTime* GameTime::GetInstance()
{
    static GameTime instance;
    return &instance;
}

// フレーム間の経過時間を更新.
void GameTime::Update()
{
    // インスタンスを取得.
    GameTime* pI = GetInstance();

    // 現在の時間を取得.
    auto currentTime = std::chrono::high_resolution_clock::now();

    // 前回からの経過時間を計算.
    std::chrono::duration<float> elapsed = currentTime - pI->m_PreviousTime;

    // 経過時間を秒単位で保持.
    pI->m_DeltaTime = elapsed.count();

    // 次のフレームのために更新.
    pI->m_PreviousTime = currentTime;
}

// FPSを維持するための処理.
void GameTime::MaintainFPS()
{
    // インスタンスを取得.
    GameTime* pI = GetInstance();

    if (pI->m_DeltaTime < pI->m_TargetFrameTime) {
        pI->m_DeltaTime = pI->m_TargetFrameTime;
       /* std::this_thread::sleep_for(
            std::chrono::duration<float>(pI->m_TargetFrameTime - pI->m_DeltaTime));*/
    }
}

// デルタタイムを取得.
const float GameTime::GetDeltaTime()
{
    return GetInstance()->m_DeltaTime;
}
