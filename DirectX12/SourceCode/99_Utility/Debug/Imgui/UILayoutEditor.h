#pragma once

#include <functional>
#include <string>

#include "EditorFramework/EditorCommandStack.h"
#include "00_Game/10_Object/30_UIObject/UILayoutModel.h"

/**********************************************************************************
* @date      : 2026-08-23.
* @brief     : UIレイアウトを編集・保存するImGuiツール(DEBUG限定運用).
*            : Sprite/Text要素の追加・選択・移動(プレビュー内ドラッグ)・拡縮・
*            : アンカー設定・レイヤー/表示切替・Undo/Redo(共通EditorCommandStack)・
*            : JSON保存/読込を持つ。プレビューは解像度プリセットで再計算表示し、
*            : アンカー＋オフセット方式により解像度変更へ自動追従する.
**********************************************************************************/

class UILayoutEditor final
{
public:
	UILayoutEditor();
	~UILayoutEditor() = default;

	// 毎フレーム呼ぶ. レイアウト編集・プレビュー・保存/読込を行う.
	void Draw();

	// 編集データを一括差し替える(Undo/Redoコマンドから履歴復元に使う).
	void ApplyModel(const UILayoutModel& Model);

private:
	// 要素一覧(レイヤー順)の描画.
	void DrawElementList();

	// 選択中要素に対する操作UI(プロパティ編集・複製・削除).
	void DrawSelectedPanel();

	// アンカー配置のプレビュー描画(ドラッグ移動もここで処理する).
	void DrawPreview();

	// 編集操作をスナップショット経由でUndoスタックへ積みながら実行する.
	void Mutate(const char* pLabel, const std::function<void(UILayoutModel&)>& Operation);

	// ドラッグ開始時のスナップショットを記録する.
	void BeginDrag();

	// ドラッグ終了時に変化があれば履歴へ積む.
	void EndDrag(const char* pLabel);

	// 選択解像度を取得する.
	void GetResolution(float& OutWidth, float& OutHeight) const noexcept;

private:
	static constexpr const char* kDefaultFile = "Data\\Json\\UI\\layout.json";
	static constexpr float kPreviewWidth  = 480.0f; // プレビュー描画幅(px).
	static constexpr float kPreviewHeight = 270.0f; // プレビュー描画高さ(px).

	UILayoutModel       m_Model;      // 編集中のレイアウトモデル(JSON可逆).
	EditorCommandStack  m_Commands;   // Undo/Redo履歴(共通Editorフレームワーク).
	std::string         m_SelectedId; // 選択中要素ID(空=なし).
	std::string         m_SaveStatus; // 保存/読込の結果表示.
	std::string         m_FilePath    = kDefaultFile;
	int                 m_NewElementType = 0;   // 追加用の種別選択.
	int                 m_ResolutionIndex = 0;  // プレビュー解像度プリセット.
	bool                m_IsDragging  = false;  // プレビュードラッグ中.
	float               m_GrabOffsetX = 0.0f;   // 掴んだ位置と要素原点のずれX.
	float               m_GrabOffsetY = 0.0f;   // 掴んだ位置と要素原点のずれY.
	UILayoutModel       m_DragOldModel;         // ドラッグ開始時のモデル(履歴用).
};
