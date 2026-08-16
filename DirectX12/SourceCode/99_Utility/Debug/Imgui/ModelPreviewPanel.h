#pragma once

#include <memory>
#include <string>
#include <vector>

class MmdlRenderer;
class MmdlActor;
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
	explicit ModelPreviewPanel(MmdlRenderer& Renderer);
	~ModelPreviewPanel();

	ModelPreviewPanel(const ModelPreviewPanel&)            = delete;
	ModelPreviewPanel& operator=(const ModelPreviewPanel&) = delete;

	void Update();
	void Draw();

private:
	// 1モデルぶんの情報(ドロップダウン表示・切り替え用).
	struct ModelEntry
	{
		std::string DisplayName;       // ドロップダウンに表示する名前.
		std::string FilePath;          // ロード時に使うMSKNのパス.
	};

	// Data\Model\PMX・Data\Model\X配下を再帰的に走査し、m_ModelList/m_ModelDisplayNamesを構築する.
	void ScanModels();

	// Indexのランタイムモデルへ切り替える.
	void LoadModel(int Index);

private:
	MmdlRenderer& m_Renderer;

	std::unique_ptr<MmdlActor>          m_upActor; // ランタイムモデル選択中のみ有効.
	std::unique_ptr<AnimationEditor> m_upAnimationEditor;

	std::vector<ModelEntry>  m_ModelList;
	std::vector<std::string> m_ModelDisplayNames; // m_ModelListと対応するDisplayNameの一覧(Combo用).
	std::string              m_SelectedDisplayName;
	float                    m_ActionFrame = 0.0f; // 外部駆動確認用のActionFrame.
};
