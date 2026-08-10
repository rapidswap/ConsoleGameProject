#include "GameLevel.h"
#include <Actor/Player.h>

void GameLevel::OnInitialized()
{
	Level::OnInitialized();

	// 플레이어 액터 추가.
	SpawnActor<Player>();
}