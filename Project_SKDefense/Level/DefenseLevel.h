#pragma once

#include <Level/Level.h>
#include <Math/Vector2.h>
#include <vector>
#include <string>

class DefenseLevel : public Craft::Level
{
public:
	DefenseLevel();
	~DefenseLevel() = default;

	virtual void Update(float deltaTime) override;
	virtual void Draw() override;

private:
	// 맵 로드 함수
	void LoadMap();

private:
	// 맵 데이터
	std::vector<std::string> mapData;
	
	// 카메라 오프셋
	Craft::Vector2 cameraOffset;
};
