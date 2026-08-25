#include "Turret.h"
#include <Render/Renderer.h>
#include <Engine/Engine.h>
#include <Level/Level.h>
#include <Actor/Enemy.h>
#include <Actor/TurretBullet.h>
#include <cmath>

using namespace Craft;
Turret::Turret(const Craft::Vector2& position)
	: Craft::Actor("TT", position, Craft::Color::Yellow)
{
	// 터렛은 다른 물체 위에 그려지도록 우선순위 상향
	sortingOrder = 10;

	// 자동 공격 타이머
	autoFireInterval.SetTargetTime(atkSpeed);
}

void Turret::Draw()
{
	if (!IsActive()) return;
	
	Craft::Vector2 screenPos = position;
	auto owner = GetOwner();
	if (owner)
	{
		Craft::Vector2 camPos = owner->GetCameraPosition();
		int screenWidth = Engine::Get().GetWidth();
		int screenHeight = Engine::Get().GetHeight();
		screenPos.x = position.x - camPos.x + (screenWidth / 2);
		screenPos.y = position.y - camPos.y + (screenHeight / 2);
	}

	// 2x2 크기 렌더링
	Renderer::Get().Submit("TT", screenPos, color, sortingOrder);
	
	screenPos.y += 1;
	Renderer::Get().Submit("TT", screenPos, color, sortingOrder);
}

void Turret::Tick(float deltaTime)
{
	super::Tick(deltaTime);

	// 1. 타이머 진행
	autoFireInterval.Tick(deltaTime);

	auto owner = GetOwner();
	if (owner)
	{
		// 2. 타이머가 완료되었을 때만 적 탐색 및 발사
		if (autoFireInterval.IsTimeOut())
		{
			std::shared_ptr<Enemy> targetEnemy = nullptr;
			float closestDistance = atkRange; // 사거리 내에 있는 적인지 확인하기 위한 기준

			// 맵 안의 모든 적을 순회하며 사거리 내에 있는지 검사
			for (auto enemy : owner->FindActors<Enemy>())
			{
				if (enemy->IsActive())
				{
					Vector2 tPos = GetPosition();
					Vector2 ePos = enemy->GetPosition();
					
					// 거리 계산
					float dx = static_cast<float>(ePos.x - tPos.x);
					float dy = static_cast<float>(ePos.y - tPos.y);
					float distance = std::sqrt(dx * dx + dy * dy);

					// 사거리(atkRange) 이내의 가장 가까운 적을 찾음
					if (distance <= closestDistance)
					{
						closestDistance = distance;
						targetEnemy = enemy;
					}
				}
			}

			// 사거리 내에 타겟이 존재하면 발사
			if (targetEnemy)
			{
				Vector2 tPos = GetPosition();
				Vector2 ePos = targetEnemy->GetPosition();

				// 적을 향하는 기본 방향 벡터 계산
				float baseDx = static_cast<float>(ePos.x) - tPos.x;
				float baseDy = static_cast<float>(ePos.y) - tPos.y;

				float length = std::sqrt(baseDx * baseDx + baseDy * baseDy);
				if (length > 0.0f)
				{
					// 정규화(길이를 1로 만듦)하여 방향만 추출 (유도탄 방지)
					baseDx /= length;
					baseDy /= length;
				}
				else
				{
					baseDx = 0.0f;
					baseDy = -1.0f;
				}

				// 총알 생성 및 발사 (터렛의 중심에서 발사)
				Vector2 bulletPosition(tPos.x + (width / 2), tPos.y);
				owner->SpawnActor<TurretBullet>(bulletPosition, baseDx, baseDy);

				// 3. 발사 후 타이머 리셋
				autoFireInterval.Reset();
			}
		}
	}
}
