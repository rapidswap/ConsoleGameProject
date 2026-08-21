#include "EnemyBullet.h"
#include <Engine/Engine.h>
#include <Level/Level.h>

using namespace Craft;
EnemyBullet::EnemyBullet(const Vector2& position, float dirX, float dirY, float moveSpeed)
	: Actor("#", position, Color::Red),
	moveSpeed(moveSpeed),
	xPosition(static_cast<float>(position.x)),
	yPosition(static_cast<float>(position.y)),
	directionX(dirX),
	directionY(dirY)
{
}

void EnemyBullet::Tick(float deltaTime)
{
	super::Tick(deltaTime);

	// 방향과 속도에 따른 위치 업데이트.
	xPosition += directionX * moveSpeed * deltaTime;
	yPosition += directionY * moveSpeed * deltaTime;

	// 좌표 검사 (화면 밖으로 나가면 파괴).
	int screenWidth = Engine::Get().GetWidth();
	int screenHeight = Engine::Get().GetHeight();
	
	Craft::Vector2 camPos = Craft::Vector2::Zero;
	if (GetOwner()) camPos = GetOwner()->GetCameraPosition();
	float minX = static_cast<float>(camPos.x - screenWidth / 2);
	float maxX = static_cast<float>(camPos.x + screenWidth / 2 - 1);
	float minY = static_cast<float>(camPos.y - screenHeight / 2);
	float maxY = static_cast<float>(camPos.y + screenHeight / 2 - 1);

	if (yPosition >= maxY || yPosition < minY || xPosition >= maxX || xPosition < minX)
	{
		Destroy();
		return;
	}

	// 위치 설정.
	SetPosition(Vector2(
		static_cast<int>(xPosition), static_cast<int>(yPosition)
	));
}