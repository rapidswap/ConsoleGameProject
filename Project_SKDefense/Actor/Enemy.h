#pragma once

#include <Actor/Actor.h>

class Enemy : public Craft::Actor
{
	TYPE_DECLARATIONS(Enemy, Craft::Actor)

public:
	Enemy(const Craft::Vector2& position);

	virtual void Tick(float deltaTime) override;
	virtual void Draw() override;
};
