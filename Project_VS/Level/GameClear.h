#pragma once

#include <Level/Level.h>

class GameClear:public Craft::Level
{

	TYPE_DECLARATIONS(GameClear, Level)

public:
	GameClear() : finalPlayTime(0.0f) {}
	GameClear(float playTime) : finalPlayTime(playTime) {}

private:

	virtual void Tick(float deltaTime) override;
	virtual void Draw() override;

private:
	int selectedMenuIndex = 0;
	float finalPlayTime = 0.0f;
};

