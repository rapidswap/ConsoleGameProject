#include "Actor.h"
#include <Engine/Engine.h>
#include <Render/Renderer.h>
#include <Level/Level.h>

namespace Craft
{
	Actor::Actor(
		const std::string& image,
		const Vector2& position,
		Color color)
		:image(image), position(position), color(color),
		width(static_cast<int>(image.length()))
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
		// 비활성 상태이면 종료.
		if (!IsActive())
		{
			return;
		}
		// 기본 렌더링 위치는 월드 좌표.
		Vector2 screenPos = position;
		
		auto owner = GetOwner();
		if (owner)
		{
			Vector2 camPos = owner->GetCameraPosition();
			int screenWidth = Engine::Get().GetWidth();
			int screenHeight = Engine::Get().GetHeight();

			screenPos.x = position.x - camPos.x + (screenWidth / 2);
			screenPos.y = position.y - camPos.y + (screenHeight / 2);
		}

		// 환산된 가짜(화면) 좌표를 렌더러에 제출.
		Renderer::Get().Submit(image, screenPos, color, sortingOrder);
	}

	void Actor::OnCollision(const std::shared_ptr<Actor>& other)
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

	void Actor::SetPosition(const Vector2& newPosition)
	{
		// 변경하려는 위치 값이 기존 값과 동일하면 종료.
		if (position == newPosition)
		{
			return;
		}

		position = newPosition;
	}
}