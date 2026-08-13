#pragma once

#include <string>

struct Transform;

class IMesh
{
public:
	virtual ~IMesh() = default;

	virtual void Update() = 0;
	virtual void Draw() = 0;
	virtual void SetWorldTransform(const Transform& InTransform) = 0;
	virtual void PlayNamedClip(const std::string& ClipName) = 0;
};
