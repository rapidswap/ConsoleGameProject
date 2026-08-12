#pragma once

#include <Actor/Actor.h>
#include <Util/Timer.h>

class EnemySpawner:public Craft::Actor
{
	TYPE_DECLARATIONS(EnemySpawner, Actor)

public:
	EnemySpawner();

private:
	virtual void Tick(float deltaTime);

	// 적 생성 함수.
	void SpawnEnemy();

	// 중간 보스 생성 함수.
	void SpawnBoss();

	
private:

	float minSpawnTime = 1.0f;
	float maxSpawnTime = 2.5f;


	//타이머.
	Timer timer;

	// 난이도를 올리기 위한 타이머.
	Timer diffcultyTimer;

	// 보스를 소환하기 위한 타이머.
	Timer eliteBossTimer;
};

