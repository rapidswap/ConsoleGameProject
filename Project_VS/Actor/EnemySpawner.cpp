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

		minSpawnTime -= 0.002f;
		maxSpawnTime -= 0.002f;

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
	// 적 생성 처리.

	// 적 이미지 배열의 길이 확인.
	const int length = sizeof(enemyType) / sizeof(enemyType[0]);

	// 랜덤 인덱스.
	const int index = Util::RandomRange(0, length - 1);

	// 뱀파이어 서바이벌 방식: 화면 가장자리 랜덤 스폰.
	int screenWidth = Engine::Get().GetWidth();
	int screenHeight = Engine::Get().GetHeight();
	
	// 외곽선 4면(0:상, 1:하, 2:좌, 3:우) 중 하나 선택
	int edge = Util::RandomRange(0, 3);
	float spawnX = 0.0f;
	float spawnY = 0.0f;

	if (edge == 0) // 상단
	{
		spawnX = (float)Util::RandomRange(0, screenWidth - 1);
		spawnY = 0.0f;
	}
	else if (edge == 1) // 하단
	{
		spawnX = (float)Util::RandomRange(0, screenWidth - 1);
		spawnY = (float)(screenHeight - 1);
	}
	else if (edge == 2) // 좌측
	{
		spawnX = 0.0f;
		spawnY = (float)Util::RandomRange(0, screenHeight - 1);
	}
	else // 우측
	{
		spawnX = (float)(screenWidth - 1);
		spawnY = (float)Util::RandomRange(0, screenHeight - 1);
	}

	// 적 액터 생성.
	std::shared_ptr<Level> owner = GetOwner();
	if (owner)
	{
		owner->SpawnActor<Enemy>(enemyType[index], spawnX, spawnY);
	}
}

void EnemySpawner::SpawnElite()
{
	int screenWidth = Engine::Get().GetWidth();
	int screenHeight = Engine::Get().GetHeight();
	
	int edge = Util::RandomRange(0, 3);
	float spawnX = 0.0f;
	float spawnY = 0.0f;

	if (edge == 0) // 상단
	{
		spawnX = (float)Util::RandomRange(0, screenWidth - 1);
		spawnY = 0.0f;
	}
	else if (edge == 1) // 하단
	{
		spawnX = (float)Util::RandomRange(0, screenWidth - 1);
		spawnY = (float)(screenHeight - 1);
	}
	else if (edge == 2) // 좌측
	{
		spawnX = 0.0f;
		spawnY = (float)Util::RandomRange(0, screenHeight - 1);
	}
	else // 우측
	{
		spawnX = (float)(screenWidth - 1);
		spawnY = (float)Util::RandomRange(0, screenHeight - 1);
	}

	std::shared_ptr<Level> owner = GetOwner();
	if (owner)
	{
		owner->SpawnActor<EliteBoss>(Craft::Vector2((int)spawnX, (int)spawnY));
	}
}

void EnemySpawner::SpawnDemon()
{
	int screenWidth = Engine::Get().GetWidth();
	int screenHeight = Engine::Get().GetHeight();

	int edge = Util::RandomRange(0, 3);
	float spawnX = static_cast<float>(screenWidth) / 2;
	float spawnY = static_cast<float>(screenHeight) / 2;

	

	std::shared_ptr<Level> owner = GetOwner();
	if (owner)
	{
		owner->SpawnActor<Demon>(Craft::Vector2((int)spawnX, (int)spawnY));
	}
}

