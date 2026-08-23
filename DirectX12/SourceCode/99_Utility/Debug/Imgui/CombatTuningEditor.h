#pragma once

/**********************************************************************************
* @author    : 朱雀 (Suzaku / 閃斬 Production Loop Coder).
* @date      : 2026/08/23.
* @brief     : Combat調整値(CombatTuningData)を一画面で調整するImGuiツール.
*            : スライダー編集・既定値リセット・プリセットJSONの保存/読込を行う.
*            : _DEBUG限定(チート対策). 値は即時にゲームへ反映される.
**********************************************************************************/

class CombatTuningEditor final
{
public:
	CombatTuningEditor() = default;
	~CombatTuningEditor() = default;

	// 毎フレーム呼ぶ. 調整UI・リセット・プリセット保存/読込を行う.
	void Draw();

private:
	char m_PresetName[128] = { "tuning" }; // プリセット保存名(Data\Json\Combat配下).
	bool m_WasLoadedFromMissingFile = false; // 初回読込でファイルが無かった場合の表示用.
};
