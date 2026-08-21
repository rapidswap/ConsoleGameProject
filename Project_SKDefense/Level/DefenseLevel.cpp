#include "DefenseLevel.h"
#include <Input/Input.h>
#include <Render/Renderer.h>
#include <fstream>
#include <iostream>

DefenseLevel::DefenseLevel()
{
	cameraOffset = Craft::Vector2(0, 0);
	LoadMap();
}

void DefenseLevel::LoadMap()
{
	std::ifstream file("Assets/SK_Defense_Map.txt");
	if (file.is_open())
	{
		std::string line;
		while (std::getline(file, line))
		{
			mapData.push_back(line);
		}
		file.close();
	}
}

void DefenseLevel::Update(float deltaTime)
{
	// WASD 카메라 이동 로직
	// GetKey는 누르고 있는 동안 계속 true이므로 부드러운 스크롤은 안 될 수도 있지만, 
	// GetKeyDown과 GetKey 조합 혹은 프레임 단위 이동으로 처리합니다.
	if (Craft::Input::Get().GetKey('W')) cameraOffset.y -= 1;
	if (Craft::Input::Get().GetKey('S')) cameraOffset.y += 1;
	if (Craft::Input::Get().GetKey('A')) cameraOffset.x -= 1;
	if (Craft::Input::Get().GetKey('D')) cameraOffset.x += 1;

	// 카메라 오프셋이 음수가 되지 않도록 제한 (옵션)
	if (cameraOffset.x < 0) cameraOffset.x = 0;
	if (cameraOffset.y < 0) cameraOffset.y = 0;
	
	// 최대 카메라 오프셋 제한 (맵 사이즈 기반)
	int maxCamX = (mapData.empty() ? 0 : mapData[0].size()) - 30; // 화면 가로 크기 대략 30으로 가정
	int maxCamY = mapData.size() - 20; // 화면 세로 크기 대략 20으로 가정
	if (maxCamX < 0) maxCamX = 0;
	if (maxCamY < 0) maxCamY = 0;

	if (cameraOffset.x > maxCamX) cameraOffset.x = maxCamX;
	if (cameraOffset.y > maxCamY) cameraOffset.y = maxCamY;
}

void DefenseLevel::Draw()
{
	// 맵 렌더링
	for (int y = 0; y < (int)mapData.size(); ++y)
	{
		for (int x = 0; x < (int)mapData[y].size(); ++x)
		{
			// 화면 좌표로 변환
			Craft::Vector2 screenPos(x - cameraOffset.x, y - cameraOffset.y);

			char tile = mapData[y][x];
			std::string tileStr(1, tile);
			Craft::Color color = Craft::Color::White;

			if (tile == '#') color = Craft::Color::DarkGray;
			else if (tile == 'S') color = Craft::Color::Red;
			else if (tile == 'D') color = Craft::Color::Blue;
			else if (tile == '.') color = Craft::Color::Gray;

			Craft::Renderer::Get().Submit(tileStr, screenPos, color, 0);
		}
	}
}
