#include "stdafx.h"
#include "ParticleSystemEditor.h"

#include <filesystem>

#include "ImGuiManager.h"
#include "00_Game/10_Object/10_MeshObject/00_Character/00_Player/Player.h"
#include "99_Utility/FileManager/FileManager.h"
#include "99_Utility/ServiceLocator/ServiceLocator.h"

namespace {
	constexpr const char* kJsonDir = "Data/Json/Particle";

	nlohmann::json ParamsToJson(const ParticleSystem::EmitterParams& Params)
	{
		return {
			{ "Count",        Params.Count },
			{ "SpeedMin",     Params.SpeedMin },
			{ "SpeedMax",     Params.SpeedMax },
			{ "LifeTimeMin",  Params.LifeTimeMin },
			{ "LifeTimeMax",  Params.LifeTimeMax },
			{ "Size",         Params.Size },
			{ "GravityScale", Params.GravityScale },
			{ "Color",        nlohmann::json::array({ Params.Color.x, Params.Color.y, Params.Color.z, Params.Color.w }) },
			{ "TextureName",  Params.TextureName },
		};
	}

	void ParamsFromJson(const nlohmann::json& JsonData, ParticleSystem::EmitterParams& OutParams)
	{
		OutParams.Count        = JsonData.value("Count", OutParams.Count);
		OutParams.SpeedMin     = JsonData.value("SpeedMin", OutParams.SpeedMin);
		OutParams.SpeedMax     = JsonData.value("SpeedMax", OutParams.SpeedMax);
		OutParams.LifeTimeMin  = JsonData.value("LifeTimeMin", OutParams.LifeTimeMin);
		OutParams.LifeTimeMax  = JsonData.value("LifeTimeMax", OutParams.LifeTimeMax);
		OutParams.Size         = JsonData.value("Size", OutParams.Size);
		OutParams.GravityScale = JsonData.value("GravityScale", OutParams.GravityScale);

		if (JsonData.contains("Color") && JsonData["Color"].is_array() && JsonData["Color"].size() >= 4) {
			OutParams.Color.x = JsonData["Color"][0].get<float>();
			OutParams.Color.y = JsonData["Color"][1].get<float>();
			OutParams.Color.z = JsonData["Color"][2].get<float>();
			OutParams.Color.w = JsonData["Color"][3].get<float>();
		}
		OutParams.TextureName = JsonData.value("TextureName", "");
	}
}

void ParticleSystemEditor::Draw()
{
	if (!ImGui::Begin("Particle Editor")) {
		ImGui::End();
		return;
	}

	ParticleSystem::EmitterParams params = m_Params;

	// 発生パラメータ.
	ImGui::SliderInt(IMGUI_JP("発生数"), &params.Count, 1, 64);
	ImGui::DragFloat(IMGUI_JP("速度(最小)"), &params.SpeedMin, 0.05f, 0.0f, 20.0f, "%.2f");
	ImGui::DragFloat(IMGUI_JP("速度(最大)"), &params.SpeedMax, 0.05f, 0.0f, 30.0f, "%.2f");
	if (params.SpeedMax < params.SpeedMin) { params.SpeedMax = params.SpeedMin; }
	ImGui::DragFloat(IMGUI_JP("寿命(最小秒)"), &params.LifeTimeMin, 0.01f, 0.05f, 5.0f, "%.2f");
	ImGui::DragFloat(IMGUI_JP("寿命(最大秒)"), &params.LifeTimeMax, 0.01f, 0.05f, 10.0f, "%.2f");
	if (params.LifeTimeMax < params.LifeTimeMin) { params.LifeTimeMax = params.LifeTimeMin; }
	ImGui::DragFloat(IMGUI_JP("サイズ"), &params.Size, 0.005f, 0.01f, 1.0f, "%.3f");
	ImGui::DragFloat(IMGUI_JP("重力係数"), &params.GravityScale, 0.02f, 0.0f, 5.0f, "%.2f");
	ImGui::ColorEdit4(IMGUI_JP("色"), &params.Color.x);

	char texture_buffer[128] = {};
	params.TextureName.copy(texture_buffer, sizeof(texture_buffer) - 1);
	if (ImGui::InputText(IMGUI_JP("テクスチャ名(保存のみ)"), texture_buffer, sizeof(texture_buffer))) {
		params.TextureName = texture_buffer;
	}

	m_Params = params;

	ImGui::Separator();

	// プレビュー操作.
	if (ImGui::Button(IMGUI_JP("プレビュー再生"))) {
		if (ParticleSystem* p_particle_system = ServiceLocator::Get<ParticleSystem>()) {
			p_particle_system->SetEmitterParams(m_Params);

			DirectX::XMFLOAT3 position = { 0.0f, 1.0f, 0.0f };
			if (Player* p_player = ServiceLocator::Get<Player>()) { position = p_player->GetPosition(); position.y += 1.0f; }
			p_particle_system->SpawnHitEffect(position);
		}
	}
	ImGui::SameLine();
	if (ImGui::Button(IMGUI_JP("停止"))) {
		if (ParticleSystem* p_particle_system = ServiceLocator::Get<ParticleSystem>()) {
			p_particle_system->ClearParticles();
		}
	}

	ImGui::Separator();

	// プリセット保存/読込.
	char name_buffer[128] = {};
	m_PresetName.copy(name_buffer, sizeof(name_buffer) - 1);
	if (ImGui::InputText(IMGUI_JP("プリセット名"), name_buffer, sizeof(name_buffer))) {
		m_PresetName = name_buffer;
	}

	if (ImGui::Button(IMGUI_JP("プリセット保存")) && !m_PresetName.empty()) {
		std::error_code ec{};
		std::filesystem::create_directories(kJsonDir, ec);
		FileManager::JsonSave(std::filesystem::path{ kJsonDir } / (m_PresetName + ".json"), ParamsToJson(m_Params));
	}
	ImGui::SameLine();
	if (ImGui::Button(IMGUI_JP("プリセット読込")) && !m_PresetName.empty()) {
		const nlohmann::json data = FileManager::JsonLoad(std::filesystem::path{ kJsonDir } / (m_PresetName + ".json"));
		if (!data.is_null()) {
			ParamsFromJson(data, m_Params);
			if (ParticleSystem* p_particle_system = ServiceLocator::Get<ParticleSystem>()) {
				p_particle_system->SetEmitterParams(m_Params);
			}
		}
	}

	ImGui::End();
}
