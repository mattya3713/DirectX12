#pragma once

/**********************************************************************************
* @author    : Coder 青龍(せいりゅう).
* @date      : 2026/08/23.
* @brief     : SE/BGMとイベント名を紐付けるSound Eventのデータモデル+ImGui編集+
*            : JSON保存/読込(_DEBUG限定のエディタウィンドウ. 再生自体は
*            : SoundManagerの既存API経由).
*            : NOTE: ピッチ/同時再生数上限はSoundManagerのAPI拡張待ちのため
*            :        保存のみ(Cooldownは本クラス側で時刻管理して強制する).
**********************************************************************************/

#include <chrono>
#include <string>
#include <unordered_map>
#include <vector>

class SoundEventEditor final
{
public:
	// イベント1件分の定義.
	struct SoundEventData
	{
		std::string Name;              // イベント名(例: attack_hit).
		std::string FileName;          // 音声ファイル名(拡張子無し. SoundManagerのキー).
		float       Volume     = 1.0f; // 音量(0.0〜1.0).
		float       Pitch      = 1.0f; // ピッチ(保存のみ. SoundManager API拡張待ち).
		int         MaxVoices  = 1;    // 同時再生数上限(保存のみ. API拡張待ち).
		float       Cooldown   = 0.1f; // 再発までの最低間隔(秒).
		bool        IsLoop     = false;

		void Reset()
		{
			Name.clear();
			FileName.clear();
			Volume     = 1.0f;
			Pitch      = 1.0f;
			MaxVoices  = 1;
			Cooldown   = 0.1f;
			IsLoop     = false;
		}
	};

	SoundEventEditor() = default;
	~SoundEventEditor() = default;

	// 毎フレーム呼ぶ(DebugビルドのMainSceneから).
	void Draw();

	// イベント名を指定して再生する(Cooldown中なら無視. Combat等から呼ぶ想定).
	static void PlayCombatEvent(const char* Name);

private:
	// 試聴/Combatからの再生(Cooldown判定込み).
	void PlayEvent(SoundEventData& Event);

	// プリセット保存/読込(Data/Json/SoundEvent/<名前>.json).
	bool SavePresets() const;
	bool LoadPresets(const std::string& Name);

	std::vector<SoundEventData>                                  m_Events;      // 定義済みイベント.
	int                                                          m_Selected     = -1; // 選択中イベント(-1=なし).
	std::string                                                  m_PresetName   = "sound_events"; // プリセット名入力.
	std::unordered_map<std::string, std::chrono::steady_clock::time_point> m_LastPlayed; // Cooldown用の最終再生時刻.
};
