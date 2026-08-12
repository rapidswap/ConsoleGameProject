#pragma once

#include <Actor/Actor.h>
#include <Util/Timer.h>

class Demon:public Craft::Actor
{
	TYPE_DECLARATIONS(Demon,Actor)

public:
	Demon(const Craft::Vector2& position);


private:
	virtual void Tick(float deltaTime) override;
	virtual void OnCollision(const std::shared_ptr<Actor>& other) override;

	// 데몬의 체력 회복 패턴.
	inline void HealDemon() { demonHp += 50; }

private:
	// 데몬 체력.
	int demonHp = 100;

	// 보스의 위치, 이동 속도.
	float xPosition = 0.0f;
	float yPosition = 0.0f;
	float moveSpeed = 10.0f;

	// 발사 타이머.
	Timer timer;
	Timer fireTimer;

	// 공격 패턴 관리를 위한 변수
	int currentPattern = 0;
	float currentAngle = 0.0f;
	int patternStep = 0;

};

