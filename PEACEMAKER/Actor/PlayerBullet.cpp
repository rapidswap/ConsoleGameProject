#include "PlayerBullet.h"
#include <Engine/Engine.h>
#include <Level/Level.h>

using namespace Craft;
PlayerBullet::PlayerBullet(const Craft::Vector2& position, float dirX, float dirY, bool isBouncing, bool isShrapnel)
	:Actor("@",position,Color::Blue),
	xPosition(static_cast<float>(position.x)), 
	yPosition(static_cast<float>(position.y)), 
	directionX(dirX), directionY(dirY),
	canBounce(isBouncing),
	bIsShrapnel(isShrapnel)
{

}

void PlayerBullet::Tick(float deltaTime)
{
	// 상위 틱 로직 호출.
	super::Tick(deltaTime);

	// 지정된 방향으로 이동 처리.
	xPosition += directionX * moveSpeed * deltaTime;
	yPosition += directionY * moveSpeed * deltaTime;

	// 좌표 검사(화면 상하좌우를 벗어났는지 확인).
	int screenWidth = Engine::Get().GetWidth();
	int screenHeight = Engine::Get().GetHeight();

	Craft::Vector2 camPos = Craft::Vector2::Zero;
	if (GetOwner()) camPos = GetOwner()->GetCameraPosition();
	float minX = static_cast<float>(camPos.x - screenWidth / 2);
	float maxX = static_cast<float>(camPos.x + screenWidth / 2 - 1);
	float minY = static_cast<float>(camPos.y - screenHeight / 2);
	float maxY = static_cast<float>(camPos.y + screenHeight / 2 - 1);

	if (xPosition < minX || xPosition >= maxX ||
		yPosition < minY || yPosition >= maxY)
	{
		if (canBounce && bounceLimit > 0)
		{
			// 벽에 부딪혔을 때 방향 반전.
			if (xPosition < minX || xPosition >= maxX)
			{
				directionX *= -1.0f;
				// 뚫고 나가지 않게 좌표 보정.
				xPosition = (xPosition < minX) ? minX : maxX;
			}
			
			if (yPosition < minY || yPosition >= maxY)
			{
				directionY *= -1.0f;
				// 뚫고 나가지 않게 좌표 보정.
				yPosition = (yPosition < minY) ? minY : maxY;
			}
			bounceLimit--;
		}
		else
		{
			// 반사가 불가능하거나 횟수를 다 쓰면 삭제 처리.
			Destroy();
			return;
		}
	}

	// 위치 값 설정 및 갱신.
	Vector2 newPosition = GetPosition();
	newPosition.x = static_cast<int>(xPosition);
	newPosition.y = static_cast<int>(yPosition);
	SetPosition(newPosition);
}
