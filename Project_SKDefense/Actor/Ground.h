#pragma once

#include <Actor/Actor.h>

// 소코반 게임에서 땅(Gruound)을 대표하는 액터 클래스.
class Ground : public Craft::Actor
{
	// 커스텀 RTTI에 타입 설정.
	TYPE_DECLARATIONS(Ground,Actor)

public:
	Ground(const Craft::Vector2& position);
};

