#include "Game.h"
#include <Level/DefenseLevel.h>
#include <Level/MainMenuLevel.h>
#include <Level/GameOverLevel.h>
#include <Level/GameClearLevel.h>
#include <Level/EscMenu.h>

Game::Game()
{
	// 두 레벨 생성 및 배열에 추가.
	levelList.emplace_back(std::make_shared<MainMenuLevel>());
	levelList.emplace_back(std::make_shared<DefenseLevel>());
	levelList.emplace_back(std::make_shared<EscMenu>()); // 2: ESCMENU
	// Todo: GameOver / Clear 메뉴 구현.
	levelList.emplace_back(std::make_shared<GameOverLevel>()); // 3: GAMEOVER 
	levelList.emplace_back(std::make_shared<GameClearLevel>()); // 4: GAMECLEAR

	// 시작 상태 설정.
	state = State::MAINMENU;

	// 게임 시작시 활성화할 레벨 설정.
	mainLevel = levelList[(int)state];
}

void Game::ToggleMenu(State nextMenu)
{
	// 레벨 설정 및 상태 값 업데이트.
	state = nextMenu;
	mainLevel = levelList[int(nextMenu)];

	// 메인 메뉴로 돌아갈 때는 항상 레디 상태를 초기화!
	if (nextMenu == State::MAINMENU)
	{
		if (auto mainMenu = std::dynamic_pointer_cast<MainMenuLevel>(levelList[0]))
		{
			mainMenu->ResetReady();
		}
	}
}

void Game::RestartDefenseLevel()
{
	// 1번 인덱스가 DefenseLevel이므로 새로 할당하여 교체
	levelList[1] = std::make_shared<DefenseLevel>();
	
	// 엔진의 nextLevel 포인터에 집어넣어, 다음 프레임에 OnInitialized()와 BeginPlay()가 호출되도록 유도
	nextLevel = levelList[1];
}

