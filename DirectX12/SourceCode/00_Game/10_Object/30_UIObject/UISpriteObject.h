#pragma once

#include <string>

#include <DirectXMath.h>

#include "00_Game/10_Object/30_UIObject/UIObject.h"

/**********************************************************************************
* @brief     : 画像を表示するUI. UIObject階層の具象クラスとして、画像アセット識別子と
*            : UV矩形を保持する(Text等の他UI部品と同じ階層で扱える).
*            : 実描画は将来Renderer側(20_Resource/Image/UISprite接続先)へ委譲する.
**********************************************************************************/

class UISpriteObject final : public UIObject
{
public:
	UISpriteObject() = default;
	~UISpriteObject() override = default;

public: // Getter・Setter.

	// 画像アセット識別子(UISprite/テクスチャローダーへの接続は将来実装).
	void SetImageId(const std::string& ImageId) noexcept { m_ImageId = ImageId; }
	const std::string& GetImageId() const noexcept { return m_ImageId; }

	// UV矩形(left, top, right, bottom). 0〜1のテクスチャ座標系.
	void SetUVRect(const DirectX::XMFLOAT4& UVRect) noexcept { m_UVRect = UVRect; }
	const DirectX::XMFLOAT4& GetUVRect() const noexcept { return m_UVRect; }

private:
	std::string       m_ImageId;                              // 画像アセット識別子.
	DirectX::XMFLOAT4 m_UVRect { 0.0f, 0.0f, 1.0f, 1.0f };    // UV矩形(left, top, right, bottom).
};
