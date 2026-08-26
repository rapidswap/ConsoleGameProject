#pragma once
#include <Level/Level.h>

#include "MenuItem.h"

class EscMenu:public Craft::Level
{
	TYPE_DECLARATIONS(EscMenu,Level)

public:
	EscMenu();
private:
	virtual void Tick(float deltaTime) override;
	virtual void Draw() override;

private:
	// 현재 활성화된 메뉴 아이템 인덱스.
	int selectedMenuIndex = 0;

	// 선택된 메뉴 아이템의 색상.
	Craft::Color selectedColor = Craft::Color::Green;

	// 미 선택된 메뉴 아이템의 색상.
	Craft::Color unSelectedColor = Craft::Color::White;

	// 메뉴 아이템 배열
	std::vector<std::unique_ptr<MenuItem>> itemList;


};

