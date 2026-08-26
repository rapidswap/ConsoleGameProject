#include "TurretBullet.h"
#include <Engine/Engine.h>
#include <Level/Level.h>
#include <Actor/Turret.h>
#include <cmath>

using namespace Craft;
TurretBullet::TurretBullet(const Craft::Vector2& position, float dirX, float dirY)
	:Actor("-", position, Color::Green),
	xPosition(static_cast<float>(position.x)), yPosition(static_cast<float>(position.y)),
	directionX(dirX), directionY(dirY),
	startPosition(position)
{

}

void TurretBullet::Tick(float deltaTime)
{
	// 상위 틱 로직 호출.
	super::Tick(deltaTime);

	// 지정된 방향으로 이동 처리.
	xPosition += directionX * moveSpeed * deltaTime;
	yPosition += directionY * moveSpeed * deltaTime;

	// 실수 좌표를 정수 좌표(Vector2)로 갱신
	SetPosition(Craft::Vector2(static_cast<int>(xPosition), static_cast<int>(yPosition)));

	// 발사된 위치로부터의 거리 계산
	float dx = xPosition - startPosition.x;
	float dy = yPosition - startPosition.y;
	float distance = std::sqrt(dx * dx + dy * dy);

	// 사거리를 벗어나면 총알 파괴
	if (distance >= bulletRange)
	{
		Destroy();
	}
}

