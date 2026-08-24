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

	// 스폰 지점(S)과 목표 지점(D)
	Craft::Vector2 spawnPoint;
	Craft::Vector2 targetPoint;

public:
	bool CanBuildTurret(int x, int y);
	
	std::vector<std::vector<int>>& GetMapGrid() { return mapGrid; }
	Craft::Vector2 GetSpawnPoint() const { return spawnPoint; }
	Craft::Vector2 GetTargetPoint() const { return targetPoint; }
};