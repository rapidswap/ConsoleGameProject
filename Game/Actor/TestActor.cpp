#include "TestActor.h"
#include <Input/Input.h>
#include <iostream>
#include <Windows.h>

using namespace Craft;
void TestActor::Tick(float deltaTime)
{
	// 상위 로직 호출.
	Actor::Tick(deltaTime);

	// ESC 키 종료.
	if (Input::Get().GetKeyDown(VK_ESCAPE))
	{
		// 엔진 종료.
		QuitGame();
	}

	if (Input::Get().GetKeyDown('A'))
	{
		std::cout << "A key is down\n";
	}

	if (Input::Get().GetKey('A'))
	{
		std::cout << "A key is holding down\n";
	}

	if (Input::Get().GetKeyUp('A'))
	{
		std::cout << "A key is up\n";
	}

	//std::cout
	//	<< "TestActor::Tick() - deltaTime: " 
	//	<< deltaTime
	//	<< " | FPS: " << (1.0f / deltaTime) 
	//	<< "\n";
}