//#include <Engine/Engine.h>
//#include <Level/TestLevel.h>
#include <Game/Game.h>
#include <Level/GameLevel.h>
int main()
{
	//Craft::Engine engine;
	////engine.AddNewLevel<TestLevel>();
	//engine.AddNew
	//Level<GameLevel>();
	//engine.Run();

	// 창 제목 설정.
	SetConsoleTitleA("Sokoban Game");

	Game game;
	game.Run();
}