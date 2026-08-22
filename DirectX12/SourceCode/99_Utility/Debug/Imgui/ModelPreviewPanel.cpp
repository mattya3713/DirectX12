#include "ModelPreviewPanel.h"

#include <algorithm>
#include <cctype>
#include <filesystem>

#include "10_Ggraphic/10_Device/DirectX/DirectX12.h"
#include "10_Ggraphic/30_Asset/RuntimeModel/MMdl/MmdlRenderer.h"
#include "10_Ggraphic/30_Asset/RuntimeModel/MMdl/MMdlActor.h"
#include "99_Utility/Debug/Imgui/AnimationEditor.h"
#include "99_Utility/Debug/Imgui/ActionTimelineEditor.h"
#include "99_Utility/Debug/Imgui/ImGuiManager.h"
#include "99_Utility/ServiceLocator/ServiceLocator.h"
#include "99_Utility/String/String.h"

namespace {
	// 拡張子を小文字化して比較する.
	bool HasExtension(const std::filesystem::path& FilePath, const std::string& LowerExt)
	{
		std::string ext = FilePath.extension().string();
		std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		return ext == LowerExt;
	}
}

ModelPreviewPanel::ModelPreviewPanel(MmdlRenderer& Renderer)
	: m_Renderer(Renderer)
{
	ScanModels();
	if (!m_ModelList.empty()) {
		LoadModel(0);
	}

	m_upAnimationEditor = std::make_unique<AnimationEditor>();
	m_upAnimationEditor->Toggle(); // 常時表示する(トグルではなくデフォルトON扱い).

	m_upActionTimelineEditor = std::make_unique<ActionTimelineEditor>();
}

ModelPreviewPanel::~ModelPreviewPanel() = default;

// Data\Model\mmdl\mskin配下の.msknだけを再帰的に探す.
void ModelPreviewPanel::ScanModels()
{
	namespace fs = std::filesystem;

	m_ModelList.clear();
	m_ModelDisplayNames.clear();

	const fs::path mskin_root = "Data\\Model\\mmdl\\mskin";
	if (fs::exists(mskin_root)) {
		for (const auto& entry : fs::recursive_directory_iterator(mskin_root)) {
			if (!entry.is_regular_file() || !HasExtension(entry.path(), ".mskn")) { continue; }

			ModelEntry model;
			model.FilePath    = entry.path().string();
			model.DisplayName = fs::relative(entry.path(), mskin_root).string();
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

	// 現在のランタイムアクターを破棄する.
	m_upActor.reset();

	const ModelEntry& model = m_ModelList[Index];

	try {
		m_upActor = std::make_unique<MmdlActor>(std::filesystem::path{ model.FilePath }, m_Renderer);
		const float local_height = m_upActor->GetLocalHeight();
		const float preview_scale = local_height > 0.0001f ? std::clamp(20.0f / local_height, 0.1f, 100.0f) : 1.0f;
		m_upActor->SetWorldMatrix(DirectX::XMMatrixScaling(preview_scale, preview_scale, preview_scale));
		const auto& clips = m_upActor->GetClips();
		if (!clips.empty()) {
			m_upActor->PlayAnimation(clips.front().Name);
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
	ImGui::Begin("Model Select");
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
		ImGuiManager::Text("Data\\Model\\mmdl\\mskinにMSKNが見つかりませんでした.");
	}

	const float previous_action_frame = m_ActionFrame;
	ImGuiManager::Tweak("Action Frame (Debug)", m_ActionFrame, 0.0f, 120.0f);
	ImGui::End();

	if (m_ActionFrame != previous_action_frame)
	{
		if (m_upActor) { m_upActor->SetCurrentFrame(m_ActionFrame); }
	}

	if (m_upActor) {
		m_upActor->Update();
		m_upAnimationEditor->Draw(*m_upActor);
		m_upActionTimelineEditor->Draw(*m_upActor);
	}
}

void ModelPreviewPanel::Draw()
{
	if (m_upActor) { m_upActor->Draw(); }
}
