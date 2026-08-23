#include "TextRenderer.h"

#include <cmath>

#include "SpriteRenderer.h"
#include "20_Resource/Font/FontLoader.h"
#include "99_Utility/Localization/LocalizationTable.h"
#include "99_Utility/String/String.h"

// コンストラクタ.
TextRenderer::TextRenderer(SpriteRenderer& Sprites)
	: m_Sprites(Sprites)
{
}

// 直接文字列(UTF-8)を画面座標へ描画する.
void TextRenderer::DrawText2D(const std::string& Utf8Text,
	float PosX, float PosY, float Scale,
	const DirectX::XMFLOAT4& Color)
{
	DrawText2D((s_ActiveFontId >= 0) ? s_ActiveFontId : FontLoader::GetDefaultFont(), Utf8Text, PosX, PosY, Scale, Color);
}

void TextRenderer::DrawText2D(int FontId, const std::string& Utf8Text,
	float PosX, float PosY, float Scale,
	const DirectX::XMFLOAT4& Color)
{
	// UTF-8 → ワイド文字(日本語を含むため).
	const std::wstring wide = MyString::StringToWString(Utf8Text);
	if (wide.empty()) { return; }

	const float line_height = FontLoader::GetLineHeight(FontId) * Scale;
	if (line_height <= 0.0f) { return; }

	float cursor_x = PosX;
	float cursor_y = PosY;

	for (wchar_t ch : wide)
	{
		if (ch == L'\n')
		{
			cursor_x = PosX;
			cursor_y += line_height;
			continue;
		}

		const FontLoader::Glyph* p_glyph = FontLoader::GetGlyph(FontId, ch);
		if (!p_glyph)
		{
			cursor_x += Scale; // 欠字は既定幅で送る.
			continue;
		}

		if (p_glyph->Width <= 0.0f || p_glyph->AtlasIndex < 0)
		{
			// 空白・アトラス満杯等は描画せず送りだけ進める.
			cursor_x += (p_glyph->AdvanceX > 0.0f) ? p_glyph->AdvanceX * Scale : Scale;
			continue;
		}

		ID3D12Resource* p_atlas = FontLoader::GetAtlasPage(FontId, p_glyph->AtlasIndex);
		if (p_atlas)
		{
			m_Sprites.DrawSprite2DUV(p_atlas,
				cursor_x + p_glyph->OffsetX * Scale,
				cursor_y + p_glyph->OffsetY * Scale,
				p_glyph->Width * Scale,
				p_glyph->Height * Scale,
				p_glyph->U0, p_glyph->V0, p_glyph->U1, p_glyph->V1,
				Color);
		}

		cursor_x += ((p_glyph->AdvanceX > 0.0f) ? p_glyph->AdvanceX : Scale) * Scale;
	}
}

// ローカライズキー版.
void TextRenderer::DrawTextLocalized(int FontId, const std::string& Key,
	float PosX, float PosY, float Scale,
	const DirectX::XMFLOAT4& Color)
{
	DrawText2D(FontId, LocalizationTable::Instance().GetText(Key), PosX, PosY, Scale, Color);
}
