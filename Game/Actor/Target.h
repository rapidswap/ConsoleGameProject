#pragma once

#include <Actor/Actor.h>

// 소코반 게임에서 이동 목표 지점(타겟)을 대표하는 액터 클래스
class Target:public Craft::Actor
{
	TYPE_DECLARATIONS(Target,Actor)

public:
	Target(const Craft::Vector2& position);
};

