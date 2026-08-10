#include "Player.h"
#include <Engine/Engine.h>
#include <Input/Input.h>
#include <Level/Level.h>
#include <Actor/PlayerBullet.h>

using namespace Craft;
Player::Player()
	:Actor("<=A=>",Vector2::Zero,Color::Green),
	fireMode(FireMode::OneShot)
{
	// 생성 위치 설정.
	int x = (Engine::Get().GetWidth() / 2) - (width / 2);
	int y = (Engine::Get().GetHeight() - 2);

	SetPosition(Vector2(x, y));

	// x위치 저장.
	xPosition = static_cast<float>(x);

	// 연사 타이머 시간 설정.
	timer.SetTargetTime(fireInterval);

}


void Player::Tick(float deltaTime)
{
	super::Tick(deltaTime);

	// ESC 키 종료 처리.
	if (Input::Get().GetKeyDown(VK_ESCAPE))
	{
		QuitGame();
	}

	// 방향키 입력에 따른 이동 방향 설정.
	// 오른쪽 방향:1 | 왼쪽 방향: -1
	float direction = 0.0f;
	if (Input::Get().GetKey(VK_RIGHT))
	{
		direction = 1.0f;
	}
	if (Input::Get().GetKey(VK_LEFT))
	{
		direction = -1.0f;
	}

	// 이동 함수 호출.
	Move(direction, deltaTime);

	// 발사 타이머 업데이트.
	timer.Tick(deltaTime);

	if (fireMode == FireMode::OneShot) 
	{
		// 탄약 발사 처리.
		if (Input::Get().GetKeyDown(VK_SPACE))
		{
			Fire();
		}
	}
	else if(fireMode==FireMode::Repeat)
	{
		if (Input::Get().GetKey(VK_SPACE))
		{
			// 연사 발사 함수 호출.
			FireInterval();
		}
	}

	// 발사 모드 전환 처리.
	if (Input::Get().GetKeyDown('R'))
	{
		if (fireMode == FireMode::OneShot)
		{
			fireMode = FireMode::Repeat;
		}
		else if (fireMode == FireMode::Repeat)
		{
			fireMode = FireMode::OneShot;
		}
	}
	
}

	
void Player::Move(float direction, float deltaTime)
{
	// x위치 업데이트.
	// 이동 처리 -> 이동 방향과 빠르기를 적용해서 새로운 위치를 구하는 것.
	// 이동 방향(direction) / 빠르기(moveSpeed) /  시간.
	// 동속도 운동: 이동 거리 = 기존의 위치 + 이동 방향 x 빠르기 x 시간.
	xPosition += direction * moveSpeed * deltaTime;

	// 화면 왼쪽 벗어나지 않도록 처리
	if (xPosition < 0)
	{
		xPosition = 0.0f;
	}

	// 화면 오른쪽 벗어나지 않도록 처리.
	if (xPosition + width >= Engine::Get().GetWidth())
	{
		xPosition = static_cast<float>(Engine::Get().GetWidth() - width);
	}

	// 위치 업데이트.
	Vector2 newPosition = GetPosition();
	// float 값을 int로 형변환할 때 소숫점 값은 버림 처리된다는 점 주의
	newPosition.x = static_cast<int>(xPosition);
	SetPosition(newPosition);
}

void Player::Fire()
{
	// 탄약 생성 위치 구하기.
	// 플레이어의 가운데 위치.

	Vector2 bulletPosition(
		(GetPosition().x + (width / 2)),
		GetPosition().y);

	// 탄약 생성.
	std::shared_ptr<Level> owner = GetOwner();
	if (owner)
	{
		owner->SpawnActor<PlayerBullet>(bulletPosition);
	}
}

void Player::FireInterval()
{
	// 발사 가능 여부 확인.
	if (!CanShoot())
	{
		return;
	}

	// 발사 처리.
	Fire();

	// 경과 시간 초기화.
	timer.Reset();
	 
}
