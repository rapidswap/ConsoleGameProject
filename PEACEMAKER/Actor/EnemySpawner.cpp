#include "EnemySpawner.h"
#include <Util/Util.h>
#include <Actor/Enemy.h>
#include <Actor/EliteBoss.h>
#include <Actor/Demon.h>
#include <Actor/SpawnDemonEffect.h>
#include <Level/Level.h>
#include <Level/GameLevel.h>
#include <Engine/Engine.h>

using namespace Craft;

// 생성할 적 이미지 타입 배열.
static std::string enemyType[] =
{
	";:^:;",   
	"zZwZz",   
	"oO@Oo",   
	"<-=->",   
	")qOp(",   
	"[x_x]",   
	"~@_@~",   
	">+_+<",   
	"|O_O|",   
	"{*v*}",   
	"v^o^v",   
	"<o_o>"    
};

EnemySpawner::EnemySpawner()
{
	// 적 생성 타이머 설정.
	timer.SetTargetTime(Util::RandomRange(minSpawnTime, maxSpawnTime));
	diffcultyTimer.SetTargetTime(1.0f);
	
	// 엘리트 보스 스폰 타이머 설정.
	eliteBossTimer.SetTargetTime(50.0f);

	// 데몬 스폰 타이머 설정
	demonTimer.SetTargetTime(300.0f);

}

void EnemySpawner::Tick(float deltaTime)
{
	super::Tick(deltaTime);

	// 스폰 완료 상태(보스전 돌입)면 아무것도 하지 않음.
	if (demonSpawnStep == 2) return;

	// 대기 상태(전멸 후 이펙트 감상 중)
	if (demonSpawnStep == 1)
	{
		demonWaitTimer.Tick(deltaTime);
		if (demonWaitTimer.IsTimeOut())
		{
			demonSpawnStep = 2; // 스폰 완료 상태로 전환
			SpawnDemon();

			std::shared_ptr<Level> owner = GetOwner();
			if (owner)
			{
				std::shared_ptr<GameLevel> gameLevel = Cast<GameLevel>(owner);
				if (gameLevel)
				{
					gameLevel->ShowLevelUpMenu(3); // 대기 끝나고 데몬 나오면서 증강
				}
			}
		}
		return; // 대기 중이므로 평소 스폰 로직 실행 안 함
	}

	// 이하 demonSpawnStep == 0 (평상시) 로직
	// 타이머 업데이트.
	timer.Tick(deltaTime);
	diffcultyTimer.Tick(deltaTime);
	eliteBossTimer.Tick(deltaTime);
	demonTimer.Tick(deltaTime);

	if (demonTimer.IsTimeOut())
	{
		demonSpawnStep = 1; // 대기 상태로 전환
		demonWaitTimer.SetTargetTime(2.0f); // 2초 딜레이
		demonWaitTimer.Reset();

		// 화면의 잡몹과 엘리트 보스 즉시 전멸
		std::shared_ptr<Level> owner = GetOwner();
		if (owner)
		{
			std::shared_ptr<GameLevel> gameLevel = Cast<GameLevel>(owner);
			if (gameLevel)
			{
				gameLevel->WipeOutEnemies();
				
				// 맵 정중앙에 데몬 소환 전조 이펙트(포탈) 스폰
				int screenWidth = Engine::Get().GetWidth();
				int screenHeight = Engine::Get().GetHeight();
				float spawnX = static_cast<float>(screenWidth) / 2;
				float spawnY = static_cast<float>(screenHeight) / 2;
				
				gameLevel->SpawnActor<SpawnDemonEffect>(Craft::Vector2((int)spawnX, (int)spawnY));
			}
		}
		
		return;
	}
	if (eliteBossTimer.IsTimeOut())
	{
		eliteBossTimer.Reset();
		eliteBossTimer.SetTargetTime(50.0f);
		SpawnElite();
	}

	if (diffcultyTimer.IsTimeOut())
	{
		diffcultyTimer.Reset();

		minSpawnTime -= 0.005f;
		maxSpawnTime -= 0.005f;

		if (minSpawnTime < 0.05f) minSpawnTime = 0.05f;
		if (maxSpawnTime < 0.08f) maxSpawnTime = 0.08f;
	}

	// 경과 시간 확인.
	if (!timer.IsTimeOut())
	{
		return;
	}

	// 타이머 초기화.
	timer.Reset();

	// 다음 난이도 타이머 설정.
	timer.SetTargetTime(Util::RandomRange(minSpawnTime, maxSpawnTime));

	// 적 생성.
	SpawnEnemy();
}

void EnemySpawner::SpawnEnemy()
{
	std::shared_ptr<Level> owner = GetOwner();
	if (owner == nullptr) return;

	int screenWidth = Engine::Get().GetWidth();
	int screenHeight = Engine::Get().GetHeight();
	float spawnX = 0.0f;
	float spawnY = 0.0f;
	
	int side = Util::RandomRange(0, 3);
	switch (side)
	{
	case 0: spawnX = static_cast<float>(Util::RandomRange(0, screenWidth - 1)); spawnY = 0.0f; break;
	case 1: spawnX = static_cast<float>(Util::RandomRange(0, screenWidth - 1)); spawnY = static_cast<float>(screenHeight - 1); break;
	case 2: spawnX = 0.0f; spawnY = static_cast<float>(Util::RandomRange(0, screenHeight - 1)); break;
	case 3: spawnX = static_cast<float>(screenWidth - 1); spawnY = static_cast<float>(Util::RandomRange(0, screenHeight - 1)); break;
	}
	int type = Util::RandomRange(0, 11);
	auto enemy = owner->SpawnActor<Enemy>(enemyType[type], spawnX, spawnY);
	if (enemy)
	{
		enemy->SetMoveSpeed(5.0f * difficultyMultiplier);
	}
}

void EnemySpawner::SpawnElite()
{
	std::shared_ptr<Level> owner = GetOwner();
	if (owner == nullptr) return;

	int screenWidth = Engine::Get().GetWidth();
	int screenHeight = Engine::Get().GetHeight();
	float spawnX = 0.0f;
	float spawnY = 0.0f;
	
	int side = Util::RandomRange(0, 3);
	switch (side)
	{
	case 0: spawnX = static_cast<float>(Util::RandomRange(0, screenWidth - 1)); spawnY = 0.0f; break;
	case 1: spawnX = static_cast<float>(Util::RandomRange(0, screenWidth - 1)); spawnY = static_cast<float>(screenHeight - 1); break;
	case 2: spawnX = 0.0f; spawnY = static_cast<float>(Util::RandomRange(0, screenHeight - 1)); break;
	case 3: spawnX = static_cast<float>(screenWidth - 1); spawnY = static_cast<float>(Util::RandomRange(0, screenHeight - 1)); break;
	}

	auto boss = owner->SpawnActor<EliteBoss>(Vector2(static_cast<int>(spawnX), static_cast<int>(spawnY)));
	if (boss)
	{
		boss->SetMaxHp(static_cast<int>(20 * difficultyMultiplier));
		boss->SetMoveSpeed(8.0f * difficultyMultiplier);
	}
}

void EnemySpawner::SpawnDemon()
{
	std::shared_ptr<Level> owner = GetOwner();
	if (owner == nullptr) return;

	int screenWidth = Engine::Get().GetWidth();
	int screenHeight = Engine::Get().GetHeight();
	
	// 화면 중앙에 스폰.
	float spawnX = static_cast<float>(screenWidth / 2);
	float spawnY = static_cast<float>(screenHeight / 2);

	auto demon = owner->SpawnActor<Demon>(Craft::Vector2((int)spawnX, (int)spawnY));
	if (demon)
	{
		demon->SetMaxHp(static_cast<int>(100 * difficultyMultiplier));
	}
}

void EnemySpawner::NextLoop()
{
	loopCount++;
	difficultyMultiplier *= 2.0f; // 매 루프마다 난이도 배수 2배 증가.
	
	// 최소/최대 스폰 속도 감소 (더 빠르게 쏟아짐).
	minSpawnTime = (std::max)(0.1f, minSpawnTime * 0.7f);
	maxSpawnTime = (std::max)(0.2f, maxSpawnTime * 0.7f);
	timer.SetTargetTime(Util::RandomRange(minSpawnTime, maxSpawnTime));
	
	// 데몬 스폰 단계 초기화.
	demonSpawnStep = 0;
	
	// 다음 보스전은 더 빨리 오도록 (최소 60초).
	float nextDemonTime = (std::max)(60.0f, 300.0f * std::pow(0.8f, (float)loopCount));
	demonTimer.SetTargetTime(nextDemonTime);
	demonTimer.Reset();
	
	// 엘리트 보스도 더 자주 나오도록.
	float nextEliteTime = (std::max)(15.0f, 50.0f * std::pow(0.8f, (float)loopCount));
	eliteBossTimer.SetTargetTime(nextEliteTime);
	eliteBossTimer.Reset();
}

void EnemySpawner::ForceSpawnDemon()
{
	if (demonSpawnStep == 0)
	{
		// 즉시 타이머 만료
		demonTimer.SetTargetTime(0.01f);
		demonTimer.Reset();
	}
}
