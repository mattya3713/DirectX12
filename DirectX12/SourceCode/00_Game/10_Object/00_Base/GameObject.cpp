#include "GameObject.h"

GameObject::GameObject()
	: m_Transform {}
{
}

GameObject::~GameObject()
{
}

void GameObject::Update()
{
	// 既定では何もしない. 派生クラスでオーバーライドする.
}

void GameObject::Draw()
{
	// 既定では何もしない. 派生クラスでオーバーライドする.
}
