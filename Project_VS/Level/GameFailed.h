#pragma once

#include <Level/Level.h>

class GameFailed:public Craft::Level
{
	TYPE_DECLARATIONS(GameFailed, Level)
private:
	virtual void Tick(float deltaTime) override;
	virtual void Draw() override;

private:
	int selectedMenuIndex = 0;

};

