#include "Turret.h"
#include <Render/Renderer.h>
#include <Engine/Engine.h>
#include <Level/Level.h>

Turret::Turret(const Craft::Vector2& position)
	: Craft::Actor("TT", position, Craft::Color::Yellow)
{
	// 터렛은 다른 물체 위에 그려지도록 우선순위 상향
	sortingOrder = 10;
}

void Turret::Draw()
{
	if (!IsActive()) return;
	
	Craft::Vector2 screenPos = position;
	auto owner = GetOwner();
	if (owner)
	{
		Craft::Vector2 camPos = owner->GetCameraPosition();
		int screenWidth = Craft::Engine::Get().GetWidth();
		int screenHeight = Craft::Engine::Get().GetHeight();
		screenPos.x = position.x - camPos.x + (screenWidth / 2);
		screenPos.y = position.y - camPos.y + (screenHeight / 2);
	}

	// 2x2 크기 렌더링
	Craft::Renderer::Get().Submit("TT", screenPos, color, sortingOrder);
	
	screenPos.y += 1;
	Craft::Renderer::Get().Submit("TT", screenPos, color, sortingOrder);
}
