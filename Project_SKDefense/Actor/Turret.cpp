#include "Turret.h"
#include <Render/Renderer.h>
#include <Engine/Engine.h>
#include <Level/Level.h>
#include <Actor/Enemy.h>
#include <Actor/TurretBullet.h>
#include <cmath>

using namespace Craft;
static int globalTurretSpawnCounter = 0;

// static 멤버 변수 초기화
int Turret::upgradeLevelFlame = 0;
int Turret::upgradeLevelIce = 0;
int Turret::upgradeLevelStorm = 0;

Turret::Turret(const Craft::Vector2& position, TurretType type)
	: Craft::Actor("TT", position, Craft::Color::Yellow)
{
	// 터렛은 다른 물체 위에 그려지도록 우선순위 상향
	sortingOrder = 10;

	// 고유 생성 번호 부여 (먼저 지어진 터렛 식별용)
	spawnOrder = ++globalTurretSpawnCounter;

	// 외부에서 주입된 타입 사용
	turretType = type;
	starTier = 1;

	// 속성 초기화
	UpdateStats();
}

void Turret::UpdateStats()
{
	// 타입 및 성급에 따른 외형, 색상, 스펙 설정
	switch (turretType)
	{
	case TurretType::FLAME:
		color = Craft::Color::Red;
		if (starTier == 1) { displaySymbol = "FF"; atkSpeed = 1.2f; atkRange = 3.0f; }
		else if (starTier == 2) { displaySymbol = "F+"; atkSpeed = 0.7f; atkRange = 4.5f; }
		else { displaySymbol = "F*"; atkSpeed = 0.35f; atkRange = 6.0f; }
		break;

	case TurretType::ICE:
		color = Craft::Color::Cyan;
		if (starTier == 1) { displaySymbol = "II"; atkSpeed = 1.5f; atkRange = 5.0f; }
		else if (starTier == 2) { displaySymbol = "I+"; atkSpeed = 0.9f; atkRange = 7.0f; }
		else { displaySymbol = "I*"; atkSpeed = 0.5f; atkRange = 10.0f; }
		break;

	case TurretType::STORM:
	default:
		color = Craft::Color::Yellow;
		if (starTier == 1) { displaySymbol = "TT"; atkSpeed = 1.0f; atkRange = 4.0f; }
		else if (starTier == 2) { displaySymbol = "T+"; atkSpeed = 0.55f; atkRange = 5.0f; }
		else { displaySymbol = "T*"; atkSpeed = 0.25f; atkRange = 7.0f; }
		break;
	}

	autoFireInterval.SetTargetTime(atkSpeed);
	autoFireInterval.Reset();
}

void Turret::UpgradeStar()
{
	if (starTier < 3)
	{
		starTier++;
		UpdateStats();
	}
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
	Renderer::Get().Submit(displaySymbol, screenPos, color, sortingOrder);
	
	screenPos.y += 1;
	Renderer::Get().Submit(displaySymbol, screenPos, color, sortingOrder);
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

				// 총알 생성 위치 (터렛의 중심)
				Vector2 bulletPosition(tPos.x + (width / 2), tPos.y);

				// 총알 위치에서 적을 향하는 방향 벡터 계산
				float baseDx = static_cast<float>(ePos.x) - bulletPosition.x;
				float baseDy = static_cast<float>(ePos.y) - bulletPosition.y;

				float length = std::sqrt(baseDx * baseDx + baseDy * baseDy);
				if (length > 0.0f)
				{
					// 정규화(길이를 1로 만듦)하여 방향만 추출
					baseDx /= length;
					baseDy /= length;
				}
				else
				{
					baseDx = 0.0f;
					baseDy = -1.0f;
				}

				// 총알 생성 및 발사
				auto bullet = owner->SpawnActor<TurretBullet>(bulletPosition, baseDx, baseDy);
				bullet->SetBulletRange(atkRange); // 터렛의 사거리를 총알에 적용

				// 기본 데미지(성급) + 터렛 속성별 업그레이드 수치 합산
				float finalDamage = static_cast<float>(starTier);
				if (turretType == TurretType::FLAME) finalDamage += upgradeLevelFlame;
				else if (turretType == TurretType::ICE) finalDamage += upgradeLevelIce;
				else if (turretType == TurretType::STORM) finalDamage += upgradeLevelStorm;
				
				bullet->SetBulletDamage(finalDamage);

				// 3. 발사 후 타이머 리셋
				autoFireInterval.Reset();
			}
		}
	}
}

