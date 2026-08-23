#pragma once

#include <string>

#include "00_Game/80_CutScene/CutSceneData.h"

/**********************************************************************************
* @author    : Coder(閃斬 Production Loop).
* @date      : 2026/08/23.
* @brief     : カットシーンを編集・保存・読込・プレビュー再生するImGuiツール.
*            : トラック(Camera/SkinMesh/Sound)の追加・削除、キーフレーム編集、
*            : Data/Json/CutScene配下へのJSON保存・読込を行う.
**********************************************************************************/

class CutSceneEditor final
{
public:
	CutSceneEditor() = default;
	~CutSceneEditor() = default;

	// 毎フレーム呼ぶ(DebugビルドのMainSceneから).
	void Draw();

private:
	// 選択中トラックの編集UI.
	void DrawTrackProperties();

	// 選択中トラックのキーフレーム編集UI.
	void DrawKeyframeList();

	// 現在のイベントをData/Json/CutScene/<名前>.jsonへ保存する.
	bool SaveToFile() const;

	// 指定名のJSONを読み込む.
	bool LoadFromFile(const std::string& Name);

	CutSceneEvent m_Event;        // 編集中のカットシーン.
	int           m_SelectedTrack = -1;    // 選択中トラック(-1=なし).
	int           m_SelectedFrame = -1;    // 選択中キーフレーム(-1=なし).
	std::string   m_LoadName;              // 読込用ファイル名入力.
};
