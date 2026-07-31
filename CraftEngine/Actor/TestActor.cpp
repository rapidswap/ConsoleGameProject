#include "TestActor.h"
#include <iostream>
#include <Windows.h>
using namespace Craft;
void TestActor::Tick(float deltaTime)
{
	// 상위 로직 호출.
	Actor::Tick(deltaTime);

	//Todo: ESC 키 종료 (임시처리).
	if ((GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0)
	{
		// 엔진 종료.
		QuitGame();
	}

	std::cout << "TestActor::Tick() - deltaTime: " << deltaTime
		<< " | FPS: " << (1.0f / deltaTime) << "\n";
}
