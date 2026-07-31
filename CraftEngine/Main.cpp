#include <Engine/Engine.h>
#include <iostream>
#include <Level/TestLevel.h>
//using namespace Craft;

int main()
{
	// 엔진 객체 생성 및 실행.
	Craft::Engine engine;
	engine.AddNewLevel<TestLevel>();
	engine.Run();
}