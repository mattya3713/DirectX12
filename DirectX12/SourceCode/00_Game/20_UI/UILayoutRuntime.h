#pragma once

#include <algorithm>
#include <map>
#include <string>

#include "00_Game/10_Object/30_UIObject/UILayoutModel.h"
#include "99_Utility/FileManager/FileManager.h"

/**********************************************************************************
* @author    : Coder 玄武(閃斬 Production Loop).
* @date      : 2026-08-23.
* @brief     : UI Layout Editorの保存形式(layout.json)をランタイムへ取り込む.
*            : JSON解析は起動時/切替時の1回のみ(毎フレームI/Oはしない).
*            : Nameが"PlayerHP"/"BossHP"の要素にはHP比率が幅へ反映され、
*            : ファイル欠損/破損時はLoadDefaultLayout()の既定表示へフォールバックする.
*            : 描画(Draw)のみ.cpp側(D3D12依存). 純粋部分はヘッダinlineで単体テスト可能.
**********************************************************************************/

class Player;
class Boss;
class SpriteRenderer;

// HUD反映用のゲーム値スナップショット(単体テスト可能な純データ).
struct UIHudSnapshot
{
	float PlayerHpRatio = 1.0f; // 0〜1.
	float BossHpRatio   = 1.0f; // 0〜1.
};

class UILayoutRuntime final
{
public:
	UILayoutRuntime() = default;
	~UILayoutRuntime() = default;

	UILayoutRuntime(const UILayoutRuntime&)            = delete;
	UILayoutRuntime& operator=(const UILayoutRuntime&) = delete;

	// layout.jsonを読み込む(欠損/壊れの場合はfalse. 呼び出し側はLoadDefaultLayout()へフォールバックする).
	bool LoadFromJson(const std::string& Path)
	{
		m_Model = UILayoutModel{};
		m_BaseWidths.clear();
		m_IsLoaded = false;

		const nlohmann::json data = FileManager::JsonLoad(Path);
		if (data.empty()) { return false; } // ファイル欠損/パース失敗.

		m_Model.FromJson(data);
		if (m_Model.Elements.empty()) { return false; } // 空レイアウトもフォールバック対象.

		CacheBaseWidths();
		m_IsLoaded = true;
		return true;
	}

	// 既定表示を構築する(Player/BossのHPバー2本のみの最小構成).
	void LoadDefaultLayout()
	{
		constexpr const char* kFallbackImagePath = "Data\\Image\\toon01.bmp";

		m_Model = UILayoutModel{};
		m_BaseWidths.clear();

		UIElementDesc player_bar{};
		player_bar.Id       = "default_player_hp";
		player_bar.Type     = "Sprite";
		player_bar.Name     = "PlayerHP";
		player_bar.ImageId  = kFallbackImagePath;
		player_bar.AnchorX  = 0.03f;
		player_bar.AnchorY  = 0.05f;
		player_bar.Width    = 300.0f;
		player_bar.Height   = 24.0f;
		player_bar.ColorR   = 0.25f;
		player_bar.ColorG   = 0.9f;
		player_bar.ColorB   = 0.3f;
		m_Model.Elements.push_back(player_bar);

		UIElementDesc boss_bar{};
		boss_bar.Id      = "default_boss_hp";
		boss_bar.Type    = "Sprite";
		boss_bar.Name    = "BossHP";
		boss_bar.ImageId  = kFallbackImagePath;
		boss_bar.AnchorX = 0.5f;
		boss_bar.AnchorY = 0.04f;
		boss_bar.OffsetX = -250.0f; // 中央寄せ(幅500の半分を左へ).
		boss_bar.Width   = 500.0f;
		boss_bar.Height  = 20.0f;
		boss_bar.ColorR  = 0.9f;
		boss_bar.ColorG  = 0.25f;
		boss_bar.ColorB  = 0.25f;
		m_Model.Elements.push_back(boss_bar);

		CacheBaseWidths();
		m_IsLoaded = true;
	}

	bool IsLoaded() const noexcept { return m_IsLoaded; }
	const UILayoutModel& GetModel() const noexcept { return m_Model; }
	UILayoutModel&       GetModel() noexcept { return m_Model; }

	// 毎フレーム: ゲーム値をHUD要素へ反映する(Name一致要素のWidth=基準幅x比率).
	void BindGameValues(const UIHudSnapshot& Snapshot)
	{
		for (UIElementDesc& element : m_Model.Elements)
		{
			float ratio = -1.0f;
			if      (element.Name == "PlayerHP") { ratio = Snapshot.PlayerHpRatio; }
			else if (element.Name == "BossHP")   { ratio = Snapshot.BossHpRatio; }

			if (ratio < 0.0f) { continue; }

			const auto it = m_BaseWidths.find(element.Id);
			const float base_width = (it != m_BaseWidths.end()) ? it->second : element.Width;
			element.Width = base_width * std::clamp(ratio, 0.0f, 1.0f);
		}
	}

	// 描画する(.cpp側で実装. D3D12/SpriteRendererへ依存).
	void Draw(SpriteRenderer& Renderer);

private:
	// HPバー化で幅を書き換えるため、読み込み時の基準幅を要素IDごとに記録する.
	void CacheBaseWidths()
	{
		for (const UIElementDesc& element : m_Model.Elements)
		{
			m_BaseWidths[element.Id] = element.Width;
		}
	}

	UILayoutModel m_Model;                     // 起動時に1度だけ解析したレイアウト.
	std::map<std::string, float> m_BaseWidths; // 要素ID→基準幅(HPバーの満タン時幅).
	bool m_IsLoaded = false;                   // layout.jsonまたは既定表示のいずれかが読み込まれた.
};
