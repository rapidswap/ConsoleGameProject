#define WIN32_LEAN_AND_MEAN
#include <WinSock2.h>
#include "Network/NetworkManager.h"
#include <Common/Protocol.h>
#include "GameClearLevel.h"
#include <Engine/Engine.h>
#include <Game/Game.h>
#include <Render/Renderer.h>
#include <Input/Input.h>
#include <cassert>
#include <string>

using namespace Craft;

void GameClearLevel::SetSinglePlayerSpend(int spend)
{
	s_isMultiplayer = false;
	s_singlePlayerSpend = spend;
	s_multiplayerRecords.clear();
}

void GameClearLevel::SetMultiplayerRecords(int count, const ClearRecord* records)
{
	s_isMultiplayer = true;
	s_multiplayerRecords.clear();
	for (int i = 0; i < count; ++i)
	{
		ClearPlayerRecord r;
		r.playerId = records[i].playerId;
		r.playerName = records[i].playerName;
		r.totalGoldSpent = records[i].totalGoldSpent;
		r.rank = records[i].rank;
		s_multiplayerRecords.push_back(r);
	}
}

GameClearLevel::GameClearLevel()
{
	// 메뉴 아이템 생성.
	itemList.emplace_back(
		std::make_unique<MenuItem>(
			"Restart Game",
			[]()
			{
				Game& game = dynamic_cast<Game&>(Engine::Get());
				if (NetworkManager::Get()->IsConnected())
				{
					// 멀티플레이: 대기실로 돌아가서 다시 전원 레디 후 동시 시작
					game.ToggleMenu(State::MAINMENU);
				}
				else
				{
					game.RestartDefenseLevel();
					game.ToggleMenu(State::GAMEPLAY);
				}
			}
		)
	);

	itemList.emplace_back(
		std::make_unique<MenuItem>(
			"Quit to Main Menu",
			[]()
			{
				Game& game = dynamic_cast<Game&>(Engine::Get());
				game.ToggleMenu(State::MAINMENU);
			}
		)
	);
}

void GameClearLevel::Tick(float deltaTime)
{
	if (NetworkManager::Get()->IsConnected())
	{
		NetworkManager::Get()->Update();
	}

	const int length = static_cast<int>(itemList.size());
	if (Input::Get().GetKeyDown(VK_UP))
	{
		selectedMenuIndex = (selectedMenuIndex - 1 + length) % length;
	}

	if (Input::Get().GetKeyDown(VK_DOWN))
	{
		selectedMenuIndex = (selectedMenuIndex + 1) % length;
	}

	// 엔터 입력 처리 -> 현재 선택된 메뉴의 로직 실행.
	if (Input::Get().GetKeyDown(VK_RETURN))
	{
		assert(selectedMenuIndex >= 0
			&& selectedMenuIndex < (int)itemList.size()
			&& itemList[selectedMenuIndex]->onSelected
		);

		itemList[selectedMenuIndex]->onSelected();
	}
}

void GameClearLevel::Draw()
{
	int screenWidth = Engine::Get().GetWidth();
	int screenHeight = Engine::Get().GetHeight();

	auto drawCentered = [&](const std::string& text, int y, Color color)
	{
		int x = (screenWidth / 2) - (static_cast<int>(text.length()) / 2);
		Renderer::Get().Submit(text, Vector2(x, y), color, 100);
	};

	// 전체 블록 높이 계산하여 수직 중앙 정렬
	int totalLines = s_isMultiplayer
		? (13 + static_cast<int>(s_multiplayerRecords.size()))
		: 14;

	int currentY = (screenHeight / 2) - (totalLines / 2);
	if (currentY < 1) currentY = 1;

	// 1. 헤더 배너
	drawCentered("============================================================", currentY++, Color::Yellow);
	drawCentered("                 * * *  GAME CLEAR!  * * *                  ", currentY++, Color::Yellow);
	drawCentered("              ALL WAVES DEFENDED SUCCESSFULLY!              ", currentY++, Color::Cyan);
	drawCentered("============================================================", currentY++, Color::Yellow);
	currentY++; // 한 줄 여백

	// 2. 모드별 통계 / 랭킹
	if (s_isMultiplayer)
	{
		drawCentered("[ MULTIPLAYER MATCH RESULTS ]", currentY++, Color::BrightWhite);
		drawCentered("------------------------------------------------------------", currentY++, Color::DarkGray);
		drawCentered(" RANK   PLAYER                 TOTAL GOLD SPENT   RESULT    ", currentY++, Color::Cyan);
		drawCentered("------------------------------------------------------------", currentY++, Color::DarkGray);

		uint32_t myId = NetworkManager::Get()->GetMyPlayerId();

		if (s_multiplayerRecords.empty())
		{
			drawCentered("No player data received.", currentY++, Color::Gray);
		}
		else
		{
			for (const auto& rec : s_multiplayerRecords)
			{
				bool isMe = (rec.playerId == myId);
				std::string rankStr = (rec.rank == 1) ? "1st *" : (rec.rank == 2) ? "2nd  " : (std::to_string(rec.rank) + "th ");
				std::string badge = (rec.rank == 1) ? "MVP *" : "Runner-Up";

				char nameBuf[32];
				if (isMe)
				{
					sprintf_s(nameBuf, "Player %u (YOU)", rec.playerId);
				}
				else
				{
					sprintf_s(nameBuf, "Player %u", rec.playerId);
				}

				char rowBuf[128];
				sprintf_s(rowBuf, " %-6s %-22s %7d G   %-10s",
					rankStr.c_str(), nameBuf, rec.totalGoldSpent, badge.c_str());

				Color rowColor = (rec.rank == 1) ? Color::Yellow : (isMe ? Color::Green : Color::White);
				drawCentered(rowBuf, currentY++, rowColor);
			}
		}

		drawCentered("------------------------------------------------------------", currentY++, Color::DarkGray);
		drawCentered("* 1st Place awarded to the highest spender in battle!", currentY++, Color::DarkGray);
	}
	else
	{
		drawCentered("[ SINGLE PLAYER STATS ]", currentY++, Color::BrightWhite);
		drawCentered("------------------------------------------------------------", currentY++, Color::DarkGray);

		char goldBuf[128];
		sprintf_s(goldBuf, "Total Gold Spent : %d G", s_singlePlayerSpend);
		drawCentered(goldBuf, currentY++, Color::Green);

		// 지출 평가 문구
		std::string eval;
		if (s_singlePlayerSpend < 1000)
		{
			eval = "Evaluation: Master Tactician (Extreme Efficiency!)";
		}
		else if (s_singlePlayerSpend < 3000)
		{
			eval = "Evaluation: Veteran Commander (Balanced Spending!)";
		}
		else
		{
			eval = "Evaluation: Tycoon Defender (Overwhelming Gold Power!)";
		}
		drawCentered(eval, currentY++, Color::Cyan);
		drawCentered("------------------------------------------------------------", currentY++, Color::DarkGray);
	}

	currentY++; // 한 줄 여백

	// 3. 인터랙티브 메뉴
	const int count = static_cast<int>(itemList.size());
	for (int ix = 0; ix < count; ++ix)
	{
		bool isSelected = (ix == selectedMenuIndex);
		Color textColor = isSelected ? selectedColor : unSelectedColor;

		std::string prefix = isSelected ? ">  " : "   ";
		std::string suffix = isSelected ? "  <" : "   ";
		std::string displayText = prefix + itemList[ix]->text + suffix;

		drawCentered(displayText, currentY++, textColor);
	}
}
