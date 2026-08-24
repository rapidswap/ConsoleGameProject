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

public:
	// 디펜스 레벨에서 화면에 그려주기 위한 Getter
	int GetCurrentWave() const { return currentWave; }
	float GetRemainingWaveTime() const;
	bool IsWaveActive() const { return isWaveActive; }

private:
	// 몬스터 소환 타이머.
	Timer spawnTimer;

	// 웨이브 타이머 (처음 준비 시간은 30초).
	Timer waveTimer;

	// 오브젝트 풀링을 위한 리스트
	std::vector<std::weak_ptr<class Enemy>> enemyPool;
	
	int spawnedCount = 0;
	const int maxPerWave = 30;
	
	int currentWave = 1;
	bool isWaveActive = false;
};

