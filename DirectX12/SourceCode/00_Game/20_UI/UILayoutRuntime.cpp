#include "UILayoutRuntime.h"

#include "10_Ggraphic/10_Device/DirectX/DirectX12.h"
#include "10_Ggraphic/20_Render/Sprite/SpriteRenderer.h"
#include "99_Utility/ServiceLocator/ServiceLocator.h"

namespace {
	// テクスチャ未指定時に使うフォールバック画像(既存アセットを流用).
	constexpr const char* kFallbackImagePath = "Data\\Image\\toon01.bmp";
}

// 描画する(レイヤー昇順. 解像度は実値から毎フレーム計算してアンカー位置が破綻しないようにする).
void UILayoutRuntime::Draw(SpriteRenderer& Renderer)
{
	DirectX12* p_dx12 = ServiceLocator::Get<DirectX12>();
	if (!p_dx12) { return; }

	const float screen_w = static_cast<float>(p_dx12->GetBackBufferWidth());
	const float screen_h = static_cast<float>(p_dx12->GetBackBufferHeight());

	// テクスチャ解決結果を要素ごとにキャッシュ(同じImageIdの再解決を避ける).
	std::map<std::string, ID3D12Resource*> resolved;

	for (const std::string& id : m_Model.GetSortedIdsByLayer())
	{
		const UIElementDesc* p_element = m_Model.Find(id);
		if (!p_element || !p_element->IsVisible || p_element->Type != "Sprite") { continue; }

		auto texture_it = resolved.find(p_element->ImageId);
		if (texture_it == resolved.end())
		{
			const std::string path = p_element->ImageId.empty() ? kFallbackImagePath : p_element->ImageId;
			MyComPtr<ID3D12Resource> texture = p_dx12->GetTextureByPath(path.c_str());
			texture_it = resolved.emplace(p_element->ImageId, texture.Detach()).first;
		}

		ID3D12Resource* p_texture = texture_it->second;
		if (!p_texture) { continue; } // 画像欠損時はその要素だけ描画をスキップする.

		float pos_x = 0.0f, pos_y = 0.0f;
		UILayoutModel::GetPosition(*p_element, screen_w, screen_h, pos_x, pos_y);
		Renderer.DrawSprite2D(p_texture, pos_x, pos_y,
			p_element->Width, p_element->Height,
			DirectX::XMFLOAT4{ p_element->ColorR, p_element->ColorG, p_element->ColorB, p_element->ColorA });
	}
}
