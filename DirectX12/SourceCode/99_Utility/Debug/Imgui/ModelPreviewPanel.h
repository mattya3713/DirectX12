#pragma once

#include <memory>
#include <string>
#include <vector>

class PMXRenderer;
class PMXActor;
class XActor;
class AnimationEditor;

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/12.
* @brief     : Data\Model\PMX・Data\Model\X配下で見つかった全モデルをドロップダウンで
*            : 切り替えながら、AnimationEditorで再生・調整を確認できるデバッグ専用パネル.
*            : AnimationTuningScene専用だった機能を、シーンを問わずどこからでも(MainSceneに
*            : 常駐させる場合も含めて)使える部品として切り出したもの. Unityの「Sceneビュー/
*            : Gameビュー」のように、シーンを切り替えずにいつでもモデル確認ができるようにする狙い.
**********************************************************************************/

class ModelPreviewPanel final
{
public:
	explicit ModelPreviewPanel(PMXRenderer& Renderer);
	~ModelPreviewPanel();

	ModelPreviewPanel(const ModelPreviewPanel&)            = delete;
	ModelPreviewPanel& operator=(const ModelPreviewPanel&) = delete;

	void Update();
	void Draw();

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
	PMXRenderer& m_Renderer;

	std::shared_ptr<PMXActor>        m_pPMXActor; // PMXモデル選択中のみ有効.
	std::unique_ptr<XActor>          m_upXActor;  // .xモデル選択中のみ有効.
	std::unique_ptr<AnimationEditor> m_upAnimationEditor;

	std::vector<ModelEntry>  m_ModelList;
	std::vector<std::string> m_ModelDisplayNames; // m_ModelListと対応するDisplayNameの一覧(Combo用).
	std::string              m_SelectedDisplayName;
	float                    m_ActionFrame = 0.0f; // 外部駆動確認用のActionFrame.
};
