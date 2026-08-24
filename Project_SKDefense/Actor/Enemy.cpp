#include "Enemy.h"
#include <Render/Renderer.h>
#include <Engine/Engine.h>
#include <Level/Level.h>

Enemy::Enemy(const Craft::Vector2& position)
	: Craft::Actor("Z", position, Craft::Color::Red)
{
	sortingOrder = 15;
}

void Enemy::Tick(float deltaTime)
{
	Craft::Actor::Tick(deltaTime);
	
	// TODO: A* 알고리즘을 사용해서 한 칸씩 이동하는 로직 추가
}

void Enemy::Draw()
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

	Craft::Renderer::Get().Submit(image, screenPos, color, sortingOrder);
}

void Enemy::OnCollision(const std::shared_ptr<Craft::Actor>& other)
{
	Craft::Actor::OnCollision(other);
}
