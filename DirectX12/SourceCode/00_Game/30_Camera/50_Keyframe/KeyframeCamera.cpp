#include "stdafx.h"
#include "KeyframeCamera.h"

#include "00_Game/00_GameLoop/Time/Time.h"
#include "99_Utility/DirectXMath/DirectXMathExpansion.h"

KeyframeCamera::KeyframeCamera(std::vector<CameraKeyframe> Keyframes, bool IsRelativeToFirst, std::function<void()> OnFinished)
	: CameraBase{}
	, m_Keyframes  { std::move(Keyframes) }
	, m_OnFinished { std::move(OnFinished) }
{
	// 相対座標指定の場合、先頭キーフレーム(基準)を絶対座標のまま残し、
	// 2番目以降のPosition/Lookをそこからのオフセットとして絶対座標へ変換しておく.
	if (IsRelativeToFirst && !m_Keyframes.empty()) {
		const DirectX::XMFLOAT3 base_position = m_Keyframes.front().Position;
		const DirectX::XMFLOAT3 base_look     = m_Keyframes.front().Look;
		for (size_t i = 1; i < m_Keyframes.size(); ++i) {
			m_Keyframes[i].Position = base_position + m_Keyframes[i].Position;
			m_Keyframes[i].Look     = base_look     + m_Keyframes[i].Look;
		}
	}
}

KeyframeCamera::~KeyframeCamera()
{
}

void KeyframeCamera::Update()
{
	if (m_IsFinished) { return; }

	// キーフレームが1つ以下なら遷移する区間が無いため、即座にその姿勢で終了する.
	if (m_Keyframes.size() < 2) {
		if (!m_Keyframes.empty()) {
			SetPosition(m_Keyframes.front().Position);
			SetLook(m_Keyframes.front().Look);
			SetFovY(m_Keyframes.front().FovY);
		}
		UpdateViewProjection();
		Finish();
		return;
	}

	m_SegmentElapsed += GameTime::GetDeltaTime();

	// Duration<=0のキーフレーム(瞬間移動)は同一フレーム内で連続して読み飛ばす.
	while (m_SegmentIndex < m_Keyframes.size() && m_SegmentElapsed >= m_Keyframes[m_SegmentIndex].Duration) {
		m_SegmentElapsed -= m_Keyframes[m_SegmentIndex].Duration;
		++m_SegmentIndex;
	}

	if (m_SegmentIndex >= m_Keyframes.size()) {
		const CameraKeyframe& last = m_Keyframes.back();
		SetPosition(last.Position);
		SetLook(last.Look);
		SetFovY(last.FovY);
		UpdateViewProjection();
		Finish();
		return;
	}

	const CameraKeyframe& from = m_Keyframes[m_SegmentIndex - 1];
	const CameraKeyframe& to   = m_Keyframes[m_SegmentIndex];

	DirectX::XMFLOAT3 position = {};
	DirectX::XMFLOAT3 look     = {};
	float             fov_y    = 0.0f;
	MyEasing::UpdateEasing(to.Easing, m_SegmentElapsed, to.Duration, from.Position, to.Position, position);
	MyEasing::UpdateEasing(to.Easing, m_SegmentElapsed, to.Duration, from.Look,     to.Look,     look);
	MyEasing::UpdateEasing(to.Easing, m_SegmentElapsed, to.Duration, from.FovY,     to.FovY,     fov_y);

	SetPosition(position);
	SetLook(look);
	SetFovY(fov_y);
	UpdateViewProjection();
}

void KeyframeCamera::Finish()
{
	m_IsFinished = true;
	if (m_OnFinished) { m_OnFinished(); }
}
