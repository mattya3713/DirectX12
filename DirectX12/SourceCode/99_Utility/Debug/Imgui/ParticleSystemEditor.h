#pragma once

/**********************************************************************************
* @author    : Coder 青龍(せいりゅう).
* @date      : 2026/08/23.
* @brief     : ParticleSystemの発生パラメータを編集し、プリセット保存/読込/
*            : プレビュー再生を行うImGuiツール(_DEBUG限定).
**********************************************************************************/

#include <string>

#include "10_Ggraphic/20_Render/Particle/ParticleSystem.h"

class ParticleSystemEditor final
{
public:
	ParticleSystemEditor() = default;
	~ParticleSystemEditor() = default;

	// 毎フレーム呼ぶ(DebugビルドのMainSceneから).
	void Draw();

private:
	ParticleSystem::EmitterParams m_Params;      // 編集中のパラメータ.
	std::string                   m_PresetName = "default_hit"; // プリセット名入力.
};
