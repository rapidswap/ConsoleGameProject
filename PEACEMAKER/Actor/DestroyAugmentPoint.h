#pragma once

#include <Actor/Actor.h>
#include <Util/Timer.h>

class DestroyAugmentPoint:public Craft::Actor
{
	// 커스텀 RTTI 등록.
	TYPE_DECLARATIONS(DestroyAugmentPoint, Actor)

public:
	DestroyAugmentPoint(const Craft::Vector2& position);
	~DestroyAugmentPoint() = default;
	virtual void Tick(float deltaTime) override;

private:
	Timer lifeTimer;
};

