#pragma once

#include <Level/Level.h>

class MainMenuLevel:public Craft::Level
{
	TYPE_DECLARATIONS(MainMenuLevel,Level)
public:
	virtual void Tick(float deltaTime) override;
	virtual void Draw() override;


private:
	// 메뉴 선택 인덱스.
	int selectedMenuIndex = 0;
};

