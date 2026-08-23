#pragma once

/**********************************************************************************
* @date      : 2026-08-23.
* @brief     : エディタ共通のUndo/Redoコマンドインターフェース(Commandパターン).
*            : Execute()/Undo()が何度呼ばれても同じ結果になるよう、状態の完全な
*            : 再現(スナップショット等)を実装クラスが保証すること.
**********************************************************************************/

class IEditorCommand
{
public:
	virtual ~IEditorCommand() = default;

	// 操作を適用する(Redo時にも呼ばれる).
	virtual void Execute() = 0;

	// 操作を取り消す.
	virtual void Undo() = 0;

	// デバッグ表示用の操作名(例: "区間移動"). 所有権は持たない.
	virtual const char* GetLabel() const = 0;
};
