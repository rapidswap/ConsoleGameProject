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
	
	// 오브젝트 풀링을 위한 활성화 설정
	inline void SetActive(bool active) { isActive = active; }

private:
	// 충돌 처리 함수 오버라이드.
	virtual void OnCollision(const std::shared_ptr<Actor>& other) override;

private:
private:
	// 위치 이동 처리를 위한 변수.
	float xPosition = 0.0f;
	float yPosition = 0.0f;
	float moveSpeed = 5.0f;

	// A* 경로 추적을 위한 변수들
	std::vector<Craft::Vector2> path;
	int currentPathIndex = 0;
	float moveAccumulator = 0.0f;

	// 적의 체력.
	float enemyHealth = 5.0f;
};

