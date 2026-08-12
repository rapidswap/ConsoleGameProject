#pragma once

#include <Actor/Actor.h>
#include <Util/Timer.h>

// 좌우로 이동/스페이스 키로 탄약을 발사하는 플레이어.
class Player:public Craft::Actor
{
	// 커스텀 타입 등록.
	TYPE_DECLARATIONS(Player,Actor)

public:
	Player();
	
	// GameLevel 등 외부에서 플레이어의 정보를 읽어갈 수 있도록 Getter 추가
	inline int GetLevel() const { return playerLevel; }
	inline float GetEXP() const { return playerEXP; }
	inline float GetTargetEXP() const { return targetEXP; }
	inline int GetHp() const { return playerHp; }
	inline int GetMaxHp() const { return maxHp; }
	inline int GetAttackSpeed() const { return autoFireInterval; }
	
	// 증강으로 인한 플레이어의 정보 변경.
	inline void PlayerSpeedUp() { moveSpeed += 1.0f; }
	inline void PlayerExpUp() { EXP *= 1.1f; }
	inline void AddProjectile() { projectileCount++; }
	inline void AttackSpeedUp() {
		autoFireInterval -= 0.5f;
		timer.SetTargetTime(autoFireInterval);
	}
	inline void hpDown() { playerHp--; }
	inline void hpUp() { playerHp++; }
	inline void MaxHpUp() { maxHp++; playerHp++; }
	
private:
	// 이벤트 함수 오버라이드.
	virtual void Tick(float deltaTime) override;
	virtual void Draw() override;

	// 충돌 이벤트 함수 오버라이드.
	virtual void OnCollision(const std::shared_ptr<Actor>& other)override;

	// 이동 처리 함수.
	void xMove(float direction, float deltaTime);
	void yMove(float direction, float deltaTime);

	// 탄약 발사 관련 함수 및 변수들은 제거됨.

private:
	// 이동 처리에 필요한 변수.
	float xPosition = 0.0f;
	float yPosition = 0.0f;

	// 이동 속도 변수.
	float moveSpeed = 20.0f;

	// 타이머 변수.
	Timer timer;

	// 자동 발사 간격(단위: 초).
	float autoFireInterval = 2.0f;

	// 플레이어 체력.
	int playerHp = 3;
	int maxHp = 3;

	// 플레이어 레벨.
	int playerLevel = 1;
	
	// 플레이어의 경험치.
	float playerEXP = 0.0f;

	// 레벨업 목표
	float targetEXP = 10.0f;

	// 증강 업글을 위한 경험치.
	float EXP = 1.0f;

	// 한 번에 발사할 총알의 개수
	int projectileCount = 1;

	// 발사 모드 결정.
	int bulletMode = 1;

	// 일직선 연사 모드를 위한 변수.
	Timer burstTimer;
	int pendingBullets = 0;

	// 피격 시 무적 판정을 위한 변수
	bool isInvincible = false;
	bool isVisible = true;
	Timer invincibilityTimer;
	Timer blinkTimer;
};

