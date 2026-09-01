#pragma once

#include <Actor/Actor.h>
#include <vector>

class Enemy : public Craft::Actor
{
	// 커스텀 타입설정.
	TYPE_DECLARATIONS(Enemy, Actor)

public:
	Enemy(const Craft::Vector2& position);

	// 스탯 설정용 Setter
	inline void SetMoveSpeed(float speed) { moveSpeed = speed; }

	inline void SetHealth(float health) { enemyHealth = health; }

private:
	// 이벤트 함수 오버라이드.
	virtual void BeginPlay() override;
	virtual void Tick(float deltaTime) override;
	virtual void Draw() override;

public:
	// 터렛이 설치될 때마다 경로를 갱신하기 위한 함수
	void RecalculatePath();
	
	const std::vector<Craft::Vector2>& GetPath() const { return path; }
	int GetCurrentPathIndex() const { return currentPathIndex; }
	
	const std::vector<Craft::Vector2>& GetSearchHistory() const { return searchHistory; }

	// 오브젝트 풀링을 위한 활성화 설정
	inline void SetActive(bool active) { isActive = active; }

private:
	// 충돌 처리 함수 오버라이드.
	virtual void OnCollision(const std::shared_ptr<Actor>& other) override;

private:
	// 위치 이동 처리를 위한 변수.
	float xPosition = 0.0f;
	float yPosition = 0.0f;
	float moveSpeed = 4.0f; // 한 칸 이동하는데 걸리는 시간(초).
	float movementTimer = 0.0f;

	// 이동 경로와 시각화 히스토리
	std::vector<Craft::Vector2> path;
	std::vector<Craft::Vector2> searchHistory;
	int currentPathIndex = 0;
	float moveAccumulator = 0.0f;

public:
	int debugAnimFrame = 0;

private:
	// 적의 체력.
	float enemyHealth = 1.0f;
};
