#pragma once

#include <d3d12.h>
#include <DirectXMath.h>
#include <string>

class SpriteRenderer;

/**********************************************************************************
* @date      : 2026-08-23.
* @brief     : FontLoaderのグリフを使った画面空間テキスト描画.
*            : Sprite2Dパイプラインへグリフ四角形を流し込む薄いレイヤーで、
*            : 改行・スケール・色に対応する。ローカライズキー版も用意.
**********************************************************************************/

class TextRenderer final
{
public:
	explicit TextRenderer(SpriteRenderer& Sprites);

	// 既定フォントIDの取得/切替(FontRegistry::Resolveの結果を設定する).
	static int  GetActiveFontId() noexcept { return s_ActiveFontId; }
	static void SetActiveFont(int FontId) noexcept { s_ActiveFontId = FontId; }

	// 直接文字列(UTF-8)を画面座標へ描画する(アクティブフォント使用).
	void DrawText2D(const std::string& Utf8Text,
		float PosX, float PosY, float Scale,
		const DirectX::XMFLOAT4& Color = { 1.0f, 1.0f, 1.0f, 1.0f });

	// 直接文字列(UTF-8)を画面座標へ描画する(フォントID明示).
	void DrawText2D(int FontId, const std::string& Utf8Text,
		float PosX, float PosY, float Scale,
		const DirectX::XMFLOAT4& Color = { 1.0f, 1.0f, 1.0f, 1.0f });

	// ローカライズキー版(LocalizationTable::GetText()で文字列を解決してから描画).
	void DrawTextLocalized(int FontId, const std::string& Key,
		float PosX, float PosY, float Scale,
		const DirectX::XMFLOAT4& Color = { 1.0f, 1.0f, 1.0f, 1.0f });

private:
	SpriteRenderer& m_Sprites;
	static inline int s_ActiveFontId = -1; // -1=FontLoader既定.
};
