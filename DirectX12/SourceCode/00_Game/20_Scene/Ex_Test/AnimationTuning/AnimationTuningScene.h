#pragma once

#include <memory>
#include <string>
#include <vector>

#include "99_System/Scene/SceneBase.h"

class PMXRenderer;
class PMXActor;
class XActor;
class AnimationEditor;

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/10.
* @brief     : アニメーション調整用シーン(デバッグ専用).
*            : Data\Model\PMX・Data\Model\X配下で見つかった全モデルをドロップダウンで
*            : 切り替えながら、AnimationEditorで再生・調整を確認できる.
**********************************************************************************/

class AnimationTuningScene final : public SceneBase
{
public:
	AnimationTuningScene();
	~AnimationTuningScene() override;

	void Initialize() override;
	void Create() override;
	void Update() override;
	void LateUpdate() override;
	void Draw() override;

private:
	// 1モデルぶんの情報(ドロップダウン表示・切り替え用).
	struct ModelEntry
	{
		std::string DisplayName;       // ドロップダウンに表示する名前(Data\Model\から下の相対パス).
		std::string FilePath;          // ロード時に使う実際のパス.
		bool        IsXFormat = false; // false=PMX(PMXActor)、true=.x(XActor).
	};

	// Data\Model\PMX・Data\Model\X配下を再帰的に走査し、m_ModelList/m_ModelDisplayNamesを構築する.
	void ScanModels();

	// Indexのモデルへ切り替える. 現在のPMXActor/XActor(どちらか一方しか同時に持たない)を破棄し、
	// モデルの形式に応じて対応する方を作り直す.
	void LoadModel(int Index);

private:
	std::shared_ptr<PMXRenderer>     m_pPMXRenderer;
	std::shared_ptr<PMXActor>        m_pPMXActor; // PMXモデル選択中のみ有効.
	std::unique_ptr<XActor>          m_upXActor;  // .xモデル選択中のみ有効.
	std::unique_ptr<AnimationEditor> m_upAnimationEditor;

	std::vector<ModelEntry>  m_ModelList;
	std::vector<std::string> m_ModelDisplayNames; // m_ModelListと対応するDisplayNameの一覧(Combo用).
	std::string              m_SelectedDisplayName;
};
