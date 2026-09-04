#pragma once

#include <Level/Level.h>

class MainMenuLevel:public Craft::Level
{
	TYPE_DECLARATIONS(MainMenuLevel, Level)

public:
	void ResetReady() { isReady = false; }

private:
	virtual void OnInitialized() override;
	virtual void Tick(float deltaTime) override;
	virtual void Draw() override;

private:
	int selectedMenuIndex = 0;

	// 타이틀 색상 변경용 타이머
	float colorTimer = 0.0f;
	int currentColorIndex = 0;

	// 레디 상태 여부
	bool isReady = false;
};

