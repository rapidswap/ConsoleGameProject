#include <Engine/Engine.h>
#include <Level/DefenseLevel.h>
#include <Game/Game.h>
#include <Windows.h>


int main()
{
	// 콘솔 창 제목을 SK Defense로 설정
	SetConsoleTitleA("SK Defense");

	// 엔진 객체 생성 및 실행.
	Game game;
	game.Run();
}
