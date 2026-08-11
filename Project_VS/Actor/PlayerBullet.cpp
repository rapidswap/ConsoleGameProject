#include "PlayerBullet.h"
#include <Engine/Engine.h>

using namespace Craft;
PlayerBullet::PlayerBullet(const Craft::Vector2& position, float dirX, float dirY)
	:Actor("@",position,Color::Blue),
	xPosition(static_cast<float>(position.x)), 
	yPosition(static_cast<float>(position.y)), 
	directionX(dirX), directionY(dirY)
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
	if (xPosition < 0.0f || xPosition >= Engine::Get().GetWidth() ||
		yPosition < 0.0f || yPosition >= Engine::Get().GetHeight())
	{
		// 삭제 처리.
		Destroy();
		return;
	}

	// 위치 값 설정 및 갱신.
	Vector2 newPosition = GetPosition();
	newPosition.x = static_cast<int>(xPosition);
	newPosition.y = static_cast<int>(yPosition);
	SetPosition(newPosition);
}
