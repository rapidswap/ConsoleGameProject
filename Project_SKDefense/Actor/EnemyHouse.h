#pragma once
#include <Actor/Actor.h>

class EnemyHouse:public Craft::Actor
{
	TYPE_DECLARATIONS(EnemyHouse,Actor)

public:
	EnemyHouse(const Craft::Vector2& position);
};

