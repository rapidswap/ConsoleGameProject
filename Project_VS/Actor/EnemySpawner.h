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

	// 엘리트 보스 생성 함수.
	void SpawnElite();

	// 데몬 생성 함수.
	void SpawnDemon();

	
private:

	float minSpawnTime = 1.0f;
	float maxSpawnTime = 2.0f;


	//타이머.
	Timer timer;

	// 난이도를 올리기 위한 타이머.
	Timer diffcultyTimer;

	// 엘리트를 소환하기 위한 타이머.
	Timer eliteBossTimer;

	// 데몬을 소환하기 위한 타이머.
	Timer demonTimer;

	// 데몬 소환시 모든 액터가 나오지 않게 할 변수.
	bool isDemonSpawned = false;
	

};

