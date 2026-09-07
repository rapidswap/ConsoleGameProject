#pragma once

#include <Level/Level.h>
#include <Math/Vector2.h>
#include <vector>
#include <string>
#include <Actor/Turret.h>

class DefenseLevel : public Craft::Level
{
public:
	DefenseLevel();
	virtual ~DefenseLevel();

	virtual void OnInitialized();
	virtual void Tick(float deltaTime) override;
	virtual void Draw() override;

private:
	Craft::Vector2 GetRealMousePos();
	void LoadMap(const std::string& filename);
	void CheckTurretMerge();

	// Tick 함수 정리를 위한 입력 처리 분리
	void HandleCameraInput();
	void HandleMouseInput();
	void HandleUIInput();
	
	void RandomUpgrade();
	void Gamble();
	void DrawGambleAnimation();
	void DrawGameInfo(const std::string& filename);

public:
	void GameClear();
	void OnGameClearReceived();

private:
	int mapWidth = 0;
	int mapHeight = 0;

	bool isGambling = false;
	float gambleTimer = 0.0f;
	int gambleResults[3] = {0, 0, 0};

	bool isGameInfo = false;
	bool showAStarDebug = false;


	// 맵 데이터 (0: 빈공간/바닥, 1: 벽, 2: 터렛, 3: 아지트)
	std::vector<std::vector<int>> mapGrid;

	// 스폰 지점(S) 배열과 목표 지점(D)
	std::vector<Craft::Vector2> spawnPoints;
	Craft::Vector2 targetPoint;

	// 웨이브 관리를 위한 스포너 참조
	std::weak_ptr<class EnemySpawner> enemySpawner;

public:
	// 어디서든 현재 DefenseLevel에 접근할 수 있는 싱글톤 포인터
	static DefenseLevel* Get() { return s_instance; }

	// 서버로부터 타워 건설 패킷을 받았을 때 호출할 함수
	void BuildTurretFromNetwork(int x, int y, int turretType);

	// 서버로부터 타워 철거 패킷을 받았을 때 호출할 함수
	void SellTurretFromNetwork(int x, int y, uint32_t sellerPlayerId, int refundGold);

	// 서버로부터 몬스터 소환 패킷을 받았을 때 호출할 함수
	void SpawnMonsterFromNetwork(int monsterId, int spawnIndex, int maxHp, float speed);

	// 서버로부터 몬스터 처치 패킷을 받았을 때 호출할 함수
	void KillMonsterFromNetwork(int monsterId, int rewardGold);

	// 서버로부터 타워 속성 업그레이드 패킷을 받았을 때 호출할 함수
	void UpgradeTurretFromNetwork(int upgradeType, int newLevel);

	// 서버로부터 아지트 체력 동기화 패킷을 받았을 때 호출할 함수
	void SetAgitHealthFromNetwork(int health);

	// 몬스터가 아지트에 도달했을 때 호출 (네트워크 연결 여부에 따라 패킷 전송 or 로컬 차감)
	void DamageAgit(int damage = 1);

	void GameOver();

	bool IsAStarDebug() const { return showAStarDebug; }
	bool CanBuildTurret(int x, int y);
	
	// 스폰 지점 관리용.
	inline Craft::Vector2 GetSpawnPoint(int index) const 
	{ 
		if (index >= 0 && index < spawnPoints.size()) return spawnPoints[index]; 
		return spawnPoints.empty() ? Craft::Vector2(0,0) : spawnPoints[0];
	}
	inline int GetSpawnPointCount() const { return (int)spawnPoints.size(); }
	
	// 타겟 지점 접근용.
	Craft::Vector2 GetTargetPoint() const { return targetPoint; }
	const std::vector<std::vector<int>>& GetMapGrid() const { return mapGrid; }
	std::vector<std::vector<int>>& GetMapGrid() { return mapGrid; }


	// 골드 시스템 접근용
	int GetTurretCost() const { return turretCost; }
	int GetGold() const { return currentGold; }
	void SetGold(int amount) { currentGold = amount; }
	void AddGold(int amount, bool syncServer = true);
	bool SpendGold(int amount);
	int GetTotalGoldSpent() const { return totalGoldSpent; }

	void SetNextTurretType(int32_t type) { nextTurretType = static_cast<TurretType>(type); }

	// 재화 (골드)
	int currentGold = 300;
	// 누적 소비 골드
	int totalGoldSpent = 0;
	// 터렛 설치 비용
	const int turretCost = 50;

	// 다음에 설치될 터렛의 타입
	TurretType nextTurretType = TurretType::FLAME;

	// 멀티플레이어 세션 여부
	bool isMultiplayerGame = false;

private:
	static inline DefenseLevel* s_instance = nullptr;
};