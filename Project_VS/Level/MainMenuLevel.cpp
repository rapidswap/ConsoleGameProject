#include "MainMenuLevel.h"
#include <Input/Input.h>
#include <Engine/Engine.h>
#include <Level/GameLevel.h>
#include <Render/Renderer.h>


using namespace Craft;
void MainMenuLevel::Tick(float deltaTime)
{
	super::Tick(deltaTime);

	// 위 방향키
	if (Input::Get().GetKeyDown(VK_UP))
	{
		selectedMenuIndex--;
		if (selectedMenuIndex < 0) selectedMenuIndex = 1;
	}

	// 아래 방향키
	if (Input::Get().GetKeyDown(VK_DOWN))
	{
		selectedMenuIndex++;
		if (selectedMenuIndex > 1)selectedMenuIndex = 0;
	}
	
	// 엔터키 확정.
	if (Input::Get().GetKeyDown(VK_RETURN))
	{
		if (selectedMenuIndex == 0)
		{
			Engine::Get().AddNewLevel<GameLevel>();
		}
		else if (selectedMenuIndex == 1)
		{
			Engine::Get().Quit();
		}
	}
}

void MainMenuLevel::Draw()
{
	super::Draw();

	int screenWidth = Engine::Get().GetWidth();
	int screenHeight = Engine::Get().GetHeight();

	// 타이틀 출력 (스피드 액션 스타일)
	std::string titleArt[4] = {
		" ___  ___  __    _ ___ ___ _____   __   __ ___ ",
		"| _ \\| _ \\/  \\  | | __/ __|_   _|  \\ \\ / // __|",
		"|  _/|   / () |_| | _| (__  | |     \\ V / \\__ \\",
		"|_|  |_|_\\\\__/\\___/___\\___| |_|      \\_/  |___/"
	};

	int artHeight = 4;
	int artWidth = titleArt[0].length();
	int startY = screenHeight / 2 - 10;

	for (int i = 0; i < artHeight; ++i)
	{
		Renderer::Get().Submit(
			titleArt[i],
			Vector2((screenWidth / 2) - (artWidth / 2), startY + i),
			Color::Yellow,
			100
		);
	}

	
	std::string menus[2] = { "Game Start", "Exit Game" };

	// 메뉴 그리기.
	for (int i = 0;i < 2;++i)
	{
		Color textColor = Color::White;

		// 선택된 메뉴는 초록색.
		if (i == selectedMenuIndex)
		{
			textColor = Color::Green;
			menus[i] = "-[ " + menus[i] + " ]-";
		}

		Renderer::Get().Submit(
			menus[i],
			Vector2((screenWidth / 2) - (menus[i].length() / 2),
				screenHeight / 2 + (i * 2)),
			textColor,
			100
		);
	}
}
