#pragma once
#include <Actor/Actor.h>

class Turret : public Craft::Actor
{
	TYPE_DECLARATIONS(Turret, Craft::Actor)
public:
	Turret(const Craft::Vector2& position);
	virtual ~Turret() = default;

	virtual void Draw() override;
};
