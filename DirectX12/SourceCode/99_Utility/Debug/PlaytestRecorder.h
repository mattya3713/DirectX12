#pragma once

#if _DEBUG

#include <string>
#include <vector>

/**********************************************************************************
* @author    : Coder 玄武(閃斬 Production Loop).
* @date      : 2026/08/23.
* @brief     : Playtest Recorder(DEBUG限定. F9で記録開始/停止をトグル).
*            : 毎フレームの入力・Player/BossのState・HP・コンボ・ゲージ・
*            : TimeScale・カメラ位置を監視してCSVへ書き出し、State遷移と
*            : HP変化はイベントとしてImGuiログに表示する。
*            : ServiceLocator経由で状態を取得するためゲームロジックには
*            : 一切介入しない(読み取り専用の監視).
**********************************************************************************/

class PlaytestRecorder final
{
public:
	static PlaytestRecorder& Instance();

	// 記録開始/停止をトグルする(停止時にCSVへ保存する).
	void Toggle();
	bool IsRecording() const noexcept { return m_IsRecording; }

	// 毎フレーム呼ぶ(MainScene::Updateの_DEBUGブロックから呼ぶ想定).
	void Tick();

	// ImGui表示(記録状態・イベント履歴・保存先).
	void DrawImGui();

private:
	PlaytestRecorder() = default;
	~PlaytestRecorder() = default;

	PlaytestRecorder(const PlaytestRecorder&)            = delete;
	PlaytestRecorder& operator=(const PlaytestRecorder&) = delete;
	PlaytestRecorder(PlaytestRecorder&&)                 = delete;
	PlaytestRecorder& operator=(PlaytestRecorder&&)      = delete;

	// 停止時にこれまでの記録をCSVファイルへ書き出す.
	bool SaveToCsv();

	std::vector<std::string> m_CsvLines;   // 1フレーム1行のCSV本体.
	std::vector<std::string> m_Events;     // State遷移・HP変化などのイベント行(ImGui表示用).
	int    m_Frame        = 0;             // 記録中のフレーム数.
	float  m_ElapsedTime  = 0.0f;          // 記録中の経過時間(秒).
	float  m_PrevPlayerHp = -1.0f;         // 前フレームHP(変化検出用).
	float  m_PrevBossHp   = -1.0f;
	std::string m_PrevPlayerState = "?";   // 前フレームState名(遷移検出用).
	std::string m_PrevBossState   = "?";
	std::string m_LastSavePath;            // 最後に保存したCSVパス(表示用).
	bool   m_IsRecording   = false;
};

#endif // _DEBUG
