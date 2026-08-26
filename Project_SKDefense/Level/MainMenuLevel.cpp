#include "MainMenuLevel.h"
#include <Input/Input.h>
#include <Game/Game.h>
#include <Render/Renderer.h>

using namespace Craft;
void MainMenuLevel::Tick(float deltaTime)
{
	super::Tick(deltaTime);

	// 엔터키를 누르면 게임 시작.
	if (Input::Get().GetKeyDown(VK_RETURN))
	{
		Game& game = dynamic_cast<Game&>(Engine::Get());
		game.ToggleMenu(State::GAMEPLAY);
	}

	// 1초마다 색상 인덱스 변경
	colorTimer += deltaTime;
	if (colorTimer >= 1.0f)
	{
		colorTimer = 0.0f;
		currentColorIndex++;
	}
}

void MainMenuLevel::Draw()
{
	super::Draw();

	int screenWidth = Engine::Get().GetWidth();
	int screenHeight = Engine::Get().GetHeight();

	// 타이틀 출력 (SKDefense)
	std::string titleArt[4] = {
		"  ___  _  __  ___       __                    ",
		" / __|| |/ / |   \\ ___ / _|___ _ _  ___ ___ ",
		" \\__ \\| ' <  | |) / -_)  _/ -_) ' \\(_-</ -_)",
		" |___/|_|\\_\\ |___/\\___|_| \\___|_||_/__/\\___|"
	};

	// 사용 가능한 색상 배열
	Color titleColors[] = {
		Color::Yellow, Color::Cyan, Color::Purple, Color::Green, Color::BrightWhite, Color::Magenta
	};
	int colorCount = sizeof(titleColors) / sizeof(titleColors[0]);
	Color currentColor = titleColors[currentColorIndex % colorCount];

	int artHeight = 4;
	int artWidth = titleArt[0].length();
	int startY = screenHeight / 2 - 10;

	for (int i = 0; i < artHeight; ++i)
	{
		Renderer::Get().Submit(
			titleArt[i],
			Vector2((screenWidth / 2) - (artWidth / 2), startY + i),
			currentColor,
			100
		);
	}

	Renderer::Get().Submit(
		"Press Enter Button."
		,Vector2((screenWidth / 2) - 10, (screenHeight / 2) + 5)
		, Color::Green, 100);
	
}





