#include "EnemySpawner.h"
#include <Util/Util.h>
#include <Actor/Enemy.h>
#include <Actor/EliteBoss.h>
#include <Level/Level.h>
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
	
	// 보스 스폰 타이머 설정 (180초 = 3분)
	eliteBossTimer.SetTargetTime(10.0f);

}

void EnemySpawner::Tick(float deltaTime)
{
	super::Tick(deltaTime);

	// 타이머 업데이트.
	timer.Tick(deltaTime);
	diffcultyTimer.Tick(deltaTime);
	eliteBossTimer.Tick(deltaTime);
	
	// 보스 스폰 체크
	if (eliteBossTimer.IsTimeOut())
	{
		eliteBossTimer.Reset();
		eliteBossTimer.SetTargetTime(180.0f);
		SpawnBoss();
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

void EnemySpawner::SpawnBoss()
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

