#pragma once

#include <Level/Level.h>
#include <Math/Vector2.h>
#include <vector>
#include <string>

class DefenseLevel : public Craft::Level
{
public:

	virtual void OnInitialized();
	virtual void Tick(float deltaTime) override;
	virtual void Draw() override;

private:
	Craft::Vector2 GetRealMousePos();
	void LoadMap(const std::string& filename);

private:
	int mapWidth = 0;
	int mapHeight = 0;

	// 맵 데이터 (0: 빈공간/바닥, 1: 벽, 2: 터렛, 3: 아지트)
	std::vector<std::vector<int>> mapGrid;

public:
	bool CanBuildTurret(int x, int y);
};