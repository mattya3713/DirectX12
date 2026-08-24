#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "json/json.hpp"

/**********************************************************************************
* @date      : 2026-08-23.
* @brief     : UI Layout Editorが扱うレイアウトのデータモデル(JSON可逆).
*            : UISpriteObject等のランタイムから独立した純データであり、UI要素の
*            : 配置・アンカー・レイヤー・表示状態をすべてこのモデルへ保持する.
*            : 要素の描画位置は「アンカー(画面比)＋オフセット(px)」から解像度ごとに
*            : 導出するため、解像度変更時は再計算関数を呼ぶだけで追従する.
**********************************************************************************/

struct UIElementDesc
{
	std::string Id;        // 一意ID("ui1"等. エディタが採番).
	std::string Type;      // "Sprite" / "Text".
	std::string Name;      // 表示名.
	std::string ImageId;   // Sprite用の画像アセット識別子.
	std::string Text;      // Text用の文字列.
	float AnchorX = 0.5f;  // アンカーX(画面幅に対する比0〜1).
	float AnchorY = 0.5f;  // アンカーY(画面高さに対する比0〜1).
	float OffsetX = 0.0f;  // アンカーからのオフセットX(px).
	float OffsetY = 0.0f;  // アンカーからのオフセットY(px).
	float Width   = 100.0f; // 幅(px).
	float Height  = 32.0f;  // 高さ(px).
	float RotationDeg = 0.0f; // 回転角(度. 要素中心周り).
	int   Layer   = 0;     // レイヤー深度(大きいほど前面).
	float ColorR  = 1.0f;  // 色(R).
	float ColorG  = 1.0f;  // 色(G).
	float ColorB  = 1.0f;  // 色(B).
	float ColorA  = 1.0f;  // 色(A).
	bool  IsVisible = true; // 表示状態.

	bool operator==(const UIElementDesc& Other) const noexcept
	{
		return Id == Other.Id && Type == Other.Type && Name == Other.Name &&
		       ImageId == Other.ImageId && Text == Other.Text &&
		       AnchorX == Other.AnchorX && AnchorY == Other.AnchorY &&
		       OffsetX == Other.OffsetX && OffsetY == Other.OffsetY &&
		       Width == Other.Width && Height == Other.Height &&
		       RotationDeg == Other.RotationDeg && Layer == Other.Layer &&
		       ColorR == Other.ColorR && ColorG == Other.ColorG &&
		       ColorB == Other.ColorB && ColorA == Other.ColorA &&
		       IsVisible == Other.IsVisible;
	}

	bool operator!=(const UIElementDesc& Other) const noexcept
	{
		return !(*this == Other);
	}
};

class UILayoutModel
{
public:
	std::vector<UIElementDesc> Elements;

	// 要素検索(見つからなければnullptr).
	const UIElementDesc* Find(const std::string& Id) const;
	UIElementDesc*       Find(const std::string& Id);

	// 新規要素を追加する(IDは自動採番). 追加したIDを返す.
	std::string AddElement(const std::string& Type, const std::string& Name);

	// 指定要素を削除する(存在しなければfalse).
	bool RemoveElement(const std::string& Id);

	// 指定要素を複製する(新IDを採番し、少し右へずらす). 成功時は複製IDを返す.
	std::string DuplicateElement(const std::string& Id);

	// アンカー＋オフセットから画面上の描画位置(px)を計算する.
	static void GetPosition(const UIElementDesc& Element, float ScreenWidth, float ScreenHeight, float& OutX, float& OutY);

	// レイヤー昇順(同値は登録順)に並べ替えた要素IDリストを返す.
	std::vector<std::string> GetSortedIdsByLayer() const;

	// ---- JSON変換 ----
	nlohmann::json ToJson() const;
	void           FromJson(const nlohmann::json& Data);

	// モデル全体の同値比較(Undo履歴判定用).
	static bool Equal(const UILayoutModel& A, const UILayoutModel& B);
};
