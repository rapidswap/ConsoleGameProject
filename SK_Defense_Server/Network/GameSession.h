#pragma once

#include "Main/pch.h"
#include "Network/Session.h"
#include <memory>

class GameRoom;

class GameSession : public Session
{
public:
	GameSession() = default;
	virtual ~GameSession() = default;

	virtual void OnConnected() override;
	virtual int32_t OnRecv(BYTE* buffer, int32_t len) override;
	virtual void OnSend(int32_t len) override;
	virtual void OnDisconnected() override;

	std::shared_ptr<GameRoom> GetRoom() const { return room.lock(); }
	void SetRoom(std::shared_ptr<GameRoom> inRoom) { room = inRoom; }

public:
	// 이 세션의 고유 플레이어 ID.
	uint32_t playerId = 0;
	// 플레이어 닉네임.
	char playerName[32] = {};
	// 서버에서 관리하는 플레이어의 골드.
	int32_t gold = 100;
	// 사용한 총 골드 (클리어 랭킹 산출용)
	int32_t totalGoldSpent = 0;

private:
	std::weak_ptr<GameRoom> room;
};