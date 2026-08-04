#pragma once
#include <chrono>

/************************
*   タイムクラス.
************************/
class GameTime
{
public:
	~GameTime();

	// インスタンスを取得.
	static GameTime* GetInstance();

	// フレーム間の経過時間を更新.
	static void Update();

	// FPSを維持するための処理.
	static void MaintainFPS();

	// デルタタイムを取得.
	static const float GetDeltaTime();
private:
	GameTime();

	// 生成やコピーを削除.
	GameTime(const GameTime& rhs)					= delete;
	GameTime& operator = (const GameTime& rhs)	= delete;
private:
	// 前フレームの時間.
	std::chrono::time_point<std::chrono::high_resolution_clock> m_PreviousTime;

	float m_TargetFrameTime;// 目標フレーム時間(秒).
	float m_DeltaTime;		// フレーム間の時間差.
};
