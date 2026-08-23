#include "stdafx.h"
#include "CutScenePlayer.h"

#include <algorithm>

#include "00_Game/10_Object/00_Base/GameObject.h"
#include "00_Game/10_Object/10_MeshObject/00_Character/00_Player/Player.h"
#include "00_Game/10_Object/10_MeshObject/00_Character/20_Boss/Boss.h"
#include "00_Game/30_Camera/50_Keyframe/KeyframeCamera.h"
#include "00_Game/30_Camera/99_Manager/CameraManager.h"
#include "99_Utility/FileManager/FileManager.h"
#include "99_Utility/Debug/Log/DebugLog.h"
#include "99_Utility/Math/Easing/Easing.h"
#include "99_Utility/Sound/SoundManager.h"
#include "99_Utility/ServiceLocator/ServiceLocator.h"

namespace {
	constexpr float kCameraFovDeg = 35.0f; // カットシーンカメラの仮画角(PlayOneShot経由で再生).
	constexpr float kDegToRad     = DirectX::XM_PI / 180.0f;
}

bool CutScenePlayer::Play(const std::string& CutSceneName, std::function<void()> OnFinished)
{
	const nlohmann::json data = FileManager::JsonLoad(std::filesystem::path{ "Data/Json/CutScene/" } / (CutSceneName + ".json"));
	CutSceneEvent event{};
	if (!CutSceneEventFromJson(data, event)) {
		return false;
	}
	event.RecalculateDuration();
	return PlayEvent(event, std::move(OnFinished));
}

bool CutScenePlayer::PlayEvent(const CutSceneEvent& Event, std::function<void()> OnFinished)
{
	if (Event.Tracks.empty()) { return false; }

	m_Tracks        = Event.Tracks;
	m_TrackStarted.assign(m_Tracks.size(), false);
	m_EventName     = Event.Name;
	m_TotalDuration = Event.TotalDuration;
	m_ElapsedTime   = 0.0f;
	m_IsPlaying     = true;
	m_OnFinished    = std::move(OnFinished);

	return true;
}

void CutScenePlayer::Stop()
{
	m_IsPlaying   = false;
	m_OnFinished  = nullptr;
}

void CutScenePlayer::Update(float DeltaTime)
{
	if (!m_IsPlaying) { return; }

	m_ElapsedTime += DeltaTime;

	for (size_t i = 0; i < m_Tracks.size(); ++i) {
		CutSceneTrack& track = m_Tracks[i];

		if (track.Keyframes.empty()) { continue; }

		const float start_time = track.Keyframes.front().Time;
		if (m_ElapsedTime < start_time) { continue; }

		if (!m_TrackStarted[i]) {
			m_TrackStarted[i] = true;
			BeginTrack(track);
		}

		if (track.Type == eCutSceneTrackType::SkinMesh) {
			ProcessSkinMeshTrack(track, m_ElapsedTime - start_time);
		}
	}

	// 全トラック終了(=イベント時間経過)で停止しコールバック.
	if (m_ElapsedTime >= m_TotalDuration) {
		std::function<void()> finished = std::move(m_OnFinished);
		Stop();
		if (finished) { finished(); }
	}
}

void CutScenePlayer::BeginTrack(CutSceneTrack& Track)
{
	switch (Track.Type) {
	case eCutSceneTrackType::Camera:
	{
		// キーフレームをKeyframeCameraへ変換し、OneShotとして再生する
		// (ParryReaction演出カメラと同じ仕組み. 再生後に自動で元カメラへ戻る).
		std::vector<CameraKeyframe> frames;
		frames.reserve(Track.Keyframes.size());
		for (const CutSceneKeyframe& frame : Track.Keyframes) {
			frames.push_back({ frame.Position, frame.LookAt, DirectX::XMConvertToRadians(kCameraFovDeg), frame.Time, frame.Easing });
		}

		if (CameraManager* p_camera_manager = ServiceLocator::Get<CameraManager>()) {
			p_camera_manager->PlayOneShot("CutScene_" + m_EventName, std::move(frames));
		}
		break;
	}

	case eCutSceneTrackType::SkinMesh:
	{
		// アニメクリップ指定があれば開始時に切り替える(実インスタンスのみ対応).
		if (Track.AnimClipName.empty() || Track.TargetMode != eCutSceneTargetMode::ExistingInstance) { break; }

		if (Track.TargetKey == "Player") {
			if (Player* p_player = ServiceLocator::Get<Player>()) { p_player->PlayNamedClip(Track.AnimClipName); }
		}
		else if (Track.TargetKey == "Boss") {
			if (Boss* p_boss = ServiceLocator::Get<Boss>()) { p_boss->PlayNamedClip(Track.AnimClipName); }
		}
		break;
	}

	case eCutSceneTrackType::Sound:
	{
		if (Track.SoundName.empty()) { break; }
		if (SoundManager* p_sound_manager = ServiceLocator::Get<SoundManager>()) {
			p_sound_manager->Play(Track.SoundName, Track.IsLoopSound);
		}
		break;
	}

	default:
		break;
	}
}

void CutScenePlayer::ProcessSkinMeshTrack(CutSceneTrack& Track, float LocalTime)
{
	// 実インスタンスの解決(v1はExistingInstanceのみ. SpawnedInstanceは要MmdlRenderer登録で別タスク).
	GameObject* p_target = nullptr;
	if (Track.TargetMode == eCutSceneTargetMode::ExistingInstance) {
		if (Track.TargetKey == "Player") {
			p_target = ServiceLocator::Get<Player>();
		}
		else if (Track.TargetKey == "Boss") {
			p_target = ServiceLocator::Get<Boss>();
		}
	}

	if (p_target == nullptr) {
		static bool warned = false; // 毎フレーム警告を避ける初回のみ.
		if (!warned) {
			warned = true;
			if (DebugLog* p_debug_log = ServiceLocator::Get<DebugLog>()) {
				p_debug_log->LogWarning("CutScenePlayer: SkinMeshトラックのターゲットを解決できません: " + Track.TargetKey);
			}
		}
		return;
	}	// 現在時刻を挟むキーフレーム対を見つけて補間する.
	const size_t frame_count = Track.Keyframes.size();
	if (frame_count == 0) { return; }

	const CutSceneKeyframe* p_prev = &Track.Keyframes.front();
	const CutSceneKeyframe* p_next = &Track.Keyframes.back();

	for (size_t i = 0; i + 1 < frame_count; ++i) {
		if (LocalTime >= Track.Keyframes[i].Time && LocalTime < Track.Keyframes[i + 1].Time) {
			p_prev = &Track.Keyframes[i];
			p_next = &Track.Keyframes[i + 1];
			break;
		}
	}

	const float segment = (p_next->Time > p_prev->Time) ? (p_next->Time - p_prev->Time) : 1.0f;
	const float local   = std::clamp(LocalTime - p_prev->Time, 0.0f, segment);

	DirectX::XMFLOAT3 position{};
	DirectX::XMFLOAT3 rotation{};
	float scale = 1.0f;

	MyEasing::UpdateEasing(p_next->Easing, local, segment, p_prev->Position.x, p_next->Position.x, position.x);
	MyEasing::UpdateEasing(p_next->Easing, local, segment, p_prev->Position.y, p_next->Position.y, position.y);
	MyEasing::UpdateEasing(p_next->Easing, local, segment, p_prev->Position.z, p_next->Position.z, position.z);
	MyEasing::UpdateEasing(p_next->Easing, local, segment, p_prev->RotationDeg.x * kDegToRad, p_next->RotationDeg.x * kDegToRad, rotation.x);
	MyEasing::UpdateEasing(p_next->Easing, local, segment, p_prev->RotationDeg.y * kDegToRad, p_next->RotationDeg.y * kDegToRad, rotation.y);
	MyEasing::UpdateEasing(p_next->Easing, local, segment, p_prev->RotationDeg.z * kDegToRad, p_next->RotationDeg.z * kDegToRad, rotation.z);
	MyEasing::UpdateEasing(p_next->Easing, local, segment, p_prev->Scale, p_next->Scale, scale);

	Transform transform{};
	transform.Position = position;
	transform.Rotation = rotation;
	transform.Scale    = { scale, scale, scale };
	p_target->SetTransform(transform);
}
