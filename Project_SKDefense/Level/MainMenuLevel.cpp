#define WIN32_LEAN_AND_MEAN
#include <WinSock2.h>
#include "Network/NetworkManager.h"
#include "MainMenuLevel.h"
#include <Input/Input.h>
#include <Game/Game.h>
#include <Render/Renderer.h>

using namespace Craft;

void MainMenuLevel::OnInitialized()
{
	super::OnInitialized();

	// 메인 메뉴 진입 시 서버 자동 접속 시도 (1회만 수행)
	if (!NetworkManager::Get()->IsConnected())
	{
		if (NetworkManager::Get()->Connect(L"127.0.0.1", 7777))
		{
			C_LOGIN_PACKET loginPkt;
			strcpy_s(loginPkt.playerName, "Player_Game");
			NetworkManager::Get()->Send(reinterpret_cast<BYTE*>(&loginPkt), loginPkt.size);
		}
	}
}

void MainMenuLevel::Tick(float deltaTime)
{
	super::Tick(deltaTime);

	
	// 1. 메인 메뉴에서도 서버 패킷을 계속 수신하여 인원수 및 시작 신호 확인!
	NetworkManager::Get()->Update();

	// 만약 서버에서 게임오버 등으로 대기실 레디 인원이 0으로 리셋되었다면, 내 레디 상태도 자동 해제!
	//if (NetworkManager::Get()->GetReadyPlayerCount() == 0 && isReady)
	//{
	//	isReady = false;
	//}

	// 2. 서버가 S_GAME_START 신호를 보냈다면 양쪽 클라가 동시에 게임 레벨 전환!
	if (NetworkManager::Get()->IsGameStartTriggered())
	{
		NetworkManager::Get()->SetGameStartTriggered(false);
		Game& game = dynamic_cast<Game&>(Engine::Get());
		game.RestartDefenseLevel();
		game.ToggleMenu(State::GAMEPLAY);
		return;
	}
	// ESC 키: 종료
	if (Input::Get().GetKeyDown(VK_ESCAPE))
	{
		Engine::Get().Quit();
	}
	// 엔터 키 입력 처리
	if (Input::Get().GetKeyDown(VK_RETURN))
	{
		if (NetworkManager::Get()->IsConnected())
		{
			// 멀티플레이 모드: 레디 패킷 전송
			if (!isReady)
			{
				isReady = true;
				NetworkManager::Get()->SendReady(false);
			}
			else if (NetworkManager::Get()->GetPlayerCount() <= 1)
			{
				// 혼자 테스트 중일 때 레디 상태에서 엔터를 한 번 더 누르면 단독 시작
				NetworkManager::Get()->SendReady(true);
			}
		}
		else
		{
			// 서버 미실행 시: 오프라인 싱글플레이 즉시 시작
			Game& game = dynamic_cast<Game&>(Engine::Get());
			game.RestartDefenseLevel();
			game.ToggleMenu(State::GAMEPLAY);
		}
	}

	// 1초마다 색상 인덱스 변경
	colorTimer += deltaTime;
	if (colorTimer >= 1.0f)
	{
		colorTimer = 0.0f;
		currentColorIndex++;
	}
}

void MainMenuLevel::Draw()
{
	super::Draw();

	int screenWidth = Engine::Get().GetWidth();
	int screenHeight = Engine::Get().GetHeight();

	// 타이틀 출력 (SKDefense)
	std::string titleArt[4] = {
		"  ___  _  __  ___       __                    ",
		" / __|| |/ / |   \\ ___ / _|___ _ _  ___ ___ ",
		" \\__ \\| ' <  | |) / -_)  _/ -_) ' \\(_-</ -_)",
		" |___/|_|\\_\\ |___/\\___|_| \\___|_||_/__/\\___|"
	};

	// 사용 가능한 색상 배열
	Color titleColors[] = {
		Color::Yellow, Color::Cyan, Color::Purple, Color::Green, Color::BrightWhite, Color::Magenta
	};
	int colorCount = sizeof(titleColors) / sizeof(titleColors[0]);
	Color currentColor = titleColors[currentColorIndex % colorCount];

	int artHeight = 4;
	int artWidth = titleArt[0].length();
	int startY = screenHeight / 2 - 10;

	for (int i = 0; i < artHeight; ++i)
	{
		Renderer::Get().Submit(
			titleArt[i],
			Vector2((screenWidth / 2) - (artWidth / 2), startY + i),
			currentColor,
			100
		);
	}

	//Renderer::Get().Submit(
	//	"Press Enter Button."
	//	,Vector2((screenWidth / 2) - 10, (screenHeight / 2) + 5)
	//	, Color::Green, 100);


	if (NetworkManager::Get()->IsConnected())
	{
		int total = NetworkManager::Get()->GetPlayerCount();
		int ready = NetworkManager::Get()->GetReadyPlayerCount();

		char lobbyStr[128];
		sprintf_s(lobbyStr, "[ MULTIPLAYER LOBBY ] Players: %d / 2 (Ready: %d)", total, ready);
		std::string lobbyString = lobbyStr;
		Renderer::Get().Submit(lobbyString, Vector2((screenWidth / 2) - ((int)lobbyString.length() / 2), (screenHeight / 2) + 3), Color::Cyan, 100);

		if (!isReady)
		{
			std::string readyText = "Press ENTER to Ready!";
			Renderer::Get().Submit(readyText, Vector2((screenWidth / 2) - ((int)readyText.length() / 2), (screenHeight / 2) + 5), Color::Green, 100);
		}
		else
		{
			if (total >= 2)
			{
				std::string waitText = "[ READY ] Waiting for other player...";
				Renderer::Get().Submit(waitText, Vector2((screenWidth / 2) - ((int)waitText.length() / 2), (screenHeight / 2) + 5), Color::Yellow, 100);
			}
			else
			{
				std::string soloText = "[ READY ] Press ENTER again to Solo Start";
				Renderer::Get().Submit(soloText, Vector2((screenWidth / 2) - ((int)soloText.length() / 2), (screenHeight / 2) + 5), Color::Yellow, 100);
			}
		}
	}
	else
	{
		std::string promptText = "Press Enter Button.";
		Renderer::Get().Submit(promptText, Vector2((screenWidth / 2) - ((int)promptText.length() / 2), (screenHeight / 2) + 5), Color::Yellow, 100);
	}
	
}





