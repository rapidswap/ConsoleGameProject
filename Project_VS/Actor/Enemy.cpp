#include "Enemy.h"
#include <Util/Util.h>
#include <Engine/Engine.h>
#include <Level/Level.h>
#include <Actor/Player.h>
#include <cmath>
#include <Actor/PlayerBullet.h>
#include <Actor/DestroyEffect.h>
#include <Actor/DestroyEXP.h>
#include <Actor/DestroyAugmentPoint.h>

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
	int random = Util::RandomRange(1, 100);

	// 충돌한 다른 액터가 플레이어 탄약이면 삭제.
	// 커스텀 타입 활용.
	if (other->IsTypeOf<PlayerBullet>())
	{
		std::shared_ptr<PlayerBullet> bullet = Cast<PlayerBullet>(other);
		bool isShrapnel = bullet ? bullet->IsShrapnel() : false;

		//  플레이어 탄약 제거.
		other->Destroy();

		// 플레이어의 Death Nova 상태 확인
		if (GetOwner() && !isShrapnel)
		{
			auto player = GetOwner()->FindActor<Player>();
			if (player && player->HasDeathNova())
			{
				Vector2 pos = GetPosition();
				
				// 대각선 4방향 (45, 135, 225, 315도)
				float baseAngles[4] = {45.0f, 135.0f, 225.0f, 315.0f};
				for (int i = 0; i < 4; ++i)
				{
					float rad = baseAngles[i] * 3.141592f / 180.0f;
					float dx = std::cos(rad);
					float dy = std::sin(rad);
					
					// isBouncing=false, isShrapnel=true 로 생성
					GetOwner()->SpawnActor<PlayerBullet>(pos, dx, dy, false, true);
				}
			}
		}

		// 적 액터 제거.
		Destroy();
		
			
		if (GetOwner())
		{
			// 죽은 자리에 이펙트 생성.
			GetOwner()->SpawnActor<DestroyEffect>(GetPosition());
			
			if (random <= 99)
			{
				// 죽은 자리에 경험치 생성.
				Vector2 expPos = GetPosition();
				expPos.x += (GetWidth() / 2);
				GetOwner()->SpawnActor<DestroyEXP>(expPos);
			}
			else
			{
				// 죽은 자리에 증강 포인트 생성.
				Vector2 augPos = GetPosition();
				augPos.x += (GetWidth() / 2);
				GetOwner()->SpawnActor<DestroyAugmentPoint>(augPos);
			}
		}
	}
}
