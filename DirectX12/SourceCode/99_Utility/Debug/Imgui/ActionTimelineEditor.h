#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "EditorFramework/EditorCommandStack.h"

class MmdlActor;

/**********************************************************************************
* @author    : Coder(閃斬 Production Loop).
* @date      : 2026/08/23.
* @brief     : Player攻撃アクションJSON(Data\Json\Player\AttackCombo配下)の
*            : ComboStartTime/MinComboTransTime/ComboEndTime/ColliderWindowsを
*            : タイムラインバー上で視覚編集するImGuiツール.
*            : 区間のドラッグでStart/Durationを調整でき、バーのスクラブで
*            : MmdlActor::SetCurrentFrame()によるポーズ固定プレビューを行う.
*            : 保存はFileManager::JsonSaveによる元JSONへの書き戻し.
**********************************************************************************/

class ActionTimelineEditor final
{
public:
	ActionTimelineEditor();
	~ActionTimelineEditor() = default;

	// 毎フレーム呼ぶ. JSON選択・数値編集・タイムライン編集・スクラブ・保存を行う.
	void Draw(MmdlActor& Actor);

public:
	// ColliderWindowの編集用データ(Combat::ColliderWindowから実行時フラグを除いたもの).
	struct WindowParam
	{
		float Start    = 0.0f;
		float Duration = 0.1f;

		bool operator==(const WindowParam& Other) const noexcept { return Start == Other.Start && Duration == Other.Duration; }
	};

	// 攻撃アクションJSON1件分の編集用データ.
	struct SettingsData
	{
		float ComboStartTime    = 0.0f;
		float MinComboTransTime = 0.0f;
		float ComboEndTime      = 1.0f;
		std::vector<WindowParam> Windows;
	};

	// 編集データを一括差し替える(Undo/Redoコマンドから履歴復元に使う).
	void ApplySettings(const SettingsData& Data);

private:
	// タイムライン上のドラッグ操作の種別.
	enum class DragMode
	{
		None,
		Scrub,        // バー空き部分のドラッグ(再生位置のスクラブ).
		MoveWindow,   // 区間本体のドラッグ(Start移動).
		ResizeWindow, // 区間右端ハンドルのドラッグ(Duration変更).
		MoveComboStart,
		MoveMinTrans,
	};

private:
	// AttackComboフォルダの*.jsonを走査して選択肢を作る.
	void ScanFiles();

	// 選択中のJSONを読み込む(pActorが非nullなら対応クリップを先頭から再生し直す).
	void LoadSelected(MmdlActor* pActor);

	// 選択中のJSONへ編集結果を書き戻す.
	bool SaveSelected() const;

	// 選択中ファイル名に対応するアニメーションクリップ名(未知は空文字).
	const char* FindClipName() const;

	// スクラブ時刻をアクターへ反映する(未再生ならクリップを起動してから固定する).
	void ApplyScrub(MmdlActor& Actor);

	// タイムラインバーの描画とドラッグ編集・スクラブ.
	void DrawTimeline(MmdlActor& Actor);

	// タイムライン全体の表示範囲(秒. 最長区間末尾に余白を足した値).
	float TimeMax() const;

private:
	static constexpr float        kBarHeight    = 48.0f; // タイムラインバーの高さ(pixel).
	static constexpr float        kHandleGrabPx = 7.0f;  // 右端ハンドル/マーカーを掴める幅(pixel).
	static constexpr const char*  kJsonDir      = "Data\\Json\\Player\\AttackCombo";

	std::vector<std::string> m_FileNames;    // 選択肢(JSONファイル名).
	std::string              m_SelectedFile; // 現在選択中のJSONファイル名.
	std::filesystem::path    m_SelectedPath; // 現在選択中のJSONフルパス.
	SettingsData             m_Data;         // 編集中の値.
	std::string              m_ClipName;     // 対応するアニメーションクリップ名.

	int      m_SelectedWindow = -1;             // 強調表示中のColliderWindow(-1=なし).
	float    m_ScrubSeconds   = 0.0f;           // スクラブ中の再生位置(秒).
	DragMode m_DragMode       = DragMode::None; // 現在のドラッグ状態.
	int      m_DragWindow     = -1;             // ドラッグ中のColliderWindow(-1=なし).
	float    m_DragGrabOffset = 0.0f;           // 掴んだ位置から区間Startまでのオフセット(秒).
	SettingsData m_DragStartSnapshot;            // ドラッグ開始時の値(Undo用の旧状態スナップショット).
	EditorCommandStack m_Commands;               // Undo/Redo履歴(共通Editorフレームワーク).
};
