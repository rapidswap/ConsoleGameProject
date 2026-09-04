#pragma once

#include "Main/pch.h"
#include "Network/Session.h"

class GameSession : public Session
{
public:
	GameSession() = default;
	virtual ~GameSession() = default;

	virtual void OnConnected() override;
	virtual int32_t OnRecv(BYTE* buffer, int32_t len)override;
	virtual void OnSend(int32_t len) override;
	virtual void OnDisconnected() override;

public:
	// 이 세션의 고유 플레이어 ID.
	uint32_t playerId = 0;
	// 서버에서 관리하는 플레이어의 골드.
	int32_t gold = 100;
};