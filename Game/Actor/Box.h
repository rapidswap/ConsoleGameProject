#pragma once

#include <Actor/Actor.h>

// 소코반 게임에서 박스를 대표하는 액터 클래스.
class Box:public Craft::Actor
{
	TYPE_DECLARATIONS(Box,Actor)

public:
	Box(const Craft::Vector2& position);

};

