#include "EnemySpawner.h"
#include <Util/Util.h>
#include <Actor/Enemy.h>
#include <Level/Level.h>
#include <Level/DefenseLevel.h>

using namespace Craft;

// 생성할 적 이미지 타입 배열.
//static std::string enemyType[] =
//{
//	";:^:;",
//	"zZwZz",
//	"oO@Oo",
//	"<-=->",
//	")qOp(",
//};

EnemySpawner::EnemySpawner()
{
	// 적 생성 타이머 설정.
	spawnTimer.SetTargetTime(1.0f);

	// 웨이브 타이머 설정.
	waveTimer.SetTargetTime(30.0f);
}

void EnemySpawner::BeginPlay()
{
	Craft::Actor::BeginPlay();

	auto defenseLevel = Craft::Cast<DefenseLevel>(GetOwner());
	if (defenseLevel)
	{
		// 오브젝트 풀링: 미리 maxPerWave(30) 마리의 적을 생성해서 비활성화 상태로 보관
		for (int i = 0; i < maxPerWave; ++i)
		{
			// 생성과 동시에 비활성화 (보이지 않고 Tick도 안 돎)
			auto enemy = defenseLevel->SpawnActor<Enemy>(defenseLevel->GetSpawnPoint());
			enemy->SetActive(false);
			
			// 풀(Pool) 리스트에 등록
			enemyPool.push_back(enemy);
		}
	}
}

float EnemySpawner::GetRemainingWaveTime() const
{
	return waveTimer.GetRemainingTime();
}

void EnemySpawner::Tick(float deltaTime)
{
	Craft::Actor::Tick(deltaTime);

	if (!isWaveActive)
	{
		// 1. 대기 시간(쉬는 시간) 처리
		waveTimer.Tick(deltaTime);
		if (waveTimer.IsTimeOut())
		{
			// 타이머 종료 -> 웨이브 시작!
			isWaveActive = true;
			spawnedCount = 0; // 스폰 카운트 리셋
			spawnTimer.Reset();
		}
	}
	else
	{
		// 2. 웨이브 진행 중 (몬스터 스폰)
		if (spawnedCount >= maxPerWave)
		{
			// 이번 웨이브의 목표량 달성 -> 웨이브 종료 및 다음 웨이브 준비
			isWaveActive = false;
			currentWave++;
			
			// 다음 웨이브부터는 90초 대기
			waveTimer.SetTargetTime(90.0f);
			waveTimer.Reset();
			return;
		}

		spawnTimer.Tick(deltaTime);
		if (spawnTimer.IsTimeOut())
		{
			spawnTimer.Reset();
			SpawnEnemy();
		}
	}
}

void EnemySpawner::SpawnEnemy()
{
	auto defenseLevel = Craft::Cast<DefenseLevel>(GetOwner());
	if (!defenseLevel) return;

	// 풀(Pool)에서 현재 사용 중이지 않은(비활성화된) 몬스터 하나 찾기
	for (auto& weakEnemy : enemyPool)
	{
		auto enemy = weakEnemy.lock();
		if (enemy && !enemy->IsActive())
		{
			// 찾았다면! 다시 깨워서 출발선에 세움
			enemy->SetPosition(defenseLevel->GetSpawnPoint());
			enemy->SetActive(true);
			enemy->RecalculatePath(); // 깨어날 때 최신 맵 기준으로 길 찾기
			
			spawnedCount++; // 스폰 횟수 증가
			break; // 한 마리만 깨우고 끝
		}
	}
}
