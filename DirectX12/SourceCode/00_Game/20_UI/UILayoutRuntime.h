#pragma once

#include <filesystem>
#include <map>
#include <string>
#include <vector>

#include <DirectXMath.h>

#include "00_Game/10_Object/30_UIObject/UILayoutModel.h"

class DirectX12;
class SpriteRenderer;

/**********************************************************************************
* @date      : 2026-08-23.
* @brief     : UI Layout EditorのレイアウトJSONを実行時に再生する最小ランタイム.
*            : Sprite要素をUISpriteObject相当の値として保持し、アンカー＋オフセット・
*            : レイヤー順・表示状態を維持したままSprite2Dへ描画する。
*            : 要素名に対応する塗り比(SetRatio)を設定すると幅が比率分になるため、
*            : HPバーとして使える。Text要素はフォント未整備のためスキップする。
*            : レイアウトが無い/壊れている場合はコード内蔵の既定HUDへフォールバック.
**********************************************************************************/

// 描画1枚分の確定情報(BuildDrawItemsの出力. 実デバイスに依存しないのでテスト可能).
struct UISpriteDrawItem
{
	std::string       ImageId;                 // 画像アセット識別子.
	float             PosX   = 0.0f;           // 描画位置X(px).
	float             PosY   = 0.0f;           // 描画位置Y(px).
	float             Width  = 0.0f;           // 幅(塗り比適用後).
	float             Height = 0.0f;           // 高さ.
	DirectX::XMFLOAT4 Color  { 1.0f, 1.0f, 1.0f, 1.0f }; // 色(RGBA).
};

class UILayoutRuntime final
{
public:
	UILayoutRuntime() = default;
	~UILayoutRuntime() = default;

	// レイアウトJSONを読み込む(ファイル無し/壊れ/Sprite要素0件はfalse).
	bool LoadFromFile(const std::filesystem::path& Path);

	// コード内蔵の最小HUDレイアウトへ切り替える(Player/BossのHPバーのみ).
	void LoadDefault();

	// 読み込みを試み、失敗したら既定レイアウトへフォールバックする.
	void LoadOrDefault(const std::filesystem::path& Path);

	// 名前指定でバーの塗り比を設定する(0〜1にクランプ. 未登録の名前は無視).
	void SetRatio(const std::string& Name, float Ratio);

	// 保持している要素の取得(デバッグ表示・テスト用).
	const std::vector<UIElementDesc>& GetElements() const noexcept { return m_Elements; }

	// 描画内容を確定する(レイヤー順・可視のみ・塗り比適用. 純粋関数なので単体テスト可能).
	static std::vector<UISpriteDrawItem> BuildDrawItems(
		const std::vector<UIElementDesc>& Elements,
		const std::map<std::string, float>& Ratios,
		float ScreenWidth, float ScreenHeight);

	// 保持しているSprite要素をSprite2Dで描画する(解像度はバックバッファから取得).
	void Draw(SpriteRenderer& Renderer, DirectX12& Dx12);

private:
	std::vector<UIElementDesc>   m_Elements; // 描画対象の要素(Spriteのみ).
	std::map<std::string, float> m_Ratios;   // 要素名→塗り比(HPバー等).
};
