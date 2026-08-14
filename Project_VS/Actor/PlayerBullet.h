#pragma once

#include <Actor/Actor.h>

// 플레이어가 발사하는 탄약 클래스.
class PlayerBullet:public Craft::Actor
{
	// 커스텀 타입 설정.
	TYPE_DECLARATIONS(PlayerBullet, Actor)

public:
	// 벽 반사 여부를 결정하는 isBouncing 파라미터 및 파편(분열탄) 여부 isShrapnel 추가
	PlayerBullet(const Craft::Vector2& position, float dirX, float dirY, bool isBouncing = false, bool isShrapnel = false);

	// 파편 총알인지 확인하는 Getter
	inline bool IsShrapnel() const { return bIsShrapnel; }

private:
	// 이벤트 함수 오버라이딩.
	virtual void Tick(float deltaTime)override;

private:
	// 이동 속도(빠르기 - 단위: 초).
	float moveSpeed = 30.0f;

	// 위치 갱신을 할 때 사용할 변수.
	float xPosition = 0.0f;
	float yPosition = 0.0f;

	// 이동 방향.
	float directionX = 0.0f;
	float directionY = 0.0f;
	
	// 벽 반사(Bouncing) 기능 활성화 여부.
	bool canBounce = false;
	// 벽에 무한정 튕겨서 남아있는 것을 방지 (최대 3회 튕김).
	int bounceLimit = 3; 

	// 파편(Shrapnel) 여부: 적이 죽어서 터져나온 총알인지 여부.
	bool bIsShrapnel = false;
};

