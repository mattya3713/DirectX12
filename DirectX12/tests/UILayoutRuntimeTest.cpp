// UILayoutRuntime単体テスト(スタンドアロン. ゲーム本体/D3D12に依存しない純粋な部分のみ検証).
// ビルド例:
//   cl /nologo /EHsc /std:c++20 /W4 /I SourceCode /I Data\Library ^
//     tests\UILayoutRuntimeTest.cpp SourceCode\00_Game\10_Object\30_UIObject\UILayoutModel.cpp
//
// 確認内容:
// 1. 既定表示(PlayerHP/BossHP)が構築されること
// 2. BindGameValuesでHP比率が幅へ反映されること(満タン/半分/ゼロ)
// 3. 名前不一致要素は影響を受けないこと
// 4. アンカー+オフセット→画面位置の計算が解像度に対して正しいこと

#include <algorithm>
#include <iostream>

#include "../SourceCode/00_Game/20_UI/UILayoutRuntime.h"

namespace {

	int g_failures = 0;

	void Check(bool Condition, const char* pLabel)
	{
		std::cout << (Condition ? "[PASS] " : "[FAIL] ") << pLabel << std::endl;
		if (!Condition) { ++g_failures; }
	}

} // namespace

int main()
{
	// --- 1: 既定表示 ---
	UILayoutRuntime runtime;
	runtime.LoadDefaultLayout();
	Check(runtime.IsLoaded(), "1a: 既定表示が読み込まれる");
	Check(runtime.GetModel().Elements.size() >= 2, "1b: HPバー2本以上の要素がある");

	const UIElementDesc* p_player_bar = runtime.GetModel().Find("default_player_hp");
	Check(p_player_bar != nullptr && p_player_bar->Name == "PlayerHP", "1c: PlayerHPバーが存在する");
	Check(p_player_bar != nullptr && p_player_bar->Width > 0.0f, "1d: 満タン時は基準幅のまま");

	// --- 2: HP比率→幅の反映 ---
	UIHudSnapshot snapshot{};
	snapshot.PlayerHpRatio = 0.5f;
	snapshot.BossHpRatio   = 0.25f;
	runtime.BindGameValues(snapshot);

	const UIElementDesc* p_half = runtime.GetModel().Find("default_player_hp");
	Check(p_half != nullptr && p_half->Width > 148.0f && p_half->Width < 152.0f, "2a: PlayerHP幅が50%になる");
	const UIElementDesc* p_quarter = runtime.GetModel().Find("default_boss_hp");
	Check(p_quarter != nullptr && p_quarter->Width > 123.0f && p_quarter->Width < 127.0f, "2b: BossHP幅が25%になる");

	UIHudSnapshot zero{};
	zero.PlayerHpRatio = 0.0f;
	zero.BossHpRatio   = 0.0f;
	runtime.BindGameValues(zero);
	Check(runtime.GetModel().Find("default_player_hp")->Width == 0.0f, "2c: HPゼロで幅もゼロ");

	// --- 3: 名前不一致要素は影響を受けない ---
	UILayoutRuntime named;
	named.LoadDefaultLayout();
	UIElementDesc extra{};
	extra.Id = "extra"; extra.Type = "Sprite"; extra.Name = "Unrelated"; extra.Width = 100.0f;
	named.GetModel().Elements.push_back(extra);
	UIHudSnapshot half{};
	half.PlayerHpRatio = 0.5f;
	half.BossHpRatio   = 0.5f;
	named.BindGameValues(half);
	Check(named.GetModel().Find("extra")->Width == 100.0f, "3: HUD名以外の要素は変化しない");

	// --- 4: アンカー位置計算(UILayoutModel::GetPosition) ---
	UIElementDesc element{};
	element.AnchorX = 0.5f; element.AnchorY = 0.0f; element.OffsetX = -100.0f; element.OffsetY = 20.0f;
	float out_x = 0.0f, out_y = 0.0f;
	UILayoutModel::GetPosition(element, 1920.0f, 1080.0f, out_x, out_y);
	Check(out_x == 860.0f && out_y == 20.0f, "4a: アンカー中央+オフセットの位置計算");

	UILayoutModel::GetPosition(element, 1280.0f, 720.0f, out_x, out_y);
	Check(out_x == 540.0f && out_y == 20.0f, "4b: 解像度変更に追従する");

	std::cout << (g_failures == 0 ? "\nALL TESTS PASSED" : "\nTESTS FAILED") << " (" << g_failures << " failures)" << std::endl;
	return g_failures == 0 ? 0 : 1;
}
