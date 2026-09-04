#pragma once

#include "Common/Protocol.h"

class ServerPacketHandler
{
public:
	// 큐에서 꺼낸 패킷을 ID별로 분류해 주는 진입점.
	static void HandlePacket(BYTE* buffer, int32_t len);

	// 세부 패킷 처리기.
	static void Handle_S_LOGIN_OK(S_LOGIN_OK_PACKET& pkt);
	static void Handle_S_CHAT(S_CHAT_PACKET& pkt);
	static void Handle_S_BUILD_TURRET(S_BUILD_TURRET_PACKET& pkt);
	static void Handle_S_SELL_TURRET(S_SELL_TURRET_PACKET& pkt);
	static void Handle_S_SPAWN_MONSTER(S_SPAWN_MONSTER_PACKET& pkt);
	static void Handle_S_GAME_OVER(S_GAME_OVER_PACKET& pkt);
};