#pragma once

#include <Level/Level.h>
#include <string>

// 소코반 게임 레벨 클래스.
// 게임 클리어 등 게임 규칙 및 전반을 관리.
// 소코반 -> 박스를 정해진 위치에 모두 옮기는 것이 클리어 규칙.
class GameLevel : public Craft::Level
{

	TYPE_DECLARATIONS(GameLevel, Level)

public:
	// 플레이어가 이동하려는 위치가 이동 가능한지를 판단해주는 함수.
	bool CanMove(
		const Craft::Vector2& playerPosition,
		const Craft::Vector2& nextPosition
	);

private:
	// 레벨 초기화 함수.
	virtual void OnInitialized() override;

	// Draw 이벤트 함수.
	virtual void Draw() override;

	// 맵 로드 함수.
	void LoadMap(const std::string& filename);

private:
	// 목표 점수 - 클리어 조건.
	int targetScore = 0;

	// 게임 클리어 여부 플래그.
	bool isGameClear = false;

};

