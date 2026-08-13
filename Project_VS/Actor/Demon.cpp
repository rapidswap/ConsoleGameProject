#include "Demon.h"
#include <Level/GameLevel.h>
#include <Actor/Player.h>
#include <Util/Util.h>
#include <Actor/EnemyBullet.h>
#include <Actor/PlayerBullet.h>
#include <Actor/DestroyEffect.h>
#include <cmath>

using namespace Craft;
Demon::Demon(const Craft::Vector2& position)
	:Actor("[==^DEMON^==]", position, Color::Red)
{
	
	xPosition = static_cast<float>(position.x);
	yPosition = static_cast<float>(position.y);

	// 발사 타이머.
	timer.SetTargetTime(3.0f);

	sortingOrder = 11;
}

void Demon::Tick(float deltaTime)
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

	// 발사 처리.
	timer.Tick(deltaTime);
	fireTimer.Tick(deltaTime);

	// 패턴이 종료되고 쿨타임(timer)이 다 차면 새로운 랜덤 패턴 시작.
	if (timer.IsTimeOut())
	{
		// 1~4 랜덤 패턴 선택.
		currentPattern = Util::RandomRange(1, 4); 
		patternStep = 0;
		currentAngle = 0.0f;
		timer.Reset();
		
		if (currentPattern == 2)
		{
			// 순차 발사는 즉시 시작.
			fireTimer.SetTargetTime(0.0f); 
		}
		else
		{
			fireTimer.SetTargetTime(0.0f); 
		}
	}

	if (currentPattern > 0 && fireTimer.IsTimeOut())
	{
		Vector2 bulletPosition(GetPosition().x + (width / 2), GetPosition().y + 1);
		// 패턴 1: 360도 동시 12발.
		if (currentPattern == 1) 
		{
			for (int i = 0; i < 12; ++i)
			{
				float rad = (i * 30.0f) * 3.141592f / 180.0f;
				float dx = std::cos(rad);
				float dy = std::sin(rad);
				owner->SpawnActor<EnemyBullet>(bulletPosition, dx, dy, 20.0f);
			}
			
			currentPattern = 0;
			// 패턴 종료 후 대기.
			timer.SetTargetTime(2.0f);
			timer.Reset();
		}
		// 패턴 2: 360도 순차 발사 (총 24발, 0.1초 간격).
		else if (currentPattern == 2)
		{
			float rad = currentAngle * 3.141592f / 180.0f;
			float dx = std::cos(rad);
			float dy = std::sin(rad);
			owner->SpawnActor<EnemyBullet>(bulletPosition, dx, dy, 20.0f);

			// 15도씩 회전.
			currentAngle += 15.0f;
			patternStep++;

			if (patternStep >= 24)
			{
				currentPattern = 0;
				timer.SetTargetTime(2.0f);
				timer.Reset();
			}
			else
			{
				fireTimer.SetTargetTime(0.1f);
				fireTimer.Reset();
			}
		}
		// 패턴 3: 십자 방향 3발씩.
		else if (currentPattern == 3) 
		{
			float baseAngles[4] = {0.0f, 90.0f, 180.0f, 270.0f};
			for (int i = 0; i < 4; ++i)
			{
				for (int j = -1; j <= 1; ++j)
				{
					float angle = baseAngles[i] + (j * 15.0f);
					float rad = angle * 3.141592f / 180.0f;
					float dx = std::cos(rad);
					float dy = std::sin(rad);
					owner->SpawnActor<EnemyBullet>(bulletPosition, dx, dy, 20.0f);
				}
			}
			
			currentPattern = 0;
			timer.SetTargetTime(2.0f);
			timer.Reset();
		}
		// 패턴 4: 대각선 십자(X자) 방향 3발씩.
		else if (currentPattern == 4) 
		{
			float baseAngles[4] = {45.0f, 135.0f, 225.0f, 315.0f};
			for (int i = 0; i < 4; ++i)
			{
				for (int j = -1; j <= 1; ++j)
				{
					float angle = baseAngles[i] + (j * 15.0f);
					float rad = angle * 3.141592f / 180.0f;
					float dx = std::cos(rad);
					float dy = std::sin(rad);
					owner->SpawnActor<EnemyBullet>(bulletPosition, dx, dy, 20.0f);
				}
			}
			
			currentPattern = 0;
			timer.SetTargetTime(2.0f);
			timer.Reset();
		}
	}

}


void Demon::OnCollision(const std::shared_ptr<Actor>& other)
{
	super::OnCollision(other);

	if (other->IsTypeOf<PlayerBullet>())
	{
		// 총알 파괴.
		other->Destroy();


		std::shared_ptr<Level> owner = GetOwner();
		if (owner)
		{
			// 레벨을 GameLevel로 변환한 뒤 체력 깎기 처리.
			std::shared_ptr<GameLevel> gameLevel = Cast<GameLevel>(owner);
			if (gameLevel)
			{
				gameLevel->TakeDemonDamage();
			}
		}

	}
}
