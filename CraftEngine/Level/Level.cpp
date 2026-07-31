#include "Level.h"

namespace Craft
{
	Level::Level()
	{
	}

	Level::~Level()
	{
	}

	void Level::OnInitialized()
	{
		// 초기화 됐다고 설정
		hasInitialized = true;
	}

	void Level::BeginPlay()
	{
	}

	void Level::Tick(float deltaTime)
	{
	}

	void Level::Draw()
	{
	}
}