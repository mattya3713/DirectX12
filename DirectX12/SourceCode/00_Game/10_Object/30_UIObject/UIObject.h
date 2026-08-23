#pragma once

#include <DirectXMath.h>

/**********************************************************************************
* @brief     : 画面空間UIの基底クラス. ワールドTransformに依存せず、画面座標系の
*            : 値(位置・サイズ・回転・レイヤー・色・表示状態)のみを保持する.
*            : 派生クラス(UISpriteObject/将来のTextやUI部品)を同じUI階層で
*            : Update/Drawできることを目的とし、親子関係や所属管理は将来拡張に残す.
*            : 実描画は将来Renderer側(20_Resource/Image/UISprite接続先)へ委譲する.
**********************************************************************************/

class UIObject
{
public:
	UIObject() = default;
	virtual ~UIObject();

	UIObject(const UIObject&)            = delete;
	UIObject& operator=(const UIObject&) = delete;

	// 毎フレーム更新(既定は何もしない. 派生クラスで上書きする).
	virtual void Update();

	// 描画フック(Renderer未接続のため現在は何もしない).
	virtual void Draw();

public: // Getter・Setter.

	// 画面座標(ピクセル. 左上原点を想定).
	void SetPosition(const DirectX::XMFLOAT2& Position) noexcept { m_Position = Position; }
	const DirectX::XMFLOAT2& GetPosition() const noexcept { return m_Position; }

	// UIサイズ(ピクセル).
	void SetSize(const DirectX::XMFLOAT2& Size) noexcept { m_Size = Size; }
	const DirectX::XMFLOAT2& GetSize() const noexcept { return m_Size; }

	// 回転角(度. 自身の中心周りを想定).
	void SetRotationDeg(float RotationDeg) noexcept { m_RotationDeg = RotationDeg; }
	float GetRotationDeg() const noexcept { return m_RotationDeg; }

	// レイヤー深度(大きいほど前面に描画する運びとする).
	void SetLayer(int Layer) noexcept { m_Layer = Layer; }
	int GetLayer() const noexcept { return m_Layer; }

	// UI色(RGBA).
	void SetColor(const DirectX::XMFLOAT4& Color) noexcept { m_Color = Color; }
	const DirectX::XMFLOAT4& GetColor() const noexcept { return m_Color; }

	// 表示状態.
	void SetVisible(bool IsVisible) noexcept { m_IsVisible = IsVisible; }
	bool IsVisible() const noexcept { return m_IsVisible; }

private:
	DirectX::XMFLOAT2 m_Position  { 0.0f, 0.0f };               // 画面座標(ピクセル).
	DirectX::XMFLOAT2 m_Size      { 32.0f, 32.0f };             // サイズ(ピクセル).
	float             m_RotationDeg = 0.0f;                     // 回転角(度).
	int               m_Layer     = 0;                          // レイヤー深度.
	DirectX::XMFLOAT4 m_Color     { 1.0f, 1.0f, 1.0f, 1.0f };   // UI色(RGBA).
	bool              m_IsVisible = true;                       // 表示状態.
};
