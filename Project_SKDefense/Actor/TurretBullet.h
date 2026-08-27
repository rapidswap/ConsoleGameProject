#pragma once

#include <Actor/Actor.h>

class TurretBullet: public Craft::Actor
{
	// 커스텀 타입 설정.
	TYPE_DECLARATIONS(TurretBullet,Actor)

public:
	TurretBullet(const Craft::Vector2& position, float dirX, float dirY);
private:
	// 이벤트 함수 오버라이딩.
	virtual void Tick(float deltaTime) override;

private:

	// 이동 속도 (빠르기 - 단위:초).
	float moveSpeed = 30.0f;

	// 이동 방향.
	float directionX = 0.0f;
	float directionY = 0.0f;

	// 현재 위치.
	float xPosition = 0.0f;
	float yPosition = 0.0f;

	// 총알이 발사된 시작 위치 (사거리 계산용).
	Craft::Vector2 startPosition;

	// 총알 데미지.
	float bulletDamage=1.0f;

	// 공격 사거리.
	float bulletRange = 5.0f;
		
	// 총알 범위 설정.
	inline void SetBulletRange(float range) { bulletRange = range; }
};

