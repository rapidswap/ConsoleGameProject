#pragma once

#include <Actor/Actor.h>
#include <Util/Timer.h>

class DestroyMagnet:public Craft::Actor
{
	// 커스텀 RTTI 등록.
	TYPE_DECLARATIONS(DestroyMagnet, Actor)

public:
	DestroyMagnet(const Craft::Vector2& position);
	~DestroyMagnet() = default;
	virtual void Tick(float deltaTime) override;

private:
	Timer lifeTimer;
};
