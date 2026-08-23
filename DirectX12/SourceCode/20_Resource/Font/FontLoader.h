#pragma once

#include <d3d12.h>
#include <string>
#include <vector>

#include "..\\..\\..\\Data\\Library\\DirectXTex\\Common\\d3dx12.h"

/**********************************************************************************
* @date      : 2026-08-23.
* @brief     : テキスト描画用のフォントローダー(GDIラスタライズ+アトラスキャッシュ).
*            : システムのTrueTypeフォントからGetGlyphOutlineWでグリフを
*            : ラスタライズし、フォントごとのアトラステクスチャへキャッシュする。
*            : 同一フォントの再ロードは発生せず、未登録文字は初回参照時に展開。
*            : 指定フォントが使えない場合は既定フォント(Meiryo)へフォールバック.
**********************************************************************************/

class FontLoader final
{
public:
	// グリフ情報(アトラスUVとレイアウト用メトリクス. 単位はピクセル).
	struct Glyph
	{
		float U0 = 0.0f; float V0 = 0.0f;
		float U1 = 0.0f; float V1 = 0.0f;
		float Width    = 0.0f; // ビットマップの幅.
		float Height   = 0.0f; // ビットマップの高さ.
		float OffsetX  = 0.0f; // 描画原点からグリフ左までのオフセット.
		float OffsetY  = 0.0f; // 描画原点(ベースライン)からグリフ上までのオフセット.
		float AdvanceX = 0.0f; // 次の文字への送り幅.
	};

	// フォントを読み込み、フォントIDを返す(同一指定はキャッシュを返す. 失敗時は-1).
	static int LoadFont(const std::wstring& FontName, int PixelHeight);

	// 既定フォント(Meiryo)のIDを返す(未ロードならロードする. 失敗時は-1).
	static int GetDefaultFont();

	// 指定文字のグリフを取得する(未展開ならその場でラスタライズ. 空白等はnullptr).
	static const Glyph* GetGlyph(int FontId, wchar_t Char);

	// アトラステクスチャ(TextRendererがこれをサンプリングする).
	static ID3D12Resource* GetAtlas(int FontId);

	// 行の高さ(ピクセル. 改行送りに使用).
	static float GetLineHeight(int FontId);

private:
	// 1フォント分のキャッシュデータ.
	struct FontCache
	{
		ID3D12Resource*                pAtlas      = nullptr; // RGBA8アップロードヒープ.
		std::wstring                   FaceName;              // ラスタライズに使うフェイス名.
		std::vector<Glyph>             Glyphs;
		std::vector<wchar_t>           Chars;
		float                          LineHeight  = 0.0f;
		float                          Ascent      = 0.0f;
		UINT                           CursorX     = 0;       // アトラス内の書き込み位置(shelf packing).
		UINT                           CursorY     = 0;
	};

	static FontCache* FindCache(int FontId);
	static int CreateCache(const std::wstring& FaceName, int PixelHeight);
	static const Glyph* RasterizeGlyph(FontCache& Cache, HDC Dc, HFONT Font, wchar_t Char);
};
