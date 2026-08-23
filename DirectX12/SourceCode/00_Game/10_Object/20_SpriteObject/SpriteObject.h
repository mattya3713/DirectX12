#pragma once

#include <string>

#include <DirectXMath.h>

#include "00_Game/10_Object/00_Base/GameObject.h"

/**********************************************************************************
* @brief     : ワールド空間に存在する2D画像(スプライト)のゲームオブジェクト基底クラス.
*            : 値の保持とUpdate/Drawフックのみを担当し、描画は将来Renderer側
*            : (20_Resource/Image/WorldSprite接続先)へ委譲する。
*            : ビルボード等の向き制御もモード保持のみで、解釈はRenderer側が行う。
*            : MeshObjectとは継承関係を持たずGameObjectから直接派生する。
**********************************************************************************/

class SpriteObject : public GameObject
{
public:
	// スプライトの向き制御種別(解釈はRenderer側).
	enum class eBillboard : std::uint8_t
	{
		None,          // 向き固定(Transformの回転に従う).
		ScreenAligned, // 常にカメラ正面を向く.
		YAxisLocked,   // Y軸回転のみカメラへ追従(縦方向は傾けない).
	};

	SpriteObject() = default;
	~SpriteObject() override = default;

	// GameObjectの更新処理を流用する(将来スプライト固有更新を追加する場所).
	void Update() override;

	// 描画フック(Renderer未接続のため現在は何もしない).
	void Draw() override;

public: // Getter・Setter.

	// 画像アセット識別子(WorldSprite/テクスチャローダーへの接続は将来実装).
	void SetImageId(const std::string& ImageId) noexcept { m_ImageId = ImageId; }
	const std::string& GetImageId() const noexcept { return m_ImageId; }

	// スプライトのワールド単位サイズ(幅・高さ).
	void SetSize(const DirectX::XMFLOAT2& Size) noexcept { m_Size = Size; }
	const DirectX::XMFLOAT2& GetSize() const noexcept { return m_Size; }

	// スプライト色(RGBA). UV参照時に乗算される前提.
	void SetColor(const DirectX::XMFLOAT4& Color) noexcept { m_Color = Color; }
	const DirectX::XMFLOAT4& GetColor() const noexcept { return m_Color; }

	// UV矩形(left, top, right, bottom). 0〜1のテクスチャ座標系.
	void SetUVRect(const DirectX::XMFLOAT4& UVRect) noexcept { m_UVRect = UVRect; }
	const DirectX::XMFLOAT4& GetUVRect() const noexcept { return m_UVRect; }

	// 表示状態.
	void SetVisible(bool IsVisible) noexcept { m_IsVisible = IsVisible; }
	bool IsVisible() const noexcept { return m_IsVisible; }

	// ビルボード種別.
	void SetBillboard(eBillboard Billboard) noexcept { m_Billboard = Billboard; }
	eBillboard GetBillboard() const noexcept { return m_Billboard; }

private:
	std::string           m_ImageId;                        // 画像アセット識別子.
	DirectX::XMFLOAT2     m_Size      { 1.0f,   1.0f   };   // 幅・高さ(ワールド単位).
	DirectX::XMFLOAT4     m_Color     { 1.0f, 1.0f, 1.0f, 1.0f }; // スプライト色(RGBA).
	DirectX::XMFLOAT4     m_UVRect    { 0.0f, 0.0f, 1.0f, 1.0f }; // UV矩形(left, top, right, bottom).
	bool                  m_IsVisible = true;                 // 表示状態.
	eBillboard            m_Billboard = eBillboard::None;     // 向き制御種別.
};
