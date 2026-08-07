#pragma once

#include <Actor/Actor.h>

// 소코반 게임에서 플레이어를 대표하는 액터 클래스.
class Player:public Craft::Actor
{
	TYPE_DECLARATIONS(Player,Actor)

public:
	Player(const Craft::Vector2& position);

private:
	virtual void Tick(float deltaTime) override;
};

