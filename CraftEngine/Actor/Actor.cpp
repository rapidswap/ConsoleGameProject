#include "Actor.h"
#include <Engine/Engine.h>

namespace Craft
{
	Actor::Actor()
	{
	}

	Actor::~Actor()
	{
	}

	void Actor::BeginPlay()
	{
		// 이벤트 처리했다고 설정.
		hasBeganPlay = true;
	}

	void Actor::Tick(float deltaTime)
	{
	}

	void Actor::Draw()
	{
	}

	void Actor::Destroy()
	{
		// 삭제 예약 설정.
		hasExpired = true;
	}

	void Actor::QuitGame()
	{
		// 엔진 종료 요청.
		Engine::Get().Quit();
	}
}