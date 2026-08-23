#include "FontLoader.h"

#include <map>
#include <windows.h>

#include "10_Ggraphic/10_Device/DirectX/DirectX12.h"
#include "99_Utility/ServiceLocator/ServiceLocator.h"

namespace {
	// アトラス解像度(shelf packingで右→下へ詰める).
	constexpr UINT ATLAS_W = 512;
	constexpr UINT ATLAS_H = 512;
	constexpr UINT GLYPH_PAD = 2;
	constexpr float GRAY_LEVELS = 64.0f; // GGO_GRAY8_BITMAPの濃淡値(1..64).

	struct GlyphKeyHash; // 未使用(将来の高速化用プレースホルダ).

	std::map<int, FontLoader::FontCache>& Caches()
	{
		static std::map<int, FontLoader::FontCache> caches;
		return caches;
	}

	int g_NextId = 1;
}

// フォントを読み込み、フォントIDを返す.
int FontLoader::LoadFont(const std::wstring& FontName, int PixelHeight)
{
	const int id = CreateCache(FontName, PixelHeight);
	if (id >= 0) { return id; }

	// 要求フォントが使えない場合は既定フォントへフォールバックする.
	return CreateCache(L"Meiryo", PixelHeight);
}

// 既定フォントのIDを返す.
int FontLoader::GetDefaultFont()
{
	static int default_id = LoadFont(L"Meiryo", 32);
	return default_id;
}

FontLoader::FontCache* FontLoader::FindCache(int FontId)
{
	const auto it = Caches().find(FontId);
	return (it != Caches().end()) ? &it->second : nullptr;
}

// フォントキャッシュとアトラスを生成する.
int FontLoader::CreateCache(const std::wstring& FaceName, int PixelHeight)
{
	DirectX12* p_dx12 = ServiceLocator::Get<DirectX12>();
	if (!p_dx12 || !p_dx12->GetDevice()) { return -1; }

	HDC dc = CreateCompatibleDC(nullptr);
	if (!dc) { return -1; }

	HFONT font = CreateFontW(
		-PixelHeight, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
		DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
		CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, FaceName.c_str());
	if (!font) { DeleteDC(dc); return -1; }

	HGDIOBJ old_font = SelectObject(dc, font);

	// 動作確認を兼ねて「M」がラスタライズできるかでフォントの有効性を判定する.
	GLYPHMETRICS probe{};
	MAT2 mat = { { 0, 1 }, { 0, 0 }, { 0, 0 }, { 0, 2 } }; // 単位行列(16.16固定小数).
	const UINT probe_size = GetGlyphOutlineW(dc, L'M', GGO_GRAY8_BITMAP, &probe, 0, nullptr, &mat);
	if (probe_size == GDI_ERROR || probe_size == 0)
	{
		SelectObject(dc, old_font);
		DeleteObject(font);
		DeleteDC(dc);
		return -1;
	}

	TEXTMETRICW tm{};
	GetTextMetricsW(dc, &tm);

	FontCache cache{};
	cache.FaceName  = FaceName;
	cache.LineHeight = static_cast<float>(tm.tmHeight);
	cache.Ascent     = static_cast<float>(tm.tmAscent);

	// ---- アトラス(UPLOADヒープのRGBA8. CPUからグリフビットマップを直接書き込む) ----
	D3D12_RESOURCE_DESC atlas_desc = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_R8G8B8A8_UNORM, ATLAS_W, ATLAS_H);
	D3D12_HEAP_PROPERTIES upload_heap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	MyAssert::IsFailed(
		_T("フォントアトラステクスチャの作成"),
		&ID3D12Device::CreateCommittedResource, p_dx12->GetDevice(),
		&upload_heap, D3D12_HEAP_FLAG_NONE, &atlas_desc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(&cache.pAtlas));

	const int new_id = g_NextId++;
	Caches()[new_id] = std::move(cache);

	SelectObject(dc, old_font);
	DeleteObject(font);
	DeleteDC(dc);

	return new_id;
}

// 指定文字のグリフを取得する(未展開ならラスタライズしてアトラスへ書き込む).
const FontLoader::Glyph* FontLoader::GetGlyph(int FontId, wchar_t Char)
{
	FontCache* p_cache = FindCache(FontId);
	if (!p_cache || !p_cache->pAtlas) { return nullptr; }

	for (size_t i = 0; i < p_cache->Chars.size(); ++i)
	{
		if (p_cache->Chars[i] == Char) { return &p_cache->Glyphs[i]; }
	}

	// 空白類はグリフを持たない(送り幅だけ進めて見えない仮グリフを返す).
	HDC dc = CreateCompatibleDC(nullptr);
	if (!dc) { return nullptr; }

	HFONT font = CreateFontW(
		-static_cast<int>(p_cache->LineHeight), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
		DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
		CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
		p_cache->FaceName.c_str()); // キャッシュ生成時と同じフェイスでラスタライズする.
	HGDIOBJ old_font = SelectObject(dc, font);

	MAT2 mat = { { 0, 1 }, { 0, 0 }, { 0, 0 }, { 0, 2 } };
	GLYPHMETRICS gm{};
	UINT size = GetGlyphOutlineW(dc, Char, GGO_GRAY8_BITMAP, &gm, 0, nullptr, &mat);

	Glyph glyph{};
	glyph.AdvanceX = static_cast<float>(gm.gmCellIncX);
	glyph.OffsetY  = static_cast<float>(p_cache->Ascent - gm.gmptGlyphOrigin.y);
	glyph.OffsetX  = static_cast<float>(gm.gmptGlyphOrigin.x);
	glyph.Width    = static_cast<float>(gm.gmBlackBoxX);
	glyph.Height   = static_cast<float>(gm.gmBlackBoxY);

	if (size != GDI_ERROR && size > 0 && gm.gmBlackBoxX > 0 && gm.gmBlackBoxY > 0)
	{
		std::vector<BYTE> buffer(size);
		if (GetGlyphOutlineW(dc, Char, GGO_GRAY8_BITMAP, &gm, size, buffer.data(), &mat) != GDI_ERROR)
		{
			// アトラス上の配置位置(shelf packing).
			const UINT w = gm.gmBlackBoxX;
			const UINT h = gm.gmBlackBoxY;
			if (p_cache->CursorX + w + GLYPH_PAD > ATLAS_W)
			{
				p_cache->CursorX = 0;
				p_cache->CursorY += p_cache->Ascent > 0 ? static_cast<UINT>(p_cache->Ascent) : h;
				p_cache->CursorY += GLYPH_PAD;
			}
			if (p_cache->CursorY + h <= ATLAS_H && p_cache->CursorX + w <= ATLAS_W)
			{
				// GDIのグレイ(1..64)を白×アルファへ展開してアトラスへ書き込む.
				std::vector<UINT32> rgba(static_cast<size_t>(w) * h, 0);
				const UINT row_pitch = (w + 3u) & ~3u; // GDIの行はDWORD境界.
				for (UINT y = 0; y < h; ++y)
				{
					for (UINT x = 0; x < w; ++x)
					{
						const BYTE gray = buffer[y * row_pitch + x];
						if (gray == 0) { continue; } // 0=透明.
						const BYTE a = static_cast<BYTE>((gray - 1) * (255.0f / (GRAY_LEVELS - 1.0f)));
						rgba[y * w + x] = 0x00FFFFFFu | (static_cast<UINT32>(a) << 24);
					}
				}

				D3D12_BOX dst_box{ p_cache->CursorX, p_cache->CursorY, 0,
					p_cache->CursorX + w, p_cache->CursorY + h, 1 };
				p_cache->pAtlas->WriteToSubresource(0, &dst_box,
					rgba.data(), w * sizeof(UINT32), static_cast<UINT>(rgba.size() * sizeof(UINT32)));

				glyph.U0 = static_cast<float>(p_cache->CursorX) / ATLAS_W;
				glyph.V0 = static_cast<float>(p_cache->CursorY) / ATLAS_H;
				glyph.U1 = static_cast<float>(p_cache->CursorX + w) / ATLAS_W;
				glyph.V1 = static_cast<float>(p_cache->CursorY + h) / ATLAS_H;

				p_cache->CursorX += w + GLYPH_PAD;
			}
		}
	}

	SelectObject(dc, old_font);
	DeleteObject(font);
	DeleteDC(dc);

	p_cache->Chars.push_back(Char);
	p_cache->Glyphs.push_back(glyph);
	return &p_cache->Glyphs.back();
}

// アトラステクスチャを返す.
ID3D12Resource* FontLoader::GetAtlas(int FontId)
{
	FontCache* p_cache = FindCache(FontId);
	return p_cache ? p_cache->pAtlas : nullptr;
}

// 行の高さを返す.
float FontLoader::GetLineHeight(int FontId)
{
	FontCache* p_cache = FindCache(FontId);
	return p_cache ? p_cache->LineHeight : 0.0f;
}
