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

public:
	// 디버그용: 즉시 보스 소환
	void ForceSpawnDemon();

	// 무한 루프 진입 (보스 처치 시 호출됨)
	void NextLoop();
	
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

	// 데몬 소환 시 연출용 타이머.
	Timer demonWaitTimer;

	// 데몬 스폰 단계 (0: 평상시, 1: 전멸 후 딜레이, 2: 데몬 소환 완료)
	int demonSpawnStep = 0;
	
	// 무한 루프 난이도 제어 변수
	int loopCount = 0;
	float difficultyMultiplier = 1.0f;
};

