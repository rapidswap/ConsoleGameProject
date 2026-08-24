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
	void LoadMap(const std::string& filename);

private:
	int mapWidth = 0;
	int mapHeight = 0;
};
