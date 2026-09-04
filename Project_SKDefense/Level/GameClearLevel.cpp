#define WIN32_LEAN_AND_MEAN
#include <WinSock2.h>
#include "Network/NetworkManager.h"
#include "GameClearLevel.h"
#include <Engine/Engine.h>
#include <Game/Game.h>
#include <Render/Renderer.h>
#include <cassert>

using namespace Craft;
GameClearLevel::GameClearLevel()
{
	// 메뉴 아이템 생성.
	// 게임으로 돌아가기.
	itemList.emplace_back(
		std::make_unique<MenuItem>(
			"Restart Game",
			[]()
			{
				// 메뉴 토글 함수 호출.
				Game& game = dynamic_cast<Game&>(Engine::Get());
				if (NetworkManager::Get()->IsConnected())
				{
					game.ToggleMenu(State::MAINMENU);
				}
				else
				{
					game.RestartDefenseLevel();
					game.ToggleMenu(State::GAMEPLAY);
				}
			}
		)
	);

	itemList.emplace_back(
		std::make_unique<MenuItem>(
			"Quit Game",
			[]()
			{
				// 메뉴 토글 함수 호출.
				Game& game = dynamic_cast<Game&>(Engine::Get());
				game.ToggleMenu(State::MAINMENU);
			}
		)
	);
}

void GameClearLevel::Tick(float deltaTime)
{
	// 배열의 요소 개수.
	const int length = static_cast<int>(itemList.size());
	if (Input::Get().GetKeyDown(VK_UP))
	{
		// 인덱스 돌리기 (-방향).
		selectedMenuIndex = (selectedMenuIndex - 1 + length) % length;
	}

	if (Input::Get().GetKeyDown(VK_DOWN))
	{
		// 인덱스 돌리기 (+방향).
		selectedMenuIndex = (selectedMenuIndex + 1) % length;
	}

	// 엔터 입력 처리 -> 현재 선택된 메뉴의 로직 실행.
	if (Input::Get().GetKeyDown(VK_RETURN))
	{
		// 어써트.
		assert(selectedMenuIndex >= 0
			&& selectedMenuIndex < (int)itemList.size()
			&& itemList[selectedMenuIndex]->onSelected
		);

		// 메뉴 아이템에 저장된 로직 실행.
		itemList[selectedMenuIndex]->onSelected();
	}
}

void GameClearLevel::Draw()
{
	// 메뉴 아이템 그리기.
	int screenWidth = Engine::Get().GetWidth();
	int screenHeight = Engine::Get().GetHeight();
	const int count = static_cast<int>(itemList.size());
	for (int ix = 0; ix < count; ++ix)
	{
		// 선택/미선택된 아이템 색상 처리.
		Color textColor = (ix == selectedMenuIndex)
			? selectedColor : unSelectedColor;

		// 전체 메뉴 아이템 개수를 기준으로 화면 정중앙(Y축)을 맞추기 위한 시작 Y 좌표 계산
		int startY = (screenHeight / 2) - (count / 2);

		// 아이템 그리기.
		Renderer::Get().Submit(
			itemList[ix]->text,
			Vector2(screenWidth / 2 - (itemList[ix]->text.length() / 2), startY + ix),
			textColor
		);
	}
}
