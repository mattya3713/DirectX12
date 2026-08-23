#include "stdafx.h"
#include "CutSceneData.h"

#include "99_Utility/FileManager/FileManager.h"

namespace {

	constexpr const char* kKeyName        = "Name";
	constexpr const char* kKeyDuration    = "TotalDuration";
	constexpr const char* kKeyTracks      = "Tracks";
	constexpr const char* kKeyType        = "Type";
	constexpr const char* kKeyTrackName   = "TrackName";
	constexpr const char* kKeyTargetMode  = "TargetMode";
	constexpr const char* kKeyTargetKey   = "TargetKey";
	constexpr const char* kKeyAnimClip    = "AnimClipName";
	constexpr const char* kKeyLoopAnim    = "IsLoopAnim";
	constexpr const char* kKeySound       = "SoundName";
	constexpr const char* kKeyLoopSound   = "IsLoopSound";
	constexpr const char* kKeyFrames      = "Keyframes";
	constexpr const char* kKeyTime        = "Time";
	constexpr const char* kKeyPosition    = "Position";
	constexpr const char* kKeyRotation    = "RotationDeg";
	constexpr const char* kKeyLookAt      = "LookAt";
	constexpr const char* kKeyScale       = "Scale";
	constexpr const char* kKeyEasing      = "Easing";

	eCutSceneTrackType TrackTypeFromInt(int Value) noexcept
	{
		if (Value == static_cast<int>(eCutSceneTrackType::SkinMesh)) { return eCutSceneTrackType::SkinMesh; }
		if (Value == static_cast<int>(eCutSceneTrackType::Sound)) { return eCutSceneTrackType::Sound; }
		return eCutSceneTrackType::Camera;
	}

	eCutSceneTargetMode TargetModeFromInt(int Value) noexcept
	{
		if (Value == static_cast<int>(eCutSceneTargetMode::ExistingInstance)) { return eCutSceneTargetMode::ExistingInstance; }
		return eCutSceneTargetMode::SpawnedInstance;
	}

	DirectX::XMFLOAT3 Vec3FromJson(const nlohmann::json& JsonData, const char* Key)
	{
		DirectX::XMFLOAT3 result{ 0.0f, 0.0f, 0.0f };
		if (JsonData.contains(Key) && JsonData[Key].is_array() && JsonData[Key].size() >= 3) {
			result.x = JsonData[Key][0].get<float>();
			result.y = JsonData[Key][1].get<float>();
			result.z = JsonData[Key][2].get<float>();
		}
		return result;
	}

	nlohmann::json Vec3ToJson(const DirectX::XMFLOAT3& Value)
	{
		return nlohmann::json::array({ Value.x, Value.y, Value.z });
	}

}

float CutSceneTrack::EndTime() const noexcept
{
	if (Keyframes.empty()) { return 0.0f; }
	return Keyframes.back().Time;
}

void CutSceneEvent::RecalculateDuration() noexcept
{
	TotalDuration = 0.0f;
	for (const CutSceneTrack& track : Tracks) {
		TotalDuration = (track.EndTime() > TotalDuration) ? track.EndTime() : TotalDuration;
	}
}

nlohmann::json CutSceneEventToJson(const CutSceneEvent& Event)
{
	nlohmann::json tracks = nlohmann::json::array();

	for (const CutSceneTrack& track : Event.Tracks) {
		nlohmann::json frames = nlohmann::json::array();
		for (const CutSceneKeyframe& frame : track.Keyframes) {
			frames.push_back({
				{ kKeyTime,     frame.Time },
				{ kKeyPosition, Vec3ToJson(frame.Position) },
				{ kKeyRotation, Vec3ToJson(frame.RotationDeg) },
				{ kKeyLookAt,   Vec3ToJson(frame.LookAt) },
				{ kKeyScale,    frame.Scale },
				{ kKeyEasing,   static_cast<int>(frame.Easing) },
			});
		}

		tracks.push_back({
			{ kKeyType,       static_cast<int>(track.Type) },
			{ kKeyTrackName,  track.Name },
			{ kKeyTargetMode, static_cast<int>(track.TargetMode) },
			{ kKeyTargetKey,  track.TargetKey },
			{ kKeyAnimClip,   track.AnimClipName },
			{ kKeyLoopAnim,   track.IsLoopAnim },
			{ kKeySound,      track.SoundName },
			{ kKeyLoopSound,  track.IsLoopSound },
			{ kKeyFrames,     frames },
		});
	}

	return {
		{ kKeyName,     Event.Name },
		{ kKeyDuration, Event.TotalDuration },
		{ kKeyTracks,   tracks },
	};
}

bool CutSceneEventFromJson(const nlohmann::json& JsonData, CutSceneEvent& OutEvent)
{
	if (JsonData.is_null()) { return false; }

	OutEvent.Name         = JsonData.value(kKeyName, "");
	OutEvent.TotalDuration = JsonData.value(kKeyDuration, 0.0f);
	OutEvent.Tracks.clear();

	if (!JsonData.contains(kKeyTracks) || !JsonData[kKeyTracks].is_array()) { return true; }

	for (const nlohmann::json& track_json : JsonData[kKeyTracks]) {
		CutSceneTrack track{};
		track.Type       = TrackTypeFromInt(track_json.value(kKeyType, 0));
		track.Name       = track_json.value(kKeyTrackName, "");
		track.TargetMode = TargetModeFromInt(track_json.value(kKeyTargetMode, 0));
		track.TargetKey  = track_json.value(kKeyTargetKey, "");
		track.AnimClipName = track_json.value(kKeyAnimClip, "");
		track.IsLoopAnim = track_json.value(kKeyLoopAnim, false);
		track.SoundName  = track_json.value(kKeySound, "");
		track.IsLoopSound = track_json.value(kKeyLoopSound, false);

		if (track_json.contains(kKeyFrames) && track_json[kKeyFrames].is_array()) {
			for (const nlohmann::json& frame_json : track_json[kKeyFrames]) {
				CutSceneKeyframe frame{};
				frame.Time        = frame_json.value(kKeyTime, 0.0f);
				frame.Position    = Vec3FromJson(frame_json, kKeyPosition);
				frame.RotationDeg = Vec3FromJson(frame_json, kKeyRotation);
				frame.LookAt      = Vec3FromJson(frame_json, kKeyLookAt);
				frame.Scale       = frame_json.value(kKeyScale, 1.0f);
				frame.Easing      = static_cast<MyEasing::Type>(frame_json.value(kKeyEasing, 0));
				track.Keyframes.push_back(frame);
			}
		}

		OutEvent.Tracks.push_back(std::move(track));
	}

	return true;
}
