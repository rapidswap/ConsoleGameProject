#pragma once

#include <Actor/Actor.h>
#include <Util/Timer.h>

class Enemy : public Craft::Actor
{
	// 커스텀 타입설정.
	TYPE_DECLARATIONS(Enemy,Actor)

	// 이동 방향 열거형.
	enum class MoveDirection
	{
		None=1,
		Left,
		Right
	};

public:
	Enemy(const std::string& image = "(oOo)", int yPosition = 5);

private:
	// 이벤트 함수 오버라이드.
	virtual void Tick(float deltaTime) override;

private:
	// 이동 방향 열거형 변수.
	MoveDirection direction = MoveDirection::None;

	// 좌우 이동 처리를 위한 변수.
	float xPosition = 0.0f;
	float moveSpeed = 5.0f;

	// 발사 타이머.
	Timer timer;
	 
};

