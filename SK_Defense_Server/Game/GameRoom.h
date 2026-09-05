#pragma once

#include "Main/pch.h"
#include "Common/Protocol.h"
#include <map>
#include <set>
#include <vector>
#include <mutex>
#include <memory>

class GameSession;

// 방 상태.
enum class RoomState
{
	WAITING,
	PLAYING
};

class GameRoom : public std::enable_shared_from_this<GameRoom>
{
public:
	explicit GameRoom(uint32_t inRoomId = 1) : roomId(inRoomId) {}
	~GameRoom() = default;

	uint32_t GetRoomId() const { return roomId; }
	RoomState GetState();
	size_t GetSessionCount();
	bool IsEmpty();

	// 방 입장 및 퇴장.
	void Enter(std::shared_ptr<GameSession> session, const char* playerName);
	void Leave(std::shared_ptr<GameSession> session);

	// 방 안의 모든 플레이어에게 패킷 일괄 전송.
	void Broadcast(BYTE* buffer, int32_t len);

	void HandleReady(std::shared_ptr<GameSession> session, C_READY_PACKET& pkt);
	void HandleGameOver();
	void HandleLeaveRoom(std::shared_ptr<GameSession> session);
	void BroadcastRoomInfo();
	void StartGame();

	// 패킷 처리 로직.
	void HandleChat(std::shared_ptr<GameSession> session, C_CHAT_PACKET& pkt);
	void HandleBuildTurret(std::shared_ptr<GameSession> session, C_BUILD_TURRET_PACKET& pkt);
	void HandleSellTurret(std::shared_ptr<GameSession> session, C_SELL_TURRET_PACKET& pkt);
	void HandleSpendGold(std::shared_ptr<GameSession> session, C_SPEND_GOLD_PACKET& pkt);
	void HandleGameClear(std::shared_ptr<GameSession> session, C_GAME_CLEAR_PACKET& pkt);

	void Update(float deltaTime);
	void SpawnMonster(int32_t spawnIndex, int32_t hp, float speed);

private:
	uint32_t roomId = 1;

	// 멀티스레드 동시 접근 보호용 락.
	std::mutex lock;

	// 방에 있는 플레이어 목록.
	std::map<uint32_t, std::shared_ptr<GameSession>> sessions;

	// 플레이어 번호표(1부터 순차 증가).
	uint32_t playerIdGenerator = 1;

	RoomState state = RoomState::WAITING;
	std::set<uint32_t> readyPlayerIds;

	// 준비 시간.
	float waveTimer = 30.0f;
	// 웨이브 진행 여부.
	bool isWaveActive = false;
	// 몬스터 소환 주기 타이머.
	float spawnTimer = 0.0f;
	// 이번 웨이브에 소환된 수.
	int32_t spawnedCount = 0;
	// 웨이브당 30마리.
	const int32_t maxPerWave = 30;
	// 현재 웨이브 번호.
	int32_t waveCount = 1;
};

class GameRoomManager
{
public:
	static GameRoomManager* Get()
	{
		static GameRoomManager instance;
		return &instance;
	}

	void EnterRoom(std::shared_ptr<GameSession> session, const char* playerName);
	void Update(float deltaTime);

private:
	std::mutex managerLock;
	std::vector<std::shared_ptr<GameRoom>> rooms;
	uint32_t nextRoomId = 1;
};
