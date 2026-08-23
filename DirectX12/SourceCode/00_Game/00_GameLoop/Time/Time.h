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

	// 一時停止状態を設定する.
	static void SetPaused(const bool IsPaused);

	// 一時停止中かを取得する.
	static const bool IsPaused();

	// 一時的な時間スケールを設定する(Duration秒経過後、自動的に通常速度へ戻る.
	// ヒットストップ等の演出用. GetDeltaTimeへグローバルに乗算される).
	static void SetTimeScale(const float Scale, const float Duration);

	// 現在の時間スケールを取得する.
	static const float GetTimeScale();
private:
	// 前フレームの時間.
	std::chrono::time_point<std::chrono::high_resolution_clock> m_PreviousTime;

	float m_TargetFrameTime;// 目標フレーム時間(秒).
	float m_DeltaTime;		// フレーム間の時間差(実時間. 時間スケールはGetDeltaTimeで乗算).
	bool m_IsPaused;		// 一時停止中か.
	float m_TimeScale = 1.0f;         // 現在の時間スケール(1=通常速度).
	float m_TimeScaleRemaining = 0.0f;// 時間スケールの残り時間(秒. 実時間で減算).
};
