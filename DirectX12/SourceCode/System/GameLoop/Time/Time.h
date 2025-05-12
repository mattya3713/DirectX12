#pragma once
#include <chrono>

/************************
*   タイムクラス.
************************/
class CTime
{
public:
	~CTime();

	// インスタンスを取得.
	static CTime* GetInstance();

	// フレーム間の経過時間を更新.
	static void Update();

	// FPSを維持するための処理.
	static void MaintainFPS();

	// デルタタイムを取得.
	static const float GetDeltaTime();
private:
	CTime();

	// 生成やコピーを削除.
	CTime(const CTime& rhs)					= delete;
	CTime& operator = (const CTime& rhs)	= delete;
private:
	// 前フレームの時間.
	std::chrono::time_point<std::chrono::high_resolution_clock> m_PreviousTime;

	float m_TargetFrameTime;// 目標フレーム時間(秒).
	float m_DeltaTime;		// フレーム間の時間差.
};
