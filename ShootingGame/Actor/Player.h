#pragma once

#include <Actor/Actor.h>


// 좌우로 이동/스페이스 키로 탄약을 발사하는 플레이어.
class Player:public Craft::Actor
{
	// 커스텀 타입 등록.
	TYPE_DECLARATIONS(Player,Actor)

public:
	Player();
	
private:
	// 이벤트 함수 오버라이드.
	virtual void Tick(float deltaTime) override;

	// 이동 처리 함수.
	void Move(float direction, float deltaTime);

	// 탄약 발사 함수.
	void Fire();

private:
	// 이동 처리에 필요한 변수.
	float xPosition = 0.0f;

	// 이동 속도 변수.
	float moveSpeed = 70.f;
};

