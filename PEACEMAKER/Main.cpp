#include <Engine/Engine.h>
#include <Level/GameLevel.h>
#include <Level/MainMenuLevel.h>
#include <Windows.h>

int main()
{
	// 콘솔 창 제목을 PEACEMAKER로 설정
	SetConsoleTitleA("PEACEMAKER");

	// 엔진 객체 생성 및 실행.
	Craft::Engine engine;
	//engine.AddNewLevel<GameLevel>();
	engine.AddNewLevel<MainMenuLevel>();
	engine.Run();
}