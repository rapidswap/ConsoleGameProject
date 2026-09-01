#include "DefenseLevel.h"
#include <Engine/Engine.h>
#include <Input/Input.h>
#include <Render/Renderer.h>
#include <Game/Game.h>
#include <Actor/Ground.h>
#include <Actor/Wall.h>
#include <Actor/Agit.h>
#include <Actor/Turret.h>
#include <Actor/Enemy.h>
#include <Actor/EnemySpawner.h>
#include<Actor/EnemyHouse.h>
#include <Actor/Agit.h>
#include <Util/Timer.h>
#include <Util/Util.h>
#include <Algorithm/AStar.h>
#include <Algorithm/Node.h>
#include <fstream>
#include <iostream>
#include <cassert>
#include <cmath>
#include <map>
#include <algorithm>
#include <utility>

using namespace Craft;

void DefenseLevel::OnInitialized()
{
	Level::OnInitialized();

	// 파일을 읽어서 맵 로드 (mapWidth, mapHeight가 계산됨).
	LoadMap("SK_Defense_Map.txt");

	// 카메라를 화면 정중앙 좌표로 맞추어, 맵(0,0)이 화면 좌측 상단(0,0)에 오도록 설정.
	// (기존에는 mapWidth/2 여서 맵이 화면 정중앙에 렌더링되어 우측 UI와 겹치는 문제가 있었음)
	int screenWidth = Engine::Get().GetWidth();
	int screenHeight = Engine::Get().GetHeight();
	cameraPosition = Vector2(screenWidth / 2, screenHeight / 2);

	// 적 생성기 액터 추가.
	enemySpawner = SpawnActor<EnemySpawner>();

	// 게임 시작 시 첫 번째로 지어질 터렛 타입 랜덤 결정
	nextTurretType = static_cast<TurretType>(rand() % 3);

}

void DefenseLevel::Tick(float deltaTime)
{
	// 게임 인포 창이 켜져 있으면 일시정지 (F12 키로만 닫기)
	if (isGameInfo)
	{
		if (Input::Get().GetKeyDown(VK_F12))
		{
			isGameInfo = false;
		}
		return;
	}

	// 도박 진행 중이면 게임 정지 (Pause) 상태로 애니메이션만 처리
	if (isGambling)
	{
		gambleTimer += deltaTime;
		// 5초가 지나면 팝업 닫고 보상 지급
		if (gambleTimer >= 5.0f)
		{
			int reward = 10 * gambleResults[0] * gambleResults[1] * gambleResults[2];
			AddGold(reward);
			isGambling = false;
		}
		return; // 아래 게임 로직 무시
	}

	// 부모의 Tick 호출 (배치된 모든 액터들의 Tick 실행)
	Level::Tick(deltaTime);

	// 게임 오버 체크
	auto agit = FindActor<Agit>();
	if (agit && agit->GetHealth() <= 0)
	{
		GameOver();
		return;
	}

	// 기능별 입력 처리 분리
	HandleCameraInput();
	HandleMouseInput();
	HandleUIInput();

	if (Input::Get().GetKeyDown('R')) RandomUpgrade();

#ifdef _DEBUG
	// 치트/디버그용 단축키 처리
	if (Input::Get().GetKeyDown(VK_F2)) GameClear();
	if (Input::Get().GetKeyDown('G')) AddGold(1000);
	if (Input::Get().GetKeyDown(VK_F1))
	{
		auto spawner = enemySpawner.lock();
		if (spawner) spawner->SkipWave();
	}
	if (Input::Get().GetKeyDown(VK_F3))
	{
		showAStarDebug = !showAStarDebug;
	}
#endif
}

void DefenseLevel::HandleCameraInput()
{
	if (Input::Get().GetKeyDown(VK_ESCAPE))
	{
		Game& game = dynamic_cast<Game&>(Engine::Get());
		game.ToggleMenu(State::ESCMENU);
	}

	// WASD 카메라 이동 로직 (제한 없이 자유롭게 이동).
	if (Input::Get().GetKey('W')) cameraPosition.y -= 1;
	if (Input::Get().GetKey('S')) cameraPosition.y += 1;
	if (Input::Get().GetKey('A')) cameraPosition.x -= 1;
	if (Input::Get().GetKey('D')) cameraPosition.x += 1;
}

void DefenseLevel::HandleMouseInput()
{
	Vector2 realMousePos = GetRealMousePos();

	// 좌클릭: 터렛 설치
	static bool wasLButtonDown = false;
	bool isLButtonDown = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;

	if (isLButtonDown && !wasLButtonDown)
	{
		int screenWidth = Engine::Get().GetWidth();
		int screenHeight = Engine::Get().GetHeight();

		Vector2 worldPos;
		worldPos.x = realMousePos.x + cameraPosition.x - (screenWidth / 2);
		worldPos.y = realMousePos.y + cameraPosition.y - (screenHeight / 2);

		if (CanBuildTurret(worldPos.x, worldPos.y) && SpendGold(turretCost))
		{
			SpawnActor<Turret>(worldPos, nextTurretType);
			nextTurretType = static_cast<TurretType>(rand() % 3);

			mapGrid[worldPos.y][worldPos.x] = 2;
			mapGrid[worldPos.y][worldPos.x + 1] = 2;
			mapGrid[worldPos.y + 1][worldPos.x] = 2;
			mapGrid[worldPos.y + 1][worldPos.x + 1] = 2;

			CheckTurretMerge();

			for (const auto& actor : actorList)
			{
				if (!actor->IsActive()) continue;
				if (auto enemy = Craft::Cast<Enemy>(actor)) enemy->RecalculatePath();
			}
		}
	}
	wasLButtonDown = isLButtonDown;

	// 우클릭: 터렛 판매
	static bool wasRButtonDown = false;
	bool isRButtonDown = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;

	if (isRButtonDown && !wasRButtonDown)
	{
		int screenWidth = Engine::Get().GetWidth();
		int screenHeight = Engine::Get().GetHeight();

		Vector2 worldPos;
		worldPos.x = realMousePos.x + cameraPosition.x - (screenWidth / 2);
		worldPos.y = realMousePos.y + cameraPosition.y - (screenHeight / 2);

		for (auto turret : FindActors<Turret>())
		{
			Vector2 tPos = turret->GetPosition();
			if (worldPos.x >= tPos.x && worldPos.x <= tPos.x + 1 &&
				worldPos.y >= tPos.y && worldPos.y <= tPos.y + 1)
			{
				mapGrid[tPos.y][tPos.x] = 0;
				mapGrid[tPos.y][tPos.x + 1] = 0;
				mapGrid[tPos.y + 1][tPos.x] = 0;
				mapGrid[tPos.y + 1][tPos.x + 1] = 0;

				int refundMultiplier = (turret->GetStarTier() == 3) ? 9 : (turret->GetStarTier() == 2) ? 3 : 1;
				AddGold((turretCost / 2) * refundMultiplier);

				turret->Destroy();

				for (auto enemy : FindActors<Enemy>())
				{
					if (enemy->IsActive()) enemy->RecalculatePath();
				}
				break;
			}
		}
	}
	wasRButtonDown = isRButtonDown;
}

void DefenseLevel::HandleUIInput()
{
	// 단축키 Z, X, C로 속성별 업그레이드 진행 (비용: 100골드)
	if (Input::Get().GetKeyDown('Z'))
	{
		if (SpendGold(100)) Turret::upgradeLevelFlame++;
	}
	if (Input::Get().GetKeyDown('X'))
	{
		if (SpendGold(100)) Turret::upgradeLevelIce++;
	}
	if (Input::Get().GetKeyDown('C'))
	{
		if (SpendGold(100)) Turret::upgradeLevelStorm++;
	}
	
	// 랜덤 업그레이드 기능 (비용: 80골드 - 랜덤이라 조금 더 저렴하게 설정해봤습니다!)
	if (Input::Get().GetKeyDown('R'))
	{
		if (SpendGold(80)) 
		{
			RandomUpgrade();
		}
	}

	if (Input::Get().GetKeyDown('T'))
	{
		Gamble();
	}

	if (Input::Get().GetKeyDown(VK_F12))
	{
		isGameInfo = !isGameInfo;
	}
}



void DefenseLevel::RandomUpgrade()
{
	int random = Util::RandomRange(1, 3);
	
	switch(random)
	{
	case 1:
		Turret::upgradeLevelFlame++;
		break;

	case 2:
		Turret::upgradeLevelIce++;
		break;

	case 3:
		Turret::upgradeLevelStorm++;
		break;
	}
}

void DefenseLevel::Gamble()
{
	if (isGambling) return; // 이미 진행 중이면 무시
	if (!SpendGold(300)) return; // 참가비 50골드

	isGambling = true;
	gambleTimer = 0.0f;
	
	// 난수 3개 미리 생성
	for (int i = 0; i < 3; ++i)
	{
		gambleResults[i] = Util::RandomRange(1, 6);
	}
}

void DefenseLevel::DrawGambleAnimation()
{
	int screenWidth = Engine::Get().GetWidth();
	int screenHeight = Engine::Get().GetHeight();
	
	// 화면 중앙 좌표
	int cx = screenWidth / 2;
	int cy = screenHeight / 2;
	
	// 검은색 반투명(또는 단색) 팝업 배경
	for(int y = cy - 5; y <= cy + 5; ++y)
	{
		Renderer::Get().Submit("                                        ", Vector2(cx - 20, y), Color::Black, 150);
	}
	
	// 테두리
	Renderer::Get().Submit("========================================", Vector2(cx - 20, cy - 5), Color::Yellow, 151);
	Renderer::Get().Submit("            SLOT MACHINE                ", Vector2(cx - 20, cy - 3), Color::White, 151);
	Renderer::Get().Submit("========================================", Vector2(cx - 20, cy + 5), Color::Yellow, 151);

	// 주사위 3개 그리기
	char slotStr[256];
	
	// 시간에 따라 첫 번째, 두 번째, 세 번째 숫자를 공개
	// (임시 객체 소멸 방지를 위해 std::string 변수에 할당)
	std::string s1 = (gambleTimer > 1.0f) ? std::to_string(gambleResults[0]) : "?";
	std::string s2 = (gambleTimer > 2.0f) ? std::to_string(gambleResults[1]) : "?";
	std::string s3 = (gambleTimer > 3.0f) ? std::to_string(gambleResults[2]) : "?";
	
	sprintf_s(slotStr, "    [ %s ]      [ %s ]      [ %s ]    ", s1.c_str(), s2.c_str(), s3.c_str());
	Renderer::Get().Submit(slotStr, Vector2(cx - 20, cy), Color::Cyan, 151);

	if (gambleTimer > 4.0f) // 4초가 넘어가면 결과 출력
	{
		int reward = 10 * gambleResults[0] * gambleResults[1] * gambleResults[2];
		char resultStr[256];
		sprintf_s(resultStr, "      WINNER! +%d GOLD!      ", reward);
		Renderer::Get().Submit(resultStr, Vector2(cx - 20, cy + 3), Color::Green, 151);
	}
}

void DefenseLevel::DrawGameInfo(const std::string& filename)
{
	// 최종 경로 조립.
	std::string path = std::string("../Info/") + filename;

	FILE* file = nullptr;
	fopen_s(&file, path.c_str(), "rt");
	if (!file)
	{
		assert(false && "failed to open Info file.");
		return;
	}

	// 파일의 내용을 저장할 버퍼 확인.
	// 파일 길이 확인 -> 파일 위치를 제일 뒤로 이동 시킨 다음, 해당 위치 값 읽기.
	fseek(file, 0, SEEK_END);
	long fileSize = ftell(file);

	// 파일 제일 끝 위치를 구한 다음에는 다시 처음으로 되돌리기.
	rewind(file);

	// 앞에서 구한 위치를 사용해서 버퍼 생성.
	char* buffer = new char[fileSize] {};

	// 데이터 읽기(파일 읽기).
	size_t readSize = fread(buffer, sizeof(char), fileSize, file);

	// 어서트.
	assert(readSize > 0 && "No data is in ther stage file.");

	// 화면 정중앙 배치를 위한 오프셋 설정
	int screenWidth = Engine::Get().GetWidth();
	int screenHeight = Engine::Get().GetHeight();
	int startX = screenWidth / 2 - 25; // 팝업 가로 크기의 절반만큼 빼줌
	int startY = screenHeight / 2 - 12;
	
	// 배경을 검정색으로 덮기 (팝업창 느낌)
	for (int y = startY - 1; y < startY + 26; ++y)
	{
		Renderer::Get().Submit("                                                      ", Vector2(startX - 2, y), Color::Black, 200);
	}

	int index = 0;
	Vector2 position;
	
	// 문자열 단위로 끊어서 한 줄씩 제출(Submit)하는 것이 렌더러에 훨씬 효율적입니다.
	std::string currentLine = "";
	
	while (index < fileSize)
	{
		char c = buffer[index];
		++index;

		if (c == '\n' || c == '\r')
		{
			if (c == '\r' && index < fileSize && buffer[index] == '\n') index++; // Windows CRLF 호환
			
			// 한 줄이 완성되면 렌더러에 제출
			Renderer::Get().Submit(currentLine, Vector2(startX, startY + position.y), Color::Yellow, 201);
			currentLine = "";
			++position.y;
			continue;
		}
		
		currentLine += c;
	}
	// 마지막 줄 처리
	if (!currentLine.empty())
	{
		Renderer::Get().Submit(currentLine, Vector2(startX, startY + position.y), Color::Yellow, 201);
	}

	delete[] buffer;
	buffer = nullptr;

	fclose(file);
	file = nullptr;
}
void DefenseLevel::Draw()
{

	// 1. 부모의 Draw 호출 (벽, 바닥, 설치된 터렛 등 기존 액터 렌더링)
	Level::Draw();

	// 아지트 체력 렌더링 (우측 UI 패널로 이동됨)
	// 2. 터렛 2x2 미리보기 렌더링
	Vector2 realMousePos = GetRealMousePos();

	// 화면 좌표를 월드 좌표로 변환하여 설치 가능 여부 확인
	int screenWidth = Engine::Get().GetWidth();
	int screenHeight = Engine::Get().GetHeight();
	Vector2 previewWorldPos;
	previewWorldPos.x = realMousePos.x + cameraPosition.x - (screenWidth / 2);
	previewWorldPos.y = realMousePos.y + cameraPosition.y - (screenHeight / 2);

	// 설치 가능 여부 및 골드 조건 확인
	bool canBuild = CanBuildTurret(previewWorldPos.x, previewWorldPos.y) && (currentGold >= turretCost);
	// 다음 설치될 터렛의 심볼 및 색상 결정
	std::string previewSymbol = "TT";
	Color typeColor = Color::Yellow;
	switch (nextTurretType)
	{
	case TurretType::FLAME: previewSymbol = "FF"; typeColor = Color::Red; break;
	case TurretType::ICE:   previewSymbol = "II"; typeColor = Color::Cyan; break;
	case TurretType::STORM: previewSymbol = "TT"; typeColor = Color::Yellow; break;
	}

	// 설치 가능하면 해당 터렛의 색상, 불가능하면 어두운 회색 (또는 빨간색)
	// 빨간색(Red)은 화염 터렛과 색이 겹치므로, 설치 불가 시 약간 다른 색(Magenta 등)을 써도 좋음.
	// 이번엔 설치 불가능을 쉽게 알 수 있도록 배경색을 씌우거나 어두운 빨강, 혹은 심볼을 XX로 바꿀 수 있음.
	if (!canBuild) 
	{
		previewSymbol = "XX";
		typeColor = Color::Red;
	}
	
	Color previewColor = typeColor;
	int previewSortingOrder = 20; // 맵 위에 떠야 하므로 높게 설정

	Renderer::Get().Submit(previewSymbol, realMousePos, previewColor, previewSortingOrder);
	Renderer::Get().Submit(previewSymbol, Vector2(realMousePos.x, realMousePos.y + 1), previewColor, previewSortingOrder);

	// 사거리 표시 렌더링
	float atkRange = 10.0f;
	switch (nextTurretType)
	{
	case TurretType::FLAME: atkRange = 3.0f; break;
	case TurretType::ICE:   atkRange = 5.0f; break;
	case TurretType::STORM: atkRange = 4.0f; break;
	}

	// 중앙점 (터렛의 중심) - 스크린 좌표 기준
	int centerScrX = realMousePos.x + 1;
	int centerScrY = realMousePos.y; 
	
	// 사거리 원 테두리 그리기
	int rangeInt = static_cast<int>(atkRange);
	for (int y = -rangeInt; y <= rangeInt; ++y)
	{
		for (int x = -rangeInt; x <= rangeInt; ++x)
		{
			float dist = std::sqrt(static_cast<float>(x * x + y * y));
			// 사거리의 테두리 부분(외곽선)만 그리기 위해 오차 범위를 줌 (0.8f로 늘려 틈새를 줄임)
			if (std::abs(dist - atkRange) <= 0.8f)
			{
				Vector2 rangePos(centerScrX + x, centerScrY + y);
				// 화면 밖을 벗어나지 않게 처리
				if (rangePos.x >= 0 && rangePos.x < screenWidth &&
					rangePos.y >= 0 && rangePos.y < screenHeight)
				{
					// 배경과 겹치지 않게 어두운 색상이나 특정 문자로 렌더링
					// 미리보기보다 바로 아래 단계인 19번 우선순위 사용
					Renderer::Get().Submit("+", rangePos, Color::Green, 9);
				}
			}
		}
	}

	// ====== UI 패널 렌더링 (우측 화면) ======
	int uiX = 65; // 맵(50) 우측에 15칸 여백을 두고 홀쭉하게 배치
	
	// UI 영역 전체를 검은색 배경(공백)으로 덮어서 맵이 관통되어 보이지 않게 처리
	std::string blackCover(55, ' ');
	for (int i = 0; i < screenHeight; ++i)
	{
		Renderer::Get().Submit(blackCover, Vector2(uiX, i), Color::Black, 90);
	}

	// [ WAVE INFO ] : 좌측 상단(맵 위)으로 이동
	auto spawner = enemySpawner.lock();
	if (spawner)
	{
		char waveStr[256];
		if (spawner->IsWaveActive())
		{
			sprintf_s(waveStr, " Wave %d : In Progress! ", spawner->GetCurrentWave());
			Renderer::Get().Submit(waveStr, Vector2(0, 0), Color::Red, 100);
		}
		else
		{
			int remainTime = static_cast<int>(std::ceil(spawner->GetRemainingWaveTime()));
			sprintf_s(waveStr, " Next Wave %d in: %d:%02d ", spawner->GetCurrentWave(), remainTime / 60, remainTime % 60);
			Renderer::Get().Submit(waveStr, Vector2(0, 0), Color::Yellow, 100);
		}
	}

	int uiY = 1;
	
	Renderer::Get().Submit("==================================================", Vector2(uiX, uiY++), Color::White, 100);
	Renderer::Get().Submit("                    SK DEFENSE                    ", Vector2(uiX, uiY++), Color::Cyan, 100);
	Renderer::Get().Submit("==================================================", Vector2(uiX, uiY++), Color::White, 100);
	uiY++;

	// 1. 아지트 체력
	auto agit = FindActor<Agit>();
	if (agit)
	{
		Renderer::Get().Submit(" [ AGIT STATUS ]", Vector2(uiX, uiY++), Color::Green, 100);
		float hpRatio = static_cast<float>(agit->GetHealth()) / 100;
		int barLength = 20;
		int filledLength = static_cast<int>(hpRatio * barLength);
		std::string hpBar = " HP: [";
		for (int i = 0; i < barLength; ++i)
		{
			hpBar += (i < filledLength) ? "#" : ".";
		}
		hpBar += "] " + std::to_string(agit->GetHealth()) + " / 100";
		Renderer::Get().Submit(hpBar, Vector2(uiX, uiY++), Color::Green, 100);
	}
	uiY++;

	// 3. 자원 (골드)
	Renderer::Get().Submit(" [ RESOURCE ]", Vector2(uiX, uiY++), Color::Yellow, 100);
	char goldStr[256];
	sprintf_s(goldStr, " Gold: %d G  (Turret: %d G)", currentGold, turretCost);
	Renderer::Get().Submit(goldStr, Vector2(uiX, uiY++), Color::Yellow, 100);
	uiY++;

	// 4. 업그레이드 상태
	Renderer::Get().Submit(" [ UPGRADE STATUS (Cost: 100G) ]", Vector2(uiX, uiY++), Color::White, 100);
	char upgFlameStr[256], upgIceStr[256], upgStormStr[256];
	sprintf_s(upgFlameStr, "  [Z] Flame Upgrade : +%d", Turret::upgradeLevelFlame);
	sprintf_s(upgIceStr,   "  [X] Ice Upgrade   : +%d", Turret::upgradeLevelIce);
	sprintf_s(upgStormStr, "  [C] Storm Upgrade : +%d", Turret::upgradeLevelStorm);
	
	Renderer::Get().Submit(upgFlameStr, Vector2(uiX, uiY++), Color::Red, 100);
	Renderer::Get().Submit(upgIceStr,   Vector2(uiX, uiY++), Color::Cyan, 100);
	Renderer::Get().Submit(upgStormStr, Vector2(uiX, uiY++), Color::Yellow, 100);

	// 5. 하단 6칸 컨트롤 패널
	int panelY = 18; // 두 줄씩 들어가므로 시작 위치를 약간 위로 올림
	Renderer::Get().Submit("==================================================", Vector2(uiX, panelY++), Color::White, 100);
	Renderer::Get().Submit("   [   F12   ]  |   [    T    ]  |   [    R    ]  ", Vector2(uiX, panelY++), Color::Cyan, 100);
	Renderer::Get().Submit("   [Game Rule]  |   [ Gamble  ]  |   [RandomUpg]  ", Vector2(uiX, panelY++), Color::Cyan, 100);
	Renderer::Get().Submit("   [---------]  |   [   300G  ]  |   [   80G   ]  ", Vector2(uiX, panelY++), Color::Cyan, 100);
	Renderer::Get().Submit("--------------------------------------------------", Vector2(uiX, panelY++), Color::DarkGray, 100);
	Renderer::Get().Submit("   [    Z    ]  |   [    X    ]  |   [    C    ]  ", Vector2(uiX, panelY++), Color::Cyan, 100);
	Renderer::Get().Submit("   [Upg Flame]  |   [ Upg Ice ]  |   [Upg Storm]  ", Vector2(uiX, panelY++), Color::Cyan, 100);
	Renderer::Get().Submit("   [   100G  ]  |   [   100G  ]  |   [   100G  ]  ", Vector2(uiX, panelY++), Color::Cyan, 100);
	Renderer::Get().Submit("==================================================", Vector2(uiX, panelY++), Color::White, 100);
	
	// 디버그용: 현재 마우스 좌표 (가장 아래 구석)
	char debugStr[256];
	sprintf_s(debugStr, "Mouse(Scr): %d,%d | World: %d,%d", realMousePos.x, realMousePos.y, previewWorldPos.x, previewWorldPos.y);
	Renderer::Get().Submit(debugStr, Vector2(uiX, 28), Color::DarkGray, 100);

	// 도박(Gamble) 진행 중이면 팝업창(애니메이션) 띄우기
	if (isGambling)
	{
		DrawGambleAnimation();
	}

	// GameInfo 진행중이면 팝업창 띄우기.
	if (isGameInfo)
	{
		DrawGameInfo("GameInfo.txt");
	}

#ifdef _DEBUG
	if (showAStarDebug)
	{
		for (auto enemy : FindActors<Enemy>())
		{
			if (!enemy->IsActive()) continue;

			const auto& path = enemy->GetPath();
			const auto& history = enemy->GetSearchHistory();
			
			// 애니메이션 프레임 증가 (각 적마다 개별 진행도 유지)
			enemy->debugAnimFrame++;
			// 속도 조절: 1프레임당 50칸씩 퍼지도록 초고속(파바박!)으로 설정
			int animProgress = enemy->debugAnimFrame * 50;

			// 1단계: 탐색 히스토리 그리기 (초록색 +)
			for (size_t i = 0; i < history.size(); ++i)
			{
				if (i > (size_t)animProgress) break;

				Vector2 p = history[i];
				int drawX = p.x - cameraPosition.x + (Engine::Get().GetWidth() / 2);
				int drawY = p.y - cameraPosition.y + (Engine::Get().GetHeight() / 2);

				if (drawX >= 0 && drawX < Engine::Get().GetWidth() && drawY >= 0 && drawY < Engine::Get().GetHeight())
				{
					Renderer::Get().Submit("+", Vector2(drawX, drawY), Color::Green, 70);
				}
			}

			// 2단계: 탐색 애니메이션이 끝난 후, 실제 경로 그리기 (마젠타 *)
			if (animProgress > history.size())
			{
				int pathProgress = animProgress - static_cast<int>(history.size());
				int currentIndex = enemy->GetCurrentPathIndex();

				for (size_t i = currentIndex; i < path.size(); ++i)
				{
					if (i - currentIndex > (size_t)pathProgress) break;

					Vector2 p = path[i];
					int drawX = p.x - cameraPosition.x + (Engine::Get().GetWidth() / 2);
					int drawY = p.y - cameraPosition.y + (Engine::Get().GetHeight() / 2);

					if (drawX >= 0 && drawX < Engine::Get().GetWidth() && drawY >= 0 && drawY < Engine::Get().GetHeight())
					{
						Renderer::Get().Submit("*", Vector2(drawX, drawY), Color::Magenta, 80);
					}
				}
			}
			
			// 루프 (탐색 히스토리 크기 + 경로 크기 + 결과를 감상할 여유 대기시간 추가) 후 리셋
			// 1프레임당 50칸씩 오르므로, 2000을 더해주면 약 40프레임(0.6초) 정도 결과가 유지됩니다.
			int maxProgress = static_cast<int>(history.size()) + static_cast<int>(path.size()) + 2000;
			if (animProgress > maxProgress)
			{
				enemy->debugAnimFrame = 0;
			}
		}
	}
#endif
}

Craft::Vector2 DefenseLevel::GetRealMousePos()
{
	HWND hwnd = GetConsoleWindow();
	if (hwnd == NULL)
	{
		// 윈도우 터미널(가상 콘솔) 등에서 핸들을 얻지 못한 경우 기존 이벤트 버퍼 좌표 반환
		return Input::Get().GetMousePosition();
	}

	POINT pt;
	GetCursorPos(&pt);
	ScreenToClient(hwnd, &pt);

	int screenWidth = Engine::Get().GetWidth();
	int screenHeight = Engine::Get().GetHeight();

	int mouseX = 0, mouseY = 0;
	RECT clientRect;
	if (GetClientRect(hwnd, &clientRect))
	{
		int clientWidth = clientRect.right - clientRect.left;
		int clientHeight = clientRect.bottom - clientRect.top;

		if (screenWidth > 0 && screenHeight > 0 && clientWidth > 0 && clientHeight > 0)
		{
			float fontWidth = (float)clientWidth / screenWidth;
			float fontHeight = (float)clientHeight / screenHeight;

			mouseX = static_cast<int>(pt.x / fontWidth);
			mouseY = static_cast<int>(pt.y / fontHeight);
		}
	}
	return Craft::Vector2(mouseX, mouseY);
}

void DefenseLevel::LoadMap(const std::string& filename)
{
	// 최종 경로 조립.
	std::string path = std::string("../Assets/") + filename;

	// 파일 열기 (C-Style).
	FILE* file = nullptr;
	fopen_s(&file, path.c_str(), "rt");
	if (!file)
	{
		assert(false && "failed to open sokoban stage file.");
		return;
	}

	// 파일의 내용을 저장할 버퍼(데이터 저장공간) 확인.
	// 파일 길이 확인 -> 파일 위치를 제일 뒤로 이동 시킨 다음, 해당 위치 값 읽기.
	fseek(file, 0, SEEK_END);
	long fileSize = ftell(file);


	// 파일 제일 끝위치를 구한 다음에는 다시 처음으로 되돌리기.
	//fseek(file, 0, SEEK_SET);
	rewind(file);

	// 앞에서 구한 위치를 사용해서 버퍼 생성.
	char* buffer = new char[fileSize] {};

	// 데이터 읽기(파일 읽기).
	size_t readSize = fread(buffer, sizeof(char), fileSize, file);


	// 어서트.
	assert(readSize > 0 && "No data is in the stage file.");

	// 읽은 데이터를 기반으로 로직 제작.
	// 1. 화면에 액터를 그리기.

	// 문자열에 저장된 값을 접근할 때 사용할 인덱스.
	int index = 0;

	// 액터 생성에 사용할 위치 값.
	Vector2 position;
	
	mapGrid.clear();
	std::vector<int> currentRow;

	// 종료 조건 - 읽는 위치가 파일의 크기를 넘으면 종료.
	while (index < fileSize)
	{
		// 이번에 확인할 문자 값.
		char mapCharacter = buffer[index];

		// 인덱스 증가 처리.
		++index;

		// 현재 문자가 개행 문자라면 로직은 건너뛰고,
		// 위치 값만 설정.
		if (mapCharacter == '\n')
		{
			if (position.x > mapWidth) mapWidth = position.x;
			++position.y;
			position.x = 0;
			mapGrid.push_back(currentRow);
			currentRow.clear();
			continue;
		}

		// 읽은 문자 별로 처리.
		switch (mapCharacter)
		{
			// 벽.
		case '#':
			SpawnActor<Wall>(position);
			currentRow.push_back(1);
			break;

			// 바닥(땅).
		case '.':
			SpawnActor<Ground>(position);
			currentRow.push_back(0);
			break;

			// 스폰 지점.
		case 'S':
			spawnPoints.push_back(position);
			SpawnActor<EnemyHouse>(position);
			currentRow.push_back(0);
			break;

			// 아지트.
		case 'D':
			targetPoint = position;
			SpawnActor<Agit>(position);
			currentRow.push_back(3);
			break;
		
		default:
			currentRow.push_back(0); // 알 수 없는 문자는 빈 공간 처리spawnPoint
			break;
		}

		// x 위치 업데이트.
		++position.x;
	}
	
	if (!currentRow.empty())
	{
		mapGrid.push_back(currentRow);
	}
	
	mapHeight = position.y + 1;

	// 모두 사용한 버퍼 해제.
	delete[] buffer;
	buffer = nullptr;

	// 파일 닫기.
	fclose(file);
	// 파일을 닫아줄 때 file을 다른 곳에서 사용 하지 않아서
	// 닫아주기만 하고 nullptr로 초기화를 않아도 상관없지만
	// nullptr로 초기화 시켜주는것은 좋은습관.
	file = nullptr;
}

void DefenseLevel::GameOver()
{
	Game& game = dynamic_cast<Game&>(Engine::Get());
	game.ToggleMenu(State::GAMEOVER);
}

void DefenseLevel::GameClear()
{
	Game& game = dynamic_cast<Game&>(Engine::Get());
	game.ToggleMenu(State::GAMECLEAR);
}

bool DefenseLevel::CanBuildTurret(int x, int y)
{
	// 1. 터렛은 2x2 사이즈이므로 4칸 모두 0(바닥)인지 확인
	for (int i = 0; i < 2; ++i)
	{
		for (int j = 0; j < 2; ++j)
		{
			int cx = x + i;
			int cy = y + j;
			
			// 맵 경계선 체크
			if (cy < 0 || cy >= static_cast<int>(mapGrid.size())) return false;
			if (cx < 0 || cx >= static_cast<int>(mapGrid[cy].size())) return false;
			
			// 벽(1), 기존 터렛(2), 아지트(3)가 있으면 설치 불가
			if (mapGrid[cy][cx] != 0) return false;
		}
	}

	// 2. 가상 시뮬레이션: 임시로 4칸에 터렛(2)을 가상 설치
	mapGrid[y][x] = 2;
	mapGrid[y][x + 1] = 2;
	mapGrid[y + 1][x] = 2;
	mapGrid[y + 1][x + 1] = 2;

	bool canReach = true;

	// (A) 모든 스폰 지점(S)에서 아지트(D)까지 경로가 여전히 존재하는지 확인
	{
		for (const auto& sp : spawnPoints)
		{
			AStar astar;
			Node* startNode = new Node(static_cast<int>(std::round(sp.x)), static_cast<int>(std::round(sp.y)));
			Node* goalNode = new Node(static_cast<int>(std::round(targetPoint.x)), static_cast<int>(std::round(targetPoint.y)));

			std::vector<Node*> path = astar.FindPath(startNode, goalNode, mapGrid);
			delete goalNode;

			if (path.empty())
			{
				canReach = false;
				break;
			}
		}
	}

	// (B) 현재 맵에 살아있는(Active) 몬스터들이 있다면, 그 몬스터들도 아지트에 갈 수 있는지 확인 (갇힘 방지)
	if (canReach)
	{
		for (const auto& actor : actorList)
		{
			if (!actor->IsActive()) continue;
			auto enemy = Craft::Cast<Enemy>(actor);
			if (enemy)
			{
				AStar astar;
				Vector2 enemyPos = enemy->GetPosition();
				Node* startNode = new Node(static_cast<int>(std::round(enemyPos.x)), static_cast<int>(std::round(enemyPos.y)));
				Node* goalNode = new Node(static_cast<int>(std::round(targetPoint.x)), static_cast<int>(std::round(targetPoint.y)));

				std::vector<Node*> path = astar.FindPath(startNode, goalNode, mapGrid);
				delete goalNode;

				if (path.empty())
				{
					canReach = false;
					break;
				}
			}
		}
	}

	// 3. 임시 설치했던 터렛을 다시 빈 바닥(0)으로 롤백(복구)
	mapGrid[y][x] = 0;
	mapGrid[y][x + 1] = 0;
	mapGrid[y + 1][x] = 0;
	mapGrid[y + 1][x + 1] = 0;

	return canReach;
}

void DefenseLevel::CheckTurretMerge()
{
	bool hasMerged = false;

	// 연쇄 합성(1성 3개 -> 2성 승급 -> 마침 2성이 3개가 되어 3성으로 연쇄 승급)을 위해 반복 검사
	do
	{
		hasMerged = false;

		// 1. 현재 활성화된 모든 터렛 수집 (배치된 목록 + 생성 요청 대기 목록)
		std::vector<std::shared_ptr<Turret>> allTurrets;
		for (const auto& actor : actorList)
		{
			auto turret = Craft::Cast<Turret>(actor);
			if (turret && turret->IsActive() && !turret->HasExpired())
			{
				allTurrets.push_back(turret);
			}
		}
		for (const auto& actor : addRequestedActorList)
		{
			auto turret = Craft::Cast<Turret>(actor);
			if (turret && turret->IsActive() && !turret->HasExpired())
			{
				allTurrets.push_back(turret);
			}
		}

		// 2. (타입, 성급)별로 그룹화 (3성은 이미 최고 등급이므로 1성과 2성만 대상)
		std::map<std::pair<TurretType, int>, std::vector<std::shared_ptr<Turret>>> groups;

		for (auto turret : allTurrets)
		{
			if (turret->GetStarTier() < 3)
			{
				groups[{turret->GetTurretType(), turret->GetStarTier()}].push_back(turret);
			}
		}

		// 3. 같은 종류 & 같은 성급이 3개 이상 모인 그룹 탐색
		for (auto& groupPair : groups)
		{
			auto& turretList = groupPair.second;
			if (turretList.size() >= 3)
			{
				// 설치 순서(spawnOrder) 기준 오름차순 정렬 (가장 먼저 지어진 터렛이 0번 인덱스)
				std::sort(turretList.begin(), turretList.end(), [](const std::shared_ptr<Turret>& a, const std::shared_ptr<Turret>& b) {
					return a->GetSpawnOrder() < b->GetSpawnOrder();
				});

				// (1) 가장 먼저 지어진 1번 터렛: 성급 업그레이드! (1성 -> 2성, 또는 2성 -> 3성)
				turretList[0]->UpgradeStar();

				// (2) 나머지 2개 터렛 (1번, 2번 인덱스): 타일 4칸을 빈 땅(0)으로 복구하고 파괴
				for (int i = 1; i <= 2; ++i)
				{
					Vector2 tPos = turretList[i]->GetPosition();
					mapGrid[tPos.y][tPos.x] = 0;
					mapGrid[tPos.y][tPos.x + 1] = 0;
					mapGrid[tPos.y + 1][tPos.x] = 0;
					mapGrid[tPos.y + 1][tPos.x + 1] = 0;

					turretList[i]->Destroy();
				}

				// 2개의 터렛이 사라져서 길이 넓어졌으므로 몬스터 경로 재계산
				for (auto enemy : FindActors<Enemy>())
				{
					if (enemy && enemy->IsActive())
					{
						enemy->RecalculatePath();
					}
				}

				hasMerged = true;
				break; // 한 번 합성했으면 상태가 바뀌었으므로 다시 루프 돌면서 연쇄 합성 확인
			}
		}
	} while (hasMerged);
}



