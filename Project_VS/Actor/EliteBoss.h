#pragma once
#include <Actor/Actor.h>
#include <Util/Timer.h>

class EliteBoss:public Craft::Actor
{
	TYPE_DECLARATIONS(EliteBoss, Actor)

public:
	EliteBoss(const Craft::Vector2& position);
	~EliteBoss() = default;

private:
	virtual void Tick(float deltaTime)override;
	virtual void OnCollision(const std::shared_ptr<Actor>& other) override;


private:
	// 엘리트 보스의 체력.
	int eliteBossHp = 20;
	
	float xPosition = 0.0f;
	float yPosition = 0.0f;
	float moveSpeed = 3.0f;

	// 타이머.
	Timer timer;


};

