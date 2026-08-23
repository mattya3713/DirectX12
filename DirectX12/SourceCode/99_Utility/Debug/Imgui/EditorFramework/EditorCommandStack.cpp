#include "EditorCommandStack.h"

#include "../ImGuiManager.h" // imgui.hはImGuiManager.h経由でincludeされる(サブディレクトリのため相対パス).

// コマンドをExecuteしてUndoスタックへ積む.
void EditorCommandStack::Execute(std::unique_ptr<IEditorCommand> Command)
{
	if (!Command) { return; }

	Command->Execute();
	m_RedoStack.clear();
	m_UndoStack.push_back(std::move(Command));
}

// 直前の操作を取り消す.
bool EditorCommandStack::Undo()
{
	if (m_UndoStack.empty()) { return false; }

	IEditorCommand& command = *m_UndoStack.back();
	command.Undo();
	m_RedoStack.push_back(std::move(m_UndoStack.back()));
	m_UndoStack.pop_back();
	return true;
}

// 取り消した操作をやり直す.
bool EditorCommandStack::Redo()
{
	if (m_RedoStack.empty()) { return false; }

	IEditorCommand& command = *m_RedoStack.back();
	command.Execute();
	m_UndoStack.push_back(std::move(m_RedoStack.back()));
	m_RedoStack.pop_back();
	return true;
}

// Ctrl+Z(取り消し)/Ctrl+YまたはCtrl+Shift+Z(やり直し)を検出して処理する.
void EditorCommandStack::HandleShortcuts()
{
	ImGuiIO& io = ImGui::GetIO();
	if (!io.KeyCtrl) { return; }

	// Ctrl+Shift+ZはRedoの別操作(一般的なエディタと同じ).
	if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Z)))
	{
		if (io.KeyShift)
		{
			Redo();
		}
		else
		{
			Undo();
		}
	}
	else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Y)))
	{
		Redo();
	}
}

// スタックを空にする.
void EditorCommandStack::Clear()
{
	m_UndoStack.clear();
	m_RedoStack.clear();
}
