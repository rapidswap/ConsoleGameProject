#pragma once

#include "Main/pch.h"
#include "Common/Protocol.h"
#include <map>
#include <mutex>
#include <memory>

class GameSession;

class GameRoom
{
public:
	GameRoom() = default;
	~GameRoom() = default;

	// 방 입장 및 퇴장.
	void Enter(std::shared_ptr<GameSession> session, const char* playerName);
	void Leave(std::shared_ptr < GameSession> session);


	// 방 안의 모든 플레이어에게 패킷 일괄 전송.
	void Broadcast(BYTE* buffer, int32_t len);

	// 패킷 처리 로직.
	void HandleChat(std::shared_ptr<GameSession> session, C_CHAT_PACKET& pkt);
	void HandleBuildTurret(std::shared_ptr<GameSession> session, C_BUILD_TURRET_PACKET& pkt);

private:
	// 멀티스레드 동시 접근 보호용 락.
	std::mutex lock;

	// 방에 있는 플레이어 목록.
	std::map<uint32_t, std::shared_ptr<GameSession>> sessions;

	// 플레이어 번호표(1부터 순차 증가).
	uint32_t playerIdGenerator = 1;
};

// 서버 전체에서 공유해서 쓸 단 하나의 방.
extern std::shared_ptr<GameRoom> GGameRoom;

