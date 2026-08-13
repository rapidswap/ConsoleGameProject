#pragma once

#include <Actor/Actor.h>
#include <Util/Timer.h>

class Demon:public Craft::Actor
{
	TYPE_DECLARATIONS(Demon,Actor)

public:
	Demon(const Craft::Vector2& position);

	inline void hpDown() { demonHp--; }

	inline void DemonHurt() { 
		if (!isDemonHurt) {
			isDemonHurt = true;
			HealDemon();
		}
		}

	// Getter.
	inline int GetHp() const { return demonHp; }
	inline int GetMaxHp() const { return maxDemonHp; }

	// 스탯 설정용 Setter
	inline void SetMaxHp(int hp) { demonHp = hp; maxDemonHp = hp; }
	inline void SetMoveSpeed(float speed) { moveSpeed = speed; }

private:
	virtual void Tick(float deltaTime) override;
	virtual void OnCollision(const std::shared_ptr<Actor>& other) override;

	// 데몬의 체력 회복 패턴.
	inline void HealDemon() { demonHp += maxDemonHp / 2; }

	

private:
	// 데몬 체력.
	int demonHp = 100;
	int maxDemonHp = 100;

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

	// 발악 패턴 확인용.
	bool isDemonHurt = false;

};

