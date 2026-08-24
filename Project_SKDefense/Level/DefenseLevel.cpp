#include "DefenseLevel.h"
#include <Input/Input.h>
#include <Render/Renderer.h>
#include <Actor/Ground.h>
#include <Actor/Wall.h>
#include <Actor/Agit.h>
#include <fstream>
#include <iostream>
#include <cassert>

using namespace Craft;

void DefenseLevel::OnInitialized()
{
	Level::OnInitialized();

	// 파일을 읽어서 맵 로드 (mapWidth, mapHeight가 계산됨).
	LoadMap("SK_Defense_Map.txt");

	// 카메라를 맵의 정중앙에 위치시킴.
	cameraPosition = Craft::Vector2(mapWidth / 2, mapHeight / 2);
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
			continue;
		}

		// 읽은 문자 별로 처리.
		switch (mapCharacter)
		{
			// 벽.
		case '#':
			// 벽 액터 생성.
			SpawnActor<Wall>(position);
			break;

			// 바닥(땅).
		case '.':
			// 바닥 액터 생성.
			SpawnActor<Ground>(position);
			break;

		case 'D':
			// 아지트 액터 생성.
			SpawnActor<Agit>(position);
			break;

		}


		// x 위치 업데이트.
		++position.x;
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


#include <Actor/Turret.h>

void DefenseLevel::Tick(float deltaTime)
{
	// WASD 카메라 이동 로직 (제한 없이 자유롭게 이동)
	if (Craft::Input::Get().GetKey('W')) cameraPosition.y -= 1;
	if (Craft::Input::Get().GetKey('S')) cameraPosition.y += 1;
	if (Craft::Input::Get().GetKey('A')) cameraPosition.x -= 1;
	if (Craft::Input::Get().GetKey('D')) cameraPosition.x += 1;

	// 마우스 클릭 시 터렛 설치
	if (Craft::Input::Get().GetKeyDown(VK_LBUTTON))
	{
		Craft::Vector2 mousePos = Craft::Input::Get().GetMousePosition();
		int screenWidth = Craft::Engine::Get().GetWidth();
		int screenHeight = Craft::Engine::Get().GetHeight();
		
		// 화면 좌표를 월드 좌표로 변환
		Craft::Vector2 worldPos;
		worldPos.x = mousePos.x + cameraPosition.x - (screenWidth / 2);
		worldPos.y = mousePos.y + cameraPosition.y - (screenHeight / 2);

		// TODO: 설치 가능 여부 검사 (벽, 기존 터렛 등)
		SpawnActor<Turret>(worldPos);
	}
}

void DefenseLevel::Draw()
{
	// 1. 부모의 Draw 호출 (벽, 바닥, 설치된 터렛 등 기존 액터 렌더링)
	Level::Draw();

	// 2. 터렛 2x2 미리보기 렌더링 (마우스 커서 위치에 바로 출력)
	Craft::Vector2 mousePos = Craft::Input::Get().GetMousePosition();
	
	// 설치 가능한 상태라고 가정하고 초록색(미리보기)으로 렌더링
	Craft::Color previewColor = Craft::Color::Green;
	int previewSortingOrder = 20; // 맵 위에 떠야 하므로 높게 설정

	Craft::Renderer::Get().Submit("TT", mousePos, previewColor, previewSortingOrder);
	Craft::Renderer::Get().Submit("TT", Craft::Vector2(mousePos.x, mousePos.y + 1), previewColor, previewSortingOrder);
}

