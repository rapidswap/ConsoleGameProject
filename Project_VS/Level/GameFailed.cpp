#include "GameFailed.h"
#include <Input/Input.h>
#include <Render/Renderer.h>
#include <Engine/Engine.h>
#include <Level/MainMenuLevel.h>

using namespace Craft;

void GameFailed::Tick(float deltaTime)
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
			Engine::Get().AddNewLevel<MainMenuLevel>();
		}
		else if (selectedMenuIndex == 1)
		{
			Engine::Get().Quit();
		}
	}
}

void GameFailed::Draw()
{
	super::Draw();

	int screenWidth = Engine::Get().GetWidth();
	int screenHeight = Engine::Get().GetHeight();

	// 타이틀 출력 (YOU DIED 아스키 아트)
	std::string titleArt[4] = {
		" __   __ ___  _   _    ___   ___  ___  ___  ",
		" \\ \\ / // _ \\| | | |  |   \\ |_ _|| __||   \\ ",
		"  \\ V /| (_) | |_| |  | |) | | | | _| | |) |",
		"   |_|  \\___/ \\___/   |___/ |___||___||___/ "
	};

	int artHeight = 4;
	int artWidth = titleArt[0].length();
	int startY = screenHeight / 2 - 10;

	for (int i = 0; i < artHeight; ++i)
	{
		Renderer::Get().Submit(
			titleArt[i],
			Vector2((screenWidth / 2) - (artWidth / 2), startY + i),
			Color::Red,
			100
		);
	}

	// 최종 플레이 타임 출력
	int minutes = static_cast<int>(finalPlayTime) / 60;
	int seconds = static_cast<int>(finalPlayTime) % 60;
	char timeBuf[32];
	sprintf_s(timeBuf, "Survived Time: %02d:%02d", minutes, seconds);
	
	std::string timeStr = timeBuf;
	Renderer::Get().Submit(
		timeStr,
		Vector2((screenWidth / 2) - (timeStr.length() / 2),
			screenHeight / 2 - 3),
		Color::Yellow,
		100
	);

	std::string menus[2] = { "Go to Lobby","Exit Game" };

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
