#include "ModelPreviewPanel.h"

#include <algorithm>
#include <cctype>
#include <filesystem>

#include "10_Ggraphic/DirectX/DirectX12.h"
#include "10_Ggraphic/PMX/PMXActor.h"
#include "10_Ggraphic/PMX/PMXRenderer.h"
#include "10_Ggraphic/X/XActor.h"
#include "99_Utility/Debug/Imgui/AnimationEditor.h"
#include "99_Utility/Debug/Imgui/ImGuiManager.h"
#include "99_Utility/ServiceLocator/ServiceLocator.h"
#include "99_Utility/String/String.h"

namespace {
	// 拡張子を小文字化して比較する(Data\Model\X配下はCube.x/player.Xのように大文字小文字が混在するため).
	bool HasExtension(const std::filesystem::path& FilePath, const std::string& LowerExt)
	{
		std::string ext = FilePath.extension().string();
		std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		return ext == LowerExt;
	}
}

ModelPreviewPanel::ModelPreviewPanel(PMXRenderer& Renderer)
	: m_Renderer(Renderer)
{
	ScanModels();
	if (!m_ModelList.empty()) {
		LoadModel(0);
	}

	m_upAnimationEditor = std::make_unique<AnimationEditor>();
	m_upAnimationEditor->Toggle(); // 常時表示する(トグルではなくデフォルトON扱い).
}

ModelPreviewPanel::~ModelPreviewPanel() = default;

// Data\Model\PMX配下の.pmx、Data\Model\X配下の.xをそれぞれ再帰的に探し、m_ModelListへ集約する.
void ModelPreviewPanel::ScanModels()
{
	namespace fs = std::filesystem;

	m_ModelList.clear();
	m_ModelDisplayNames.clear();

	const fs::path pmx_root = "Data\\Model\\PMX";
	if (fs::exists(pmx_root)) {
		for (const auto& entry : fs::recursive_directory_iterator(pmx_root)) {
			if (!entry.is_regular_file() || !HasExtension(entry.path(), ".pmx")) { continue; }

			ModelEntry model;
			model.FilePath    = entry.path().string();
			model.DisplayName = "PMX/" + fs::relative(entry.path(), pmx_root).string();
			model.IsXFormat   = false;
			m_ModelDisplayNames.push_back(model.DisplayName);
			m_ModelList.push_back(std::move(model));
		}
	}

	const fs::path x_root = "Data\\Model\\X";
	if (fs::exists(x_root)) {
		for (const auto& entry : fs::recursive_directory_iterator(x_root)) {
			if (!entry.is_regular_file() || !HasExtension(entry.path(), ".x")) { continue; }

			ModelEntry model;
			model.FilePath    = entry.path().string();
			model.DisplayName = "X/" + fs::relative(entry.path(), x_root).string();
			model.IsXFormat   = true;
			m_ModelDisplayNames.push_back(model.DisplayName);
			m_ModelList.push_back(std::move(model));
		}
	}
}

void ModelPreviewPanel::LoadModel(int Index)
{
	if (Index < 0 || Index >= static_cast<int>(m_ModelList.size())) { return; }

	// GPUがまだ参照中かもしれないリソースを破棄する前に完了を待つ
	// (フレームインフライト方式のため、待たずに破棄するとアプリが不正終了する).
	if (DirectX12* p_dx12 = ServiceLocator::Get<DirectX12>()) {
		p_dx12->WaitForGPU();
	}

	// 現在のアクターを破棄する(PMX/Xどちらか一方しか同時に持たない).
	m_pPMXActor.reset();
	m_upXActor.reset();

	const ModelEntry& model = m_ModelList[Index];

	try {
		if (model.IsXFormat) {
			m_upXActor = std::make_unique<XActor>(model.FilePath.c_str(), m_Renderer);
			// .xはPMXよりスケール単位が小さいため、MainSceneと同じ15倍を掛けて見た目を合わせる.
			m_upXActor->SetWorldMatrix(DirectX::XMMatrixScaling(15.0f, 15.0f, 15.0f));
			// Editorには一時停止/Stepの概念が無く常時再生のため、先頭クリップを既定で再生しておく.
			const auto& clips = m_upXActor->GetClips();
			if (!clips.empty()) {
				m_upXActor->PlayAnimation(clips.front().Name);
			}
		}
		else {
			m_pPMXActor = std::make_shared<PMXActor>(model.FilePath.c_str(), m_Renderer);
			// Editorは既定で一時停止のため、StepFrameで初期姿勢だけ計算しておく(でないと表示されない).
			m_pPMXActor->StepFrame();
		}
		m_SelectedDisplayName = model.DisplayName;
	}
	catch (const std::runtime_error& Msg) {
		// エラーメッセージを表示(未捕捉のまま伝播させてabortするのを防ぐ).
		std::wstring w_str = MyString::StringToWString(Msg.what());
		_ASSERT_EXPR(false, w_str.c_str());
	}
}

void ModelPreviewPanel::Update()
{
	// モデル切り替え用のドロップダウン(見つかった全モデルが対象).
	// 他のデバッグウィンドウと重ならない初期位置(初回起動時のみ).
	ImGui::SetNextWindowPos(ImVec2(500.0f, 20.0f), ImGuiCond_FirstUseEver);
	ImGui::Begin("Model Select", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
	if (!m_ModelList.empty()) {
		const std::string previous = m_SelectedDisplayName;
		ImGuiManager::Combo("Model", m_SelectedDisplayName, m_ModelDisplayNames, true);

		if (m_SelectedDisplayName != previous) {
			for (int i = 0; i < static_cast<int>(m_ModelList.size()); ++i) {
				if (m_ModelList[i].DisplayName == m_SelectedDisplayName) {
					LoadModel(i);
					break;
				}
			}
		}
	}
	else {
		ImGuiManager::Text("Data\\Model\\PMX、Data\\Model\\Xにモデルが見つかりませんでした.");
	}
	ImGui::End();

	if (m_pPMXActor) {
		const bool step_requested = m_upAnimationEditor->Draw(*m_pPMXActor);

		if (step_requested) {
			m_pPMXActor->StepFrame();
		}
	}
	else if (m_upXActor) {
		// XActorには一時停止/Stepの概念が無く常時再生のため、毎フレームUpdateする.
		m_upXActor->Update();
		m_upAnimationEditor->Draw(*m_upXActor);
	}
}

void ModelPreviewPanel::Draw()
{
	if (m_pPMXActor) {
		m_pPMXActor->Draw();
	}
	else if (m_upXActor) {
		m_upXActor->Draw();
	}
}
