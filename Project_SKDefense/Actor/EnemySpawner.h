#pragma once

#include <Actor/Actor.h>
#include <Util/Timer.h>
#include <vector>
#include <memory>

class EnemySpawner:public Craft::Actor
{
	TYPE_DECLARATIONS(EnemySpawner, Actor)

public:
	EnemySpawner();

private:
	virtual void BeginPlay() override;
	virtual void Tick(float deltaTime) override;

	// 적 생성 함수.
	void SpawnEnemy();

private:
	//타이머.
	Timer timer;

	// 오브젝트 풀링을 위한 리스트
	std::vector<std::weak_ptr<class Enemy>> enemyPool;
	
	int spawnedCount = 0;
	const int maxPerWave = 30;
};

