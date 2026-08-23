#pragma once

#include <DirectXMath.h>
#include <string>
#include <vector>

#include "json/json.hpp"
#include "99_Utility/Math/Easing/Easing.h"

/**********************************************************************************
* @author    : Coder(閃斬 Production Loop).
* @date      : 2026/08/23.
* @brief     : カットシーンのデータ構造(トラック=1件の演出要素, イベント=1本の
*            : カットシーン)とJSON変換.
**********************************************************************************/

enum class eCutSceneTrackType
{
	Camera = 0,
	SkinMesh,
	Sound,
};

enum class eCutSceneTargetMode
{
	SpawnedInstance = 0,   // 専用の一時インスタンスを生成して操作する(v1は未対応. 警告を出してスキップ).
	ExistingInstance = 1,  // TargetKey("Player"/"Boss")でServiceLocatorから実インスタンスを解決する.
};

// トラック内の1キーフレーム. Time間隔を前キーフレームからのEasingタイプで補間する.
struct CutSceneKeyframe
{
	float Time = 0.0f;

	DirectX::XMFLOAT3 Position   = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 RotationDeg = { 0.0f, 0.0f, 0.0f }; // オイラー角(度).
	DirectX::XMFLOAT3 LookAt     = { 0.0f, 0.0f, 0.0f }; // Cameraトラック用注視点.
	float Scale = 1.0f;

	MyEasing::Type Easing = MyEasing::Type::Liner;
};

// 1件分の演出要素(Camera/SkinMesh/Sound).
struct CutSceneTrack
{
	eCutSceneTrackType Type = eCutSceneTrackType::Camera;
	std::string Name;

	eCutSceneTargetMode TargetMode = eCutSceneTargetMode::ExistingInstance;
	std::string TargetKey;      // ExistingInstance用の解決キー("Player"/"Boss").

	std::string AnimClipName;   // SkinMesh用(空ならアニメ変更なし).
	bool IsLoopAnim = false;    // SkinMesh用(未使用情報. 将来拡張).

	std::string SoundName;      // Sound用(SE名).
	bool IsLoopSound = false;

	std::vector<CutSceneKeyframe> Keyframes;

	// 最終キーフレームの時刻(キーフレームが無ければ0).
	float EndTime() const noexcept;
};

// 1本のカットシーン(複数トラック).
struct CutSceneEvent
{
	std::string Name;
	float TotalDuration = 0.0f;
	std::vector<CutSceneTrack> Tracks;

	// TracksからTotalDurationを再計算する.
	void RecalculateDuration() noexcept;
};

// JSON変換(FileManager::JsonLoad/JsonSaveと組み合わせて使う).
nlohmann::json CutSceneEventToJson(const CutSceneEvent& Event);
bool CutSceneEventFromJson(const nlohmann::json& JsonData, CutSceneEvent& OutEvent);
