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
	timer.SetTargetTime(1.0f);
}

void EnemySpawner::Tick(float deltaTime)
{
	super::Tick(deltaTime);

	// 타이머 업데이트.
	timer.Tick(deltaTime);

	// 경과 시간 확인.
	if (!timer.IsTimeOut())
	{
		return;
	}

	// 타이머 초기화.
	timer.Reset();

	// 적 생성.
	SpawnEnemy();
}

void EnemySpawner::SpawnEnemy()
{
	// 적 생성 처리.

	// 적 이미지 배열의 길이 확인.
	//const int length = sizeof(enemyType) / sizeof(enemyType[0]);

	// 랜덤 인덱스.
	//const int index = Util::RandomRange(0, length - 1);

	// 생성 y 위치(랜덤)
	//int yPosition = Util::RandomRange(1, 10);

	// 적 액터 생성.
	auto defenseLevel = Craft::Cast<DefenseLevel>(GetOwner());
	if (defenseLevel)
	{
		defenseLevel->SpawnActor<Enemy>(defenseLevel->GetSpawnPoint());
	}
}
