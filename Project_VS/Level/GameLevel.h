#pragma once

#include <Level/Level.h>
#include <vector>
#include <functional>
#include <string>

class GameLevel:public Craft::Level
{
	struct Augment
	{
		std::string name;
		std::string description;
		std::function<void()> onSelected;
		std::function<bool()> canShow;
	};

private:
	// 초기화 이벤트 함수 오버라이드
	virtual void OnInitialized() override;
	virtual void Tick(float deltaTime) override;
	virtual void Draw() override;

public:
	// 외부(Player)에서 피격당했을 때 호출할 함수.
	void TakeDamage();

	// 플레이어가 레벨업했을 때 메뉴를 띄우고 게임을 프리즈시키는 함수.
	// 여러 번 연속으로 띄워야 할 경우 times 인자에 횟수를 넘깁니다 (기본값 1).
	void ShowLevelUpMenu(int times = 1);

	// 보스 등장 시 모든 일반 적과 중간 보스를 소멸시키는 함수.
	void WipeOutEnemies();

private:
	// 게임 오버 여부를 확인하는 함수.
	bool CheckGameFailed();

private:

	// 레벨업 메뉴가 켜져서 게임이 멈춰 있는지 여부
	bool isLevelUpMenuOpen = false;

	// 전체 증강의 종류를 담아둘 목록.
	std::vector<Augment> augmentList;

	// 이번 레벨업 시 랜덤으로 뽑혀 화면에 나타날 선택지.
	std::vector<Augment> currentChoices;
	
	// 현재 방향키로 선택중인 증강의 인덱스 번호.
	int seletedAugmentIndex = 0;

	// 추가로 선택해야 할 증강의 남은 횟수 (연속 선택을 위함).
	int pendingAugmentCount = 0;
};