#include "Enemy.h"
#include <Util/Util.h>
#include <Engine/Engine.h>
#include <Level/Level.h>
#include "Player.h"
#include <cmath>
#include <Actor/PlayerBullet.h>
#include <Actor/DestroyEffect.h>
#include <Actor/DestroyEXP.h>

using namespace Craft;
Enemy::Enemy(const std::string& image, float x, float y)
	: Actor(image), xPosition(x), yPosition(y)
{
	// 초기 위치 설정.
	SetPosition(Vector2(static_cast<int>(xPosition), static_cast<int>(yPosition)));

	color = Color::Cyan;
}

void Enemy::Tick(float deltaTime)
{
	super::Tick(deltaTime);

	// 플레이어 추적 로직.
	std::shared_ptr<Level> owner = GetOwner();
	if (owner)
	{
		auto player = owner->FindActor<Player>();
		if (player)
		{
			// 방향 벡터 계산.
			Vector2 pPos = player->GetPosition();
			float dx = static_cast<float>(pPos.x) - xPosition;
			float dy = static_cast<float>(pPos.y) - yPosition;
			
			// 벡터 정규화.
			float length = std::sqrt(dx * dx + dy * dy);
			if (length > 0.0f)
			{
				dx /= length;
				dy /= length;
			}
			
			// 이동.
			xPosition += dx * moveSpeed * deltaTime;
			yPosition += dy * moveSpeed * deltaTime;
		}
	}

	// 위치 설정.
	SetPosition(Vector2(static_cast<int>(xPosition), static_cast<int>(yPosition)));
}

void Enemy::OnCollision(const std::shared_ptr<Actor>& other)
{
	super::OnCollision(other);

	// Enemy가 경험치를 떨구는 것에 대한 확률을 구하기 위한 랜덤.
	int random = Util::RandomRange(1, 10);

	// 충돌한 다른 액터가 플레이어 탄약이면 삭제.
	// 커스텀 타입 활용.
	if (other->IsTypeOf<PlayerBullet>())
	{
		//  플레이어 탄약 제거.
		other->Destroy();

		// 적 액터 제거.
		Destroy();
		
			
		if (GetOwner())
		{
			// 죽은 자리에 이펙트 생성.
			GetOwner()->SpawnActor<DestroyEffect>(GetPosition());
			
			if (random <= 10)
			{
				// 죽은 자리에 경험치 생성.
				Vector2 expPos = GetPosition();
				expPos.x += (GetWidth() / 2);
				GetOwner()->SpawnActor<DestroyEXP>(expPos);
			}
		}
	}
}
