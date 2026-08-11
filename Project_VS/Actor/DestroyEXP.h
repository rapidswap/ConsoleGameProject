#pragma once

#include <Actor/Actor.h>
#include <Util/Util.h>

class DestroyEXP:public Craft::Actor
{
	// 커스텀 RTTI 등록.
	TYPE_DECLARATIONS(DestroyEXP,Actor)

public:
	DestroyEXP(const Craft::Vector2& position);
	~DestroyEXP()=default;
};

