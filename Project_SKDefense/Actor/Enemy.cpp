#define WIN32_LEAN_AND_MEAN
#include <WinSock2.h>
#include "Enemy.h"
#include <Render/Renderer.h>
#include <Engine/Engine.h>
#include <Level/DefenseLevel.h>
#include <Actor/TurretBullet.h>
#include <Actor/Agit.h>
#include "Network/NetworkManager.h"
#include "Common/Protocol.h"
#include <cmath>
#include <iostream>

using namespace Craft;
Enemy::Enemy(const Vector2& position)
	: Actor("Z", position, Color::Red)
{
	// 렌더링 우선순위 (5: 몬스터는 A* 최종 경로(4) 위에 그려짐)
	sortingOrder = 5;
}

void Enemy::BeginPlay()
{
	Actor::BeginPlay();
	RecalculatePath();
}

void Enemy::RecalculatePath()
{
	if (NetworkManager::Get()->IsConnected()) return;
	auto defenseLevel = Cast<DefenseLevel>(GetOwner());
	if (defenseLevel)
	{
		const auto result = defenseLevel->FindOfflinePath({static_cast<int>(std::round(position.x)), static_cast<int>(std::round(position.y))});

		path.clear();
		// 길찾기 결과 복사 (시작 노드는 보통 현재 위치이므로 제외)
		for (size_t i = 1; i < result.path.size(); ++i)
		{
			path.emplace_back(Vector2(static_cast<float>(result.path[i].x), static_cast<float>(result.path[i].y)));
		}
		
		currentPathIndex = 0;
		moveAccumulator = 0.0f; // 이동 진행도 초기화
		debugAnimFrame = 0; // 시각화 애니메이션 초기화
		
		// 시각화를 위해 탐색 히스토리 복사
		searchHistory.clear();
		for (const auto& pos : result.history)
		{
			searchHistory.emplace_back(Vector2(static_cast<float>(pos.x), static_cast<float>(pos.y)));
		}

		
	}
}

void Enemy::ApplyServerPath(uint32_t revision, const PathCoordinate* points, size_t pathCount, size_t historyCount)
{
	if (revision < pathRevision || pathCount == 0) return;
	pathRevision = revision;
	path.clear();
	searchHistory.clear();
	SetPosition(Vector2(points[0].x, points[0].y));
	for (size_t i = 0; i < pathCount; ++i) path.emplace_back(Vector2(points[i].x, points[i].y));
	for (size_t i = 0; i < historyCount; ++i) searchHistory.emplace_back(Vector2(points[pathCount + i].x, points[pathCount + i].y));
	currentPathIndex = 1;
	debugAnimFrame = 0;
}

void Enemy::ApplyServerMove(const S_MONSTER_MOVE_PACKET& pkt)
{
	if (pkt.mapRevision != pathRevision || pkt.pathIndex > path.size()) return;
	SetPosition(Vector2(pkt.position.x, pkt.position.y));
	currentPathIndex = pkt.pathIndex;
	if (!pkt.active) SetActive(false);
}

void Enemy::Tick(float deltaTime)
{
	Actor::Tick(deltaTime);
	if (NetworkManager::Get()->IsConnected()) return;
	
	// A* 경로를 따라 한 칸씩 이동
	if (currentPathIndex < path.size())
	{
		moveAccumulator += deltaTime * moveSpeed;
		if (moveAccumulator >= 1.0f)
		{
			SetPosition(path[currentPathIndex]);
			currentPathIndex++;
			moveAccumulator = 0.0f;
		}
	}
	else
	{
		// 경로 끝(아지트) 도착 처리: 아지트에 도달하여 데미지를 입히고 소멸(골드 미지급)
		auto defenseLevel = Craft::Cast<DefenseLevel>(GetOwner());
		// A failed search must not damage the agit from an unreachable cell.
		if (!defenseLevel || position != defenseLevel->GetTargetPoint()) return;
		if (defenseLevel)
		{
			defenseLevel->DamageAgit(1);
			std::cout << "[Enemy] Reached Agit! Agit Damaged (No Gold Awarded)\n";
		}
		SetActive(false);
	}
}

void Enemy::Draw()
{
	if (!IsActive()) return;
	
	Vector2 screenPos = position;
	auto owner = GetOwner();
	if (owner)
	{
		Vector2 camPos = owner->GetCameraPosition();
		int screenWidth = Engine::Get().GetWidth();
		int screenHeight = Engine::Get().GetHeight();
		screenPos.x = position.x - camPos.x + (screenWidth / 2);
		screenPos.y = position.y - camPos.y + (screenHeight / 2);
	}

	Renderer::Get().Submit(image, screenPos, color, sortingOrder);
}

void Enemy::OnCollision(const std::shared_ptr<Actor>& other)
{
	super::OnCollision(other);
	if (killReported) return;

	// 터렛 총알에 맞았다면.
	if (other->IsTypeOf<TurretBullet>())
	{
		auto bullet = Craft::Cast<TurretBullet>(other);
		float damage = bullet ? bullet->GetBulletDamage() : 1.0f;
		
		// 총알 파괴.
		other->Destroy();

		// enemy 체력 다운.
		enemyHealth -= damage;
	}

	if (enemyHealth <= 0)
	{
		// 죽었다면 이펙트 스폰.
		//GetOwner()->SpawnActor<DestroyEffect>(GetPosition());
		
		auto defenseLevel = Craft::Cast<DefenseLevel>(GetOwner());
		if (defenseLevel)
		{
			if (NetworkManager::Get()->IsConnected())
			{
				// 멀티플레이: 서버에 특정 몬스터 처치 보고 (서버가 검증 후 S_ENEMY_KILL로 전원에게 브로드캐스트)
				C_ENEMY_KILL_PACKET pkt;
				pkt.monsterId = monsterId;
				pkt.rewardGold = 10;
				NetworkManager::Get()->Send(reinterpret_cast<BYTE*>(&pkt), pkt.size);
				killReported = true;
				return; // Keep this pool slot until the server confirms death/arrival.
			}
			else
			{
				// 싱글플레이: 로컬 즉시 골드 지급
				defenseLevel->AddGold(10);
				std::cout << "[Enemy] Killed! +10G (Current Gold: " << defenseLevel->GetGold() << "G)\n";
			}
		}

		// 오브젝트 풀링을 위해 파괴 대신 비활성화
		SetActive(false);
		return;
	}

	// 아지트에 도달했다면.
	if (other->IsTypeOf<Agit>())
	{
		if (NetworkManager::Get()->IsConnected()) return;
		auto defenseLevel = Craft::Cast<DefenseLevel>(GetOwner());
		if (defenseLevel)
		{
			defenseLevel->DamageAgit(1);
			std::cout << "[Enemy] Collided with Agit! Agit Damaged (No Gold Awarded)\n";
		}
		else
		{
			auto agitOther = Cast<Agit>(other);
			if (agitOther)
			{
				agitOther->AgitHealthDown();
			}
		}
		SetActive(false);
	}
}
