#pragma once

#include <Actor/Actor.h>

class Enemy : public Craft::Actor
{
	// 커스텀 타입설정.
	TYPE_DECLARATIONS(Enemy,Actor)

public:
	Enemy(const std::string& image = "(oOo)", float x = 0.0f, float y = 5.0f);

	// 스탯 설정용 Setter
	inline void SetMoveSpeed(float speed) { moveSpeed = speed; }

private:
	// 이벤트 함수 오버라이드.
	virtual void Tick(float deltaTime) override;

	// 충돌 처리 함수 오버라이드.
	virtual void OnCollision(const std::shared_ptr<Actor>& other) override;

private:
private:
	// 위치 이동 처리를 위한 변수.
	float xPosition = 0.0f;
	float yPosition = 0.0f;
	float moveSpeed = 5.0f;
};

