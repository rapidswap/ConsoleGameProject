#pragma once

#include <Actor/Actor.h>
class Agit:public Craft::Actor
{
	TYPE_DECLARATIONS(Agit,Actor)

public:
	Agit(const Craft::Vector2& position);
	
	// 아지트의 체력 관리 함수.
	inline void AgitHealthDown() { --agitHealth; }

	// Getter.
	inline int GetHealth() { return agitHealth; }

private:
	// 아지트 현재 체력.
	int agitHealth = 100;

};



