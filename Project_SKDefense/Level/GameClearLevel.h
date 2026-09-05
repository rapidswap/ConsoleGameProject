#pragma once
 
#include <Level/Level.h>
#include "MenuItem.h"
#include <vector>
#include <string>

struct ClearRecord;

struct ClearPlayerRecord
{
	uint32_t playerId = 0;
	std::string playerName;
	int32_t totalGoldSpent = 0;
	int32_t rank = 1;
};

class GameClearLevel : public Craft::Level
{
	TYPE_DECLARATIONS(GameClearLevel, Level)

public:
	GameClearLevel();

	static void SetSinglePlayerSpend(int spend);
	static void SetMultiplayerRecords(int count, const ClearRecord* records);

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

	// 정산 데이터 저장용 정적 멤버
	static inline bool s_isMultiplayer = false;
	static inline int s_singlePlayerSpend = 0;
	static inline std::vector<ClearPlayerRecord> s_multiplayerRecords;
};

