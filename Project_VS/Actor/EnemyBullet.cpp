#include "EnemyBullet.h"
#include <Engine/Engine.h>

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

	if (yPosition >= screenHeight - 1 || yPosition < 0 || xPosition >= screenWidth || xPosition < 0)
	{
		Destroy();
		return;
	}

	// 위치 설정.
	SetPosition(Vector2(
		static_cast<int>(xPosition), static_cast<int>(yPosition)
	));
}