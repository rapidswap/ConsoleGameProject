#pragma once
#include <Actor/Actor.h>
#include <Util/Timer.h>
class Turret : public Craft::Actor
{
	TYPE_DECLARATIONS(Turret, Craft::Actor)
public:
	Turret(const Craft::Vector2& position);
	virtual ~Turret() = default;

	virtual void Draw() override;

	virtual void Tick(float deltaTime) override;

private:
	// 자동 공격 타이머.
	Timer autoFireInterval;

	// 터렛의 공격 속도 (초 단위).
	float atkSpeed = 3.0f;
	
	// 터렛의 인지/공격 사거리.
	float atkRange = 10.0f;
};
