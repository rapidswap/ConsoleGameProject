#include "Player.h"
#include <Input/Input.h>

using namespace Craft;
Player::Player(const Vector2& position)
	:super("P",position,Color::Green)
{
	// 우선순위 설정.
	// 액터 중에서 가장 높은 값.
	sortingOrder = 10;
}

void Player::Tick(float deltaTime)
{
	// 상위 계층의 Tick 호출.
	//Actor::Tick(deltaTime);
	super::Tick(deltaTime);

	// ESC 종료 처리.
	if (Input::Get().GetKey(VK_ESCAPE))
	{
		// 종료 처리.
		QuitGame();
		return;
	}

	// 이동 처리.
	if (Input::Get().GetKeyDown(VK_RIGHT))
	{
		// 이동하려는 위치 값 만들기.
		Vector2 newPosition = GetPosition();
		newPosition.x += 1;

		// 새로운 위치 설정.
		SetPosition(newPosition);
	}

	if (Input::Get().GetKeyDown(VK_LEFT))
	{
		// 이동하려는 위치 값 만들기.
		Vector2 newPosition = GetPosition();
		newPosition.x -= 1;

		// 새로운 위치 설정.
		SetPosition(newPosition);
	}

	if (Input::Get().GetKeyDown(VK_UP))
	{
		// 이동하려는 위치 값 만들기.
		Vector2 newPosition = GetPosition();
		newPosition.y -= 1;

		// 새로운 위치 설정.
		SetPosition(newPosition);
	}

	if (Input::Get().GetKeyDown(VK_DOWN))
	{
		// 이동하려는 위치 값 만들기.
		Vector2 newPosition = GetPosition();
		newPosition.y += 1;

		// 새로운 위치 설정.
		SetPosition(newPosition);
	}
}
