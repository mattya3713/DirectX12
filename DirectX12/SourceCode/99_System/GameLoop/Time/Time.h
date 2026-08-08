#pragma once
#include <chrono>

/************************
*   タイムクラス.
*   ServiceLocatorへ登録して使う想定(Mainが所有・登録する).
************************/
class GameTime final
{
public:
	GameTime();
	~GameTime();

	GameTime(const GameTime&)            = delete;
	GameTime& operator=(const GameTime&) = delete;

	// フレーム間の経過時間を更新.
	static void Update();

	// FPSを維持するための処理.
	static void MaintainFPS();

	// デルタタイムを取得.
	static const float GetDeltaTime();
private:
	// 前フレームの時間.
	std::chrono::time_point<std::chrono::high_resolution_clock> m_PreviousTime;

	float m_TargetFrameTime;// 目標フレーム時間(秒).
	float m_DeltaTime;		// フレーム間の時間差.
};
