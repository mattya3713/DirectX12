#pragma once

#include <memory>
#include <vector>

#include "IEditorCommand.h"
/**********************************************************************************
* @date      : 2026-08-23.
* @brief     : エディタ共通のUndo/Redoスタック.
*            : Execute()でコマンドを積んで即実行し、Ctrl+Z/Ctrl+Yで取り消し・
*            : やり直しを行える。ImGui利用のエディタから毎フレーム
*            : HandleShortcuts()を呼んでもらう想定.
**********************************************************************************/

class EditorCommandStack final
{
public:
	// コマンドをExecuteしてUndoスタックへ積む(Redoスタックはクリアされる).
	void Execute(std::unique_ptr<IEditorCommand> Command);

	// 直前の操作を取り消す(スタックが空ならfalse).
	bool Undo();

	// 取り消した操作をやり直す(スタックが空ならfalse).
	bool Redo();

	// Ctrl+Z(取り消し)/Ctrl+YまたはCtrl+Shift+Z(やり直し)を検出して処理する.
	// ImGuiのフレーム内で毎フレーム呼ぶこと.
	void HandleShortcuts();

	// スタックを空にする(ファイル切替・再読込等、履歴が無意味になるタイミングで呼ぶ).
	void Clear();

	bool CanUndo() const noexcept { return !m_UndoStack.empty(); }
	bool CanRedo() const noexcept { return !m_RedoStack.empty(); }

	// デバッグ表示用: 現在のスタック深さ.
	size_t GetUndoDepth() const noexcept { return m_UndoStack.size(); }
	size_t GetRedoDepth() const noexcept { return m_RedoStack.size(); }

private:
	std::vector<std::unique_ptr<IEditorCommand>>	m_UndoStack; // 過去に実行したコマンド(末尾=最新).
	std::vector<std::unique_ptr<IEditorCommand>>	m_RedoStack; // Undo済みコマンド(末尾=直近に取消したもの).
};
