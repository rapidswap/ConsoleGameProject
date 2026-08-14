#pragma once

#include <Actor/Actor.h>

class DestroyAugmentPoint:public Craft::Actor
{
	// 커스텀 RTTI 등록.
	TYPE_DECLARATIONS(DestroyAugmentPoint, Actor)

public:
	DestroyAugmentPoint(const Craft::Vector2& position);
	~DestroyAugmentPoint() = default;
};

