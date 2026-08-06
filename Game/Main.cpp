#include <Engine/Engine.h>
//#include <Level/TestLevel.h>
#include <Level/GameLevel.h>
int main()
{
	Craft::Engine engine;
	//engine.AddNewLevel<TestLevel>();
	engine.AddNewLevel<GameLevel>();
	engine.Run();
}