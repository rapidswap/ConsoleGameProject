#include <Engine/Engine.h>
#include <Level/DefenseLevel.h>
#include <Windows.h>

int main()
{
	// 콘솔 창 제목을 SK Defense로 설정
	SetConsoleTitleA("SK Defense");

	// 엔진 객체 생성 및 실행.
	Craft::Engine engine;
	engine.AddNewLevel<DefenseLevel>();
	engine.Run();
}
