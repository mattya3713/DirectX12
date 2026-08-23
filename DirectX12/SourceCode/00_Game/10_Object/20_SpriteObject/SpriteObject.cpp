#include "SpriteObject.h"

// GameObject側のTransform更新を流用する(スプライト固有の更新は将来追加).
void SpriteObject::Update()
{
	GameObject::Update();
}

// Renderer未接続のため現在は何もしない.
void SpriteObject::Draw()
{
}
