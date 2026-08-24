#include "DefenseLevel.h"
#include <Engine/Engine.h>
#include <Input/Input.h>
#include <Render/Renderer.h>
#include <Actor/Ground.h>
#include <Actor/Wall.h>
#include <Actor/Agit.h>
#include <Actor/Turret.h>
#include <Actor/Enemy.h>
#include <Actor/EnemySpawner.h>
#include <Util/Timer.h>
#include <fstream>
#include <iostream>
#include <cassert>
#include <cmath>

using namespace Craft;

void DefenseLevel::OnInitialized()
{
	Level::OnInitialized();

	// 파일을 읽어서 맵 로드 (mapWidth, mapHeight가 계산됨).
	LoadMap("SK_Defense_Map.txt");

	// 카메라를 맵의 정중앙에 위치시킴.
	cameraPosition = Vector2(mapWidth / 2, mapHeight / 2);

	// 적 생성기 액터 추가.
	enemySpawner = SpawnActor<EnemySpawner>();

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
			spawnPoint = position;
			SpawnActor<Ground>(position);
			currentRow.push_back(0);
			break;

			// 아지트.
		case 'D':
			targetPoint = position;
			SpawnActor<Agit>(position);
			currentRow.push_back(3);
			break;
		
		default:
			currentRow.push_back(0); // 알 수 없는 문자는 빈 공간 처리
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

bool DefenseLevel::CanBuildTurret(int x, int y)
{
	// 터렛은 2x2 사이즈이므로 4칸 모두 0(바닥)인지 확인
	for (int i = 0; i < 2; ++i)
	{
		for (int j = 0; j < 2; ++j)
		{
			int cx = x + i;
			int cy = y + j;
			
			// 맵 경계선 체크
			if (cy < 0 || cy >= mapGrid.size()) return false;
			if (cx < 0 || cx >= mapGrid[cy].size()) return false;
			
			// 벽(1), 기존 터렛(2), 아지트(3)가 있으면 설치 불가
			if (mapGrid[cy][cx] != 0) return false;
		}
	}
	return true;
}

void DefenseLevel::Tick(float deltaTime)
{
	// 부모의 Tick 호출 (배치된 모든 액터들의 Tick 실행)
	Level::Tick(deltaTime);

	// WASD 카메라 이동 로직 (제한 없이 자유롭게 이동).
	if (Input::Get().GetKey('W')) cameraPosition.y -= 1;
	if (Input::Get().GetKey('S')) cameraPosition.y += 1;
	if (Input::Get().GetKey('A')) cameraPosition.x -= 1;
	if (Input::Get().GetKey('D')) cameraPosition.x += 1;

	Vector2 realMousePos = GetRealMousePos();

	// 마우스 클릭 시 터렛 설치 (빠른 클릭 누락 방지를 위해 GetAsyncKeyState 사용)
	static bool wasLButtonDown = false;
	bool isLButtonDown = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;

	if (isLButtonDown && !wasLButtonDown)
	{
		int screenWidth = Engine::Get().GetWidth();
		int screenHeight = Engine::Get().GetHeight();
		
		// 화면 좌표를 월드 좌표로 변환
		Vector2 worldPos;
		worldPos.x = realMousePos.x + cameraPosition.x - (screenWidth / 2);
		worldPos.y = realMousePos.y + cameraPosition.y - (screenHeight / 2);

		// 설치 가능 여부 검사 (맵 바깥, 벽, 기존 터렛 등)
		if (CanBuildTurret(worldPos.x, worldPos.y))
		{
			SpawnActor<Turret>(worldPos);
			
			// 설치된 타일을 터렛(2)으로 마킹하여 이후 겹쳐 짓기나 적의 이동을 차단
			mapGrid[worldPos.y][worldPos.x] = 2;
			mapGrid[worldPos.y][worldPos.x + 1] = 2;
			mapGrid[worldPos.y + 1][worldPos.x] = 2;
			mapGrid[worldPos.y + 1][worldPos.x + 1] = 2;
			
			// 맵이 변경되었으므로 모든 적들에게 경로 재탐색 지시
			for (const std::shared_ptr<Actor>& actor : actorList)
			{
				if (!actor->IsActive()) continue;
				
				auto enemy = Craft::Cast<Enemy>(actor);
				if (enemy)
				{
					enemy->RecalculatePath();
				}
			}
		}
	}
	wasLButtonDown = isLButtonDown;

	// 테스트용: E 키를 누르면 스폰 포인트(S)에서 적 생성
	static bool wasEDown = false;
	bool isEDown = Input::Get().GetKey('E');
	if (isEDown && !wasEDown)
	{
		SpawnActor<Enemy>(spawnPoint);
	}
	wasEDown = isEDown;
}

void DefenseLevel::Draw()
{
	// 1. 부모의 Draw 호출 (벽, 바닥, 설치된 터렛 등 기존 액터 렌더링)
	Level::Draw();

	// 2. 터렛 2x2 미리보기 렌더링
	Vector2 realMousePos = GetRealMousePos();
	
	// 화면 좌표를 월드 좌표로 변환하여 설치 가능 여부 확인
	int screenWidth = Engine::Get().GetWidth();
	int screenHeight = Engine::Get().GetHeight();
	Vector2 previewWorldPos;
	previewWorldPos.x = realMousePos.x + cameraPosition.x - (screenWidth / 2);
	previewWorldPos.y = realMousePos.y + cameraPosition.y - (screenHeight / 2);

	// 설치 가능하면 초록색, 불가능하면 빨간색
	Color previewColor = CanBuildTurret(previewWorldPos.x, previewWorldPos.y) ? Color::Green : Color::Red;
	int previewSortingOrder = 20; // 맵 위에 떠야 하므로 높게 설정

	Renderer::Get().Submit("TT", realMousePos, previewColor, previewSortingOrder);
	Renderer::Get().Submit("TT", Vector2(realMousePos.x, realMousePos.y + 1), previewColor, previewSortingOrder);

	// 디버그용: 현재 마우스 스크린 좌표와 월드 좌표 출력
	char debugStr[256];
	sprintf_s(debugStr, "Mouse(Scr): %d,%d | World: %d,%d", realMousePos.x, realMousePos.y, previewWorldPos.x, previewWorldPos.y);
	Renderer::Get().Submit(debugStr, Vector2(0, 0), Color::White, 100);

	// 웨이브 상태 표시
	auto spawner = enemySpawner.lock();
	if (spawner)
	{
		char waveStr[256];
		if (spawner->IsWaveActive())
		{
			sprintf_s(waveStr, "Wave %d : In Progress!", spawner->GetCurrentWave());
			Renderer::Get().Submit(waveStr, Vector2(0, 1), Color::Red, 100);
		}
		else
		{
			int remainTime = static_cast<int>(std::ceil(spawner->GetRemainingWaveTime()));
			int minutes = remainTime / 60;
			int seconds = remainTime % 60;
			
			sprintf_s(waveStr, "Next Wave %d in: %d:%02d", spawner->GetCurrentWave(), minutes, seconds);
			Renderer::Get().Submit(waveStr, Vector2(0, 1), Color::Yellow, 100);
		}
	}
}
