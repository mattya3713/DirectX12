#pragma once

#include <functional>
#include <string>
#include <vector>

#include "00_Game/80_CutScene/CutSceneData.h"

/**********************************************************************************
* @author    : Coder(閃斬 Production Loop).
* @date      : 2026/08/23.
* @brief     : カットシーンのランタイム再生. Play(名前)でJSONを読み込み、
*            : Update(dt)で各トラックを時間進行に沿って処理する.
*            : ExistingInstanceトラックはServiceLocator経由で実インスタンスを操作する.
**********************************************************************************/

class CutScenePlayer final
{
public:
	CutScenePlayer() = default;
	~CutScenePlayer() = default;

	CutScenePlayer(const CutScenePlayer&)            = delete;
	CutScenePlayer& operator=(const CutScenePlayer&) = delete;

	// カットシーンを名前指定で再生する(Data/Json/CutScene/<名前>.json). 成功したらtrue.
	bool Play(const std::string& CutSceneName, std::function<void()> OnFinished = nullptr);

	// 編集中データを直接再生する(Editorのプレビュー用).
	bool PlayEvent(const CutSceneEvent& Event, std::function<void()> OnFinished = nullptr);

	// 再生を中断する.
	void Stop();

	// 毎フレーム呼ぶ(再生中のみ処理. 一時停止中は呼び元で制御すること).
	void Update(float DeltaTime);

	bool IsPlaying() const noexcept { return m_IsPlaying; }
	float GetElapsedTime() const noexcept { return m_ElapsedTime; }

private:
	// トラックの開始処理(アニメ切替・SE再生・カメラOneShot起動).
	void BeginTrack(CutSceneTrack& Track);

	// SkinMeshトラックのキーフレーム補間と実インスタンスへの反映.
	void ProcessSkinMeshTrack(CutSceneTrack& Track, float Elapsed);

	std::vector<CutSceneTrack> m_Tracks;          // 再生中のトラック(コピー).
	std::vector<bool>          m_TrackStarted;    // 各トラックの開始済みフラグ.
	std::string                m_EventName;
	float                      m_TotalDuration = 0.0f;
	float                      m_ElapsedTime   = 0.0f;
	bool                       m_IsPlaying     = false;
	std::function<void()>      m_OnFinished;

	std::vector<std::string> m_ActiveLoopSoundNames; // ループ再生を開始したSE名(Stop()で個別に止めるため).
	bool                      m_TargetResolveWarned = false; // SkinMeshターゲット未解決の警告済みフラグ(再生ごとにリセットする).
};
