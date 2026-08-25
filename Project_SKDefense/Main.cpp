#include <Engine/Engine.h>
#include <Level/DefenseLevel.h>
#include <Windows.h>

int main()
{
	// 콘솔 창 제목을 SK Defense로 설정
	SetConsoleTitleA("SK Defense");

	// 콘솔 설정: 빠른 편집 모드(드래그 시 게임 멈춤 현상) 끄기 및 마우스 입력 활성화
	HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
	DWORD mode;
	GetConsoleMode(hInput, &mode);
	mode &= ~ENABLE_QUICK_EDIT_MODE;
	mode |= ENABLE_EXTENDED_FLAGS | ENABLE_MOUSE_INPUT;
	SetConsoleMode(hInput, mode);

	// 엔진 객체 생성 및 실행.
	Craft::Engine engine;
	engine.AddNewLevel<DefenseLevel>();
	engine.Run();
}
