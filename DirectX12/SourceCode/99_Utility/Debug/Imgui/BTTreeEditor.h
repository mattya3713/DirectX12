#pragma once

#include <functional>
#include <map>
#include <memory>
#include <string>

#include "EditorFramework/EditorCommandStack.h"
#include "BehaviorTree/BTEditorModel.h"
#include "BehaviorTree/RootNode.h"

/**********************************************************************************
* @date      : 2026-08-23.
* @brief     : Behavior Treeをコード変更なしで編集・保存するImGuiツール.
*            : ノード配置(Selector/Sequence/Decorator/Action)・接続・削除・複製・
*            : Undo/Redo(共通EditorCommandStack)・JSON保存/読込を持つ。
*            : 実行はBuildRuntime()でランタイムツリー(unique_ptr)を生成して行い、
*            : DEBUGビルドでは直近に実行されたActionノード名を表示する.
**********************************************************************************/

class BTTreeEditor final
{
public:
	// Actionの実行関数レジストリ(名前→処理. BuildRuntime()が参照する).
	using ActionRegistry = std::map<std::string, std::function<NodeStatus()>>;

	BTTreeEditor();
	~BTTreeEditor() = default;

	// 毎フレーム呼ぶ. ツリー編集・保存/読込を行う.
	void Draw();

	// 現在のモデルからランタイムツリーを構築する(Actionsに無いAction名はFailure扱い).
	// DEBUGビルドでは実行中のAction名がGetLastActiveAction()で取得できるよう接続される.
	std::unique_ptr<RootNode> BuildRuntime(const ActionRegistry& Actions);

#if _DEBUG
	// デバッグ表示用: 直近にTickされたActionノード名(実体がDEBUGビルド限定のためgetterもガードを揃える).
	const std::string& GetLastActiveAction() const noexcept { return m_LastActiveAction; }
#endif

	// 編集データを一括差し替える(Undo/Redoコマンドから履歴復元に使う).
	void ApplyModel(const BTTreeModel& Model);

private:
	// ツリー階層の再帰描画(選択・子追加ボタン付き).
	void DrawNodeTree(const std::string& Id, int Depth);

	// 選択中ノードに対する操作UI(子追加・削除・複製・プロパティ編集).
	void DrawSelectedPanel();

	// 編集操作をスナップショット経由でUndoスタックへ積みながら実行する.
	void Mutate(const char* pLabel, const std::function<void(BTTreeModel&)>& Operation);

private:
	static constexpr const char* kDefaultFile = "Data\\Json\\BT\\demo_tree.json";

	BTTreeModel        m_Model;         // 編集中のツリーモデル(JSON可逆).
	EditorCommandStack m_Commands;      // Undo/Redo履歴(共通Editorフレームワーク).
	std::string        m_SelectedId;    // 選択中ノードID(空=なし).
	std::string        m_SaveStatus;    // 保存/読込の結果表示.
	std::string        m_FilePath       = kDefaultFile;
	int                m_NewNodeType    = 0;   // 子追加用の種別選択.
#if _DEBUG
	std::string m_LastActiveAction;            // 直近に実行されたAction名.
#endif
};
