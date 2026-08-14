#include "EliteBoss.h"
#include <Util/Util.h>
#include <Engine/Engine.h>
#include <Level/Level.h>
#include <Actor/Player.h>
#include <Actor/PlayerBullet.h>
#include <Actor/DestroyEffect.h>
#include <Actor/DestroyEXP.h>
#include <Actor/DestroyAugmentPoint.h>
#include <cmath>

using namespace Craft;

EliteBoss::EliteBoss(const Craft::Vector2& position)
	: Actor("[==ELITE==]", position, Color::Purple)
{
	xPosition = static_cast<float>(position.x);
	yPosition = static_cast<float>(position.y);

	// 일반 몬스터보다 살짝 위에 그려짐.
	sortingOrder = 10; 
}

void EliteBoss::Tick(float deltaTime)
{
	super::Tick(deltaTime);

	// 플레이어 추적
	std::shared_ptr<Level> owner = GetOwner();
	if (owner)
	{
		auto player = owner->FindActor<Player>();
		if (player)
		{
			Vector2 pPos = player->GetPosition();
			float dx = static_cast<float>(pPos.x) - xPosition;
			float dy = static_cast<float>(pPos.y) - yPosition;
			
			float length = std::sqrt(dx * dx + dy * dy);
			if (length > 0.0f)
			{
				dx /= length;
				dy /= length;
			}
			
			xPosition += dx * moveSpeed * deltaTime;
			yPosition += dy * moveSpeed * deltaTime;
		}
	}
	SetPosition(Vector2(static_cast<int>(xPosition), static_cast<int>(yPosition)));
}



void EliteBoss::OnCollision(const std::shared_ptr<Actor>& other)
{
	super::OnCollision(other);

	int random = Util::RandomRange(1, 2);

	if (other->IsTypeOf<PlayerBullet>())
	{
		std::shared_ptr<PlayerBullet> bullet = Cast<PlayerBullet>(other);
		bool isShrapnel = bullet ? bullet->IsShrapnel() : false;

		// 총알 파괴.
		other->Destroy();
		
		// 체력 감소.
		eliteBossHp--;
		if (eliteBossHp <= 0)
		{
			
			// 죽어서 나온 파편이 아닐 시.
			if (GetOwner() && !isShrapnel)
			{
				auto player = GetOwner()->FindActor<Player>();
				// 플레이어의 Death Nova 상태 확인.
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
						
						// 파편이 스폰되자마자 자기 자신을 때리지 않도록 생성 좌표를 대각선으로 1칸씩 밀어줍니다.
						Vector2 spawnPos(pos.x + (dx > 0 ? 1 : -1), pos.y + (dy > 0 ? 1 : -1));
						GetOwner()->SpawnActor<PlayerBullet>(spawnPos, dx, dy, false, true);
					}
				}
			}

			Destroy();
			
		
			if (GetOwner())
			{
				GetOwner()->SpawnActor<DestroyEffect>(GetPosition());
				
				// 경험치 다량 스폰.
				Craft::Vector2 expPos = GetPosition();
				expPos.x += (GetWidth() / 2);
				// 50% 확률로 증강 포인트 드랍. (random 변수는 1 또는 2)
				if (random == 1)
				{
					GetOwner()->SpawnActor<DestroyAugmentPoint>(Vector2(expPos.x, expPos.y));
				}
				for (int i = 0; i < 15; ++i)
				{
					int offsetX = Util::RandomRange(-4, 4);
					int offsetY = Util::RandomRange(-2, 2);
					GetOwner()->SpawnActor<DestroyEXP>(Vector2(expPos.x + offsetX, expPos.y + offsetY));
				}
			}
		}
	}
}
