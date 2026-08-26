#pragma once

#include <Engine/Engine.h>
#include <Input/Input.h>

#include <vector>

// 레벨 관리에 사용할 상태 열거형.
enum class State
{
	MAINMENU,
	GAMEPLAY,
	ESCMENU,
	GAMEOVER,
	GAMECLEAR,
	Length
}; 


class Game:public Craft::Engine
{
public:
	Game();
	~Game() = default;

	// 메뉴/게임 레벨을 전환하는 함수.
	void ToggleMenu(State nextMenu);

	// 디펜스 레벨을 초기화(재시작)하는 함수.
	void RestartDefenseLevel();

private:
	// 메뉴 레벨과 게임 레벨을 관리할 배열.
	std::vector<std::shared_ptr<Craft::Level>> levelList;

	// 현재 활성화된 레벨의 상태를 나타내는 변수.
	State state = State::MAINMENU;
};

