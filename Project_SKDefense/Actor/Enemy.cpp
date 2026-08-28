#include "Enemy.h"
#include <Render/Renderer.h>
#include <Engine/Engine.h>
#include <Level/DefenseLevel.h>
#include <Algorithm/AStar.h>
#include <Algorithm/Node.h>
#include <Actor/TurretBullet.h>
#include <Actor/Agit.h>
#include <cmath>

using namespace Craft;
Enemy::Enemy(const Vector2& position)
	: Actor("Z", position, Color::Red)
{
	sortingOrder = 15;
}

void Enemy::BeginPlay()
{
	Actor::BeginPlay();
	RecalculatePath();
}

void Enemy::RecalculatePath()
{
	auto defenseLevel = Cast<DefenseLevel>(GetOwner());
	if (defenseLevel)
	{
		AStar astar;
		// 현재 위치를 정수로 변환하여 시작점으로 사용 (정확한 칸 맞춤)
		Vector2 spawn = position; 
		Vector2 target = defenseLevel->GetTargetPoint();

		Node* startNode = new Node(static_cast<int>(std::round(spawn.x)), static_cast<int>(std::round(spawn.y)));
		Node* goalNode = new Node(static_cast<int>(std::round(target.x)), static_cast<int>(std::round(target.y)));

		std::vector<Node*> nodePath = astar.FindPath(startNode, goalNode, defenseLevel->GetMapGrid());

		path.clear();
		// 길찾기 결과 복사 (시작 노드는 보통 현재 위치이므로 제외)
		for (size_t i = 1; i < nodePath.size(); ++i)
		{
			path.push_back(Vector2(static_cast<float>(nodePath[i]->position.x), static_cast<float>(nodePath[i]->position.y)));
		}
		
		currentPathIndex = 0;
		moveAccumulator = 0.0f; // 이동 진행도 초기화
		
		delete goalNode;
	}
}

void Enemy::Tick(float deltaTime)
{
	Actor::Tick(deltaTime);
	
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
		// 경로 끝(아지트) 도착 처리: 아지트 데미지를 입히고 사라짐(비활성화)
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
		
		// 골드 획득 (예: 1마리당 10골드)
		auto defenseLevel = Craft::Cast<DefenseLevel>(GetOwner());
		if (defenseLevel)
		{
			defenseLevel->AddGold(10);
		}

		// 오브젝트 풀링을 위해 파괴 대신 비활성화
		SetActive(false);
	}

	// 아지트에 도달했다면.
	if (other->IsTypeOf<Agit>())
	{
		// 아지트 체력은 다운.
		auto agitOther = Cast<Agit>(other);
		agitOther->AgitHealthDown();
		SetActive(false);
	}

	
}
