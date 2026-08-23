#include "UIObject.h"

UIObject::~UIObject() = default;

// 既定では何もしない(派生クラスが上書きする).
void UIObject::Update()
{
}

// Renderer未接続のため現在は何もしない.
void UIObject::Draw()
{
}
