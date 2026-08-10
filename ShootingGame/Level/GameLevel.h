#pragma once

#include <Level/Level.h>

class GameLevel:public Craft::Level
{
private:
	// 초기화 이벤트 함수 오버라이드
	virtual void OnInitialized() override;
};

