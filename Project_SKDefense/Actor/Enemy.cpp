#include "Enemy.h"
#include <Render/Renderer.h>
#include <Engine/Engine.h>
#include <Level/DefenseLevel.h>
#include <Algorithm/AStar.h>
#include <Algorithm/Node.h>
#include <cmath>

Enemy::Enemy(const Craft::Vector2& position)
	: Craft::Actor("Z", position, Craft::Color::Red)
{
	sortingOrder = 15;
}

void Enemy::BeginPlay()
{
	Craft::Actor::BeginPlay();
	RecalculatePath();
}

void Enemy::RecalculatePath()
{
	auto defenseLevel = Craft::Cast<DefenseLevel>(GetOwner());
	if (defenseLevel)
	{
		AStar astar;
		// 현재 위치를 정수로 변환하여 시작점으로 사용 (정확한 칸 맞춤)
		Craft::Vector2 spawn = position; 
		Craft::Vector2 target = defenseLevel->GetTargetPoint();

		Node* startNode = new Node(static_cast<int>(std::round(spawn.x)), static_cast<int>(std::round(spawn.y)));
		Node* goalNode = new Node(static_cast<int>(std::round(target.x)), static_cast<int>(std::round(target.y)));

		std::vector<Node*> nodePath = astar.FindPath(startNode, goalNode, defenseLevel->GetMapGrid());

		path.clear();
		// 길찾기 결과 복사 (시작 노드는 보통 현재 위치이므로 제외)
		for (size_t i = 1; i < nodePath.size(); ++i)
		{
			path.push_back(Craft::Vector2(static_cast<float>(nodePath[i]->position.x), static_cast<float>(nodePath[i]->position.y)));
		}
		
		currentPathIndex = 0;
		moveAccumulator = 0.0f; // 이동 진행도 초기화
		
		delete goalNode;
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
		// Todo: 경로 끝 도착 처리 (예: 아지트 데미지 입히고 파괴)
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
