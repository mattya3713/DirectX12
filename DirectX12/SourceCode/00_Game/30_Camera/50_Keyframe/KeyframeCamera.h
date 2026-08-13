#pragma once

#include <functional>
#include <vector>

#include "00_Game/30_Camera/00_Base/CameraBase.h"
#include "99_Utility/Math/Easing/Easing.h"

// キーフレーム1つ分. Durationは直前キーフレームからの遷移時間(秒)で、先頭要素では未使用.
struct CameraKeyframe
{
	DirectX::XMFLOAT3 Position;
	DirectX::XMFLOAT3 Look;
	float             FovY;
	float             Duration;
	MyEasing::Type    Easing = MyEasing::Type::Liner;
};

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/13.
* @brief     : 決められたキーフレーム列を順に再生する演出用カメラ(パリィ演出等).
*            : 再生終了時にOnFinishedを呼び出す(元のカメラへ戻す責務は呼び出し側が持つ).
**********************************************************************************/

class KeyframeCamera final
	: public CameraBase
{
public:
	// IsRelativeToFirstがtrueの場合、先頭以外のキーフレームのPosition/Lookは
	// 先頭キーフレームからのオフセットとして扱う(FovYは常に絶対値).
	KeyframeCamera(std::vector<CameraKeyframe> Keyframes, bool IsRelativeToFirst, std::function<void()> OnFinished);
	virtual ~KeyframeCamera() override;

	virtual void Update() override;

private:
	void Finish();

private:
	std::vector<CameraKeyframe> m_Keyframes;
	std::function<void()>       m_OnFinished;

	size_t m_SegmentIndex   = 1;		// 再生中の区間(m_Keyframes[SegmentIndex-1] → [SegmentIndex]).
	float  m_SegmentElapsed = 0.0f;	// 現在区間の経過時間(秒).
	bool   m_IsFinished     = false;
};
