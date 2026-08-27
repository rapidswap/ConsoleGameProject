#pragma once
#include <Actor/Actor.h>
#include <Util/Timer.h>

enum class TurretType
{
	FLAME = 0, // 화염 (빨강, F)
	ICE = 1,   // 얼음 (시안, I)
	STORM = 2  // 전기 (노랑, T)
};

class Turret : public Craft::Actor
{
	TYPE_DECLARATIONS(Turret, Actor)
public:
	Turret(const Craft::Vector2& position, TurretType type);
	virtual ~Turret() = default;

	virtual void Draw() override;
	virtual void Tick(float deltaTime) override;

	// 성급 업그레이드 (1성 -> 2성 -> 3성)
	void UpgradeStar();

	// Getter
	TurretType GetTurretType() const { return turretType; }
	int GetStarTier() const { return starTier; }
	int GetSpawnOrder() const { return spawnOrder; }

private:
	void UpdateStats();

private:
	// 터렛 속성
	TurretType turretType = TurretType::STORM;
	int starTier = 1;      // 1성, 2성, 3성
	int spawnOrder = 0;    // 생성 순서 (작을수록 먼저 지어짐)

	// 외형 텍스트 (2x2 표시용)
	std::string displaySymbol = "TT";

	// 자동 공격 타이머.
	Timer autoFireInterval;

	// 터렛의 공격 속도 (초 단위).
	float atkSpeed = 1.0f;
	
	// 터렛의 인지/공격 사거리.
	float atkRange = 10.0f;
};

