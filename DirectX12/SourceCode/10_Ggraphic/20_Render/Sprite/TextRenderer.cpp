#include "TextRenderer.h"

#include <cmath>

#include "SpriteRenderer.h"
#include "20_Resource/Font/FontLoader.h"
#include "99_Utility/Localization/LocalizationTable.h"
#include "99_Utility/String/String.h"

// 直接文字列(UTF-8)を画面座標へ描画する.
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
		if (!p_glyph || p_glyph->Width <= 0.0f)
		{
			// グリフ無し(空白等)は送り幅だけ進める.
			cursor_x += Scale;
			continue;
		}

		ID3D12Resource* p_atlas = FontLoader::GetAtlas(FontId);
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

		cursor_x += p_glyph->AdvanceX * Scale;
	}
}

// ローカライズキー版.
void TextRenderer::DrawTextLocalized(int FontId, const std::string& Key,
	float PosX, float PosY, float Scale,
	const DirectX::XMFLOAT4& Color)
{
	DrawText2D(FontId, LocalizationTable::Instance().GetText(Key), PosX, PosY, Scale, Color);
}
