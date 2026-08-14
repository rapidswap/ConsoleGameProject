#pragma once

#include <Level/Level.h>

class GameFailed:public Craft::Level
{
	TYPE_DECLARATIONS(GameFailed, Level)
private:
	virtual void Tick(float deltaTime) override;
	virtual void Draw() override;

public:
	GameFailed() : finalPlayTime(0.0f) {}
	GameFailed(float playTime) : finalPlayTime(playTime) {}

private:
	int selectedMenuIndex = 0;
	float finalPlayTime = 0.0f;

};

