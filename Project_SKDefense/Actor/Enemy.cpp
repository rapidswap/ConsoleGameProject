#include "Enemy.h"
#include <Render/Renderer.h>
#include <Engine/Engine.h>
#include <Level/DefenseLevel.h>
#include <Algorithm/AStar.h>
#include <Algorithm/Node.h>

Enemy::Enemy(const Craft::Vector2& position)
	: Craft::Actor("Z", position, Craft::Color::Red)
{
	sortingOrder = 15;
}

void Enemy::BeginPlay()
{
	Craft::Actor::BeginPlay();

	auto defenseLevel = Craft::Cast<DefenseLevel>(GetOwner());
	if (defenseLevel)
	{
		AStar astar;
		Craft::Vector2 spawn = defenseLevel->GetSpawnPoint();
		Craft::Vector2 target = defenseLevel->GetTargetPoint();

		Node* startNode = new Node(static_cast<int>(spawn.x), static_cast<int>(spawn.y));
		Node* goalNode = new Node(static_cast<int>(target.x), static_cast<int>(target.y));

		std::vector<Node*> nodePath = astar.FindPath(startNode, goalNode, defenseLevel->GetMapGrid());

		// 길찾기 결과 복사 (시작 노드는 보통 현재 위치이므로 제외)
		for (size_t i = 1; i < nodePath.size(); ++i)
		{
			path.push_back(Craft::Vector2(static_cast<float>(nodePath[i]->position.x), static_cast<float>(nodePath[i]->position.y)));
		}
		
		currentPathIndex = 0;
	}
}

void Enemy::Tick(float deltaTime)
{
	Craft::Actor::Tick(deltaTime);
	
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
		// 경로 끝 도착 처리 (예: 아지트 데미지 입히고 파괴)
	}
}

void Enemy::Draw()
{
	if (!IsActive()) return;
	
	Craft::Vector2 screenPos = position;
	auto owner = GetOwner();
	if (owner)
	{
		Craft::Vector2 camPos = owner->GetCameraPosition();
		int screenWidth = Craft::Engine::Get().GetWidth();
		int screenHeight = Craft::Engine::Get().GetHeight();
		screenPos.x = position.x - camPos.x + (screenWidth / 2);
		screenPos.y = position.y - camPos.y + (screenHeight / 2);
	}

	Craft::Renderer::Get().Submit(image, screenPos, color, sortingOrder);
}

void Enemy::OnCollision(const std::shared_ptr<Craft::Actor>& other)
{
	Craft::Actor::OnCollision(other);
}
