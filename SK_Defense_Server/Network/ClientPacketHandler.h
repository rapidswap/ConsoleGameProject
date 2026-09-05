#pragma once
#include "Main/pch.h"
#include "Common/Protocol.h"

// 순환 참조 방지 위한 전방 선언.
class GameSession;

class ClientPacketHandler
{
public:
	// 패킷이 들어왔을 떄 맨 처음 호출되는 교통정리 함수.
	static void HandlePacket(std::shared_ptr<GameSession> session, BYTE* buffer, int32_t len);

private:
	// 각각의 패킷별 세부 처리 함수.
	static void Handle_C_LOGIN(std::shared_ptr<GameSession> session, C_LOGIN_PACKET& pkt);
	static void Handle_C_CHAT(std::shared_ptr<GameSession> session, C_CHAT_PACKET& pkt);
	static void Handle_C_BUILD_TURRET(std::shared_ptr<GameSession> session, C_BUILD_TURRET_PACKET& pkt);
	static void Handle_C_SELL_TURRET(std::shared_ptr<GameSession> session, C_SELL_TURRET_PACKET& pkt);
	static void Handle_C_READY(std::shared_ptr<GameSession> session, C_READY_PACKET& pkt);
	static void Handle_C_GAME_OVER(std::shared_ptr<GameSession> session, C_GAME_OVER_PACKET& pkt);
	static void Handle_C_LEAVE_ROOM(std::shared_ptr<GameSession> session, C_LEAVE_ROOM_PACKET& pkt);
	static void Handle_C_SPEND_GOLD(std::shared_ptr<GameSession> session, C_SPEND_GOLD_PACKET& pkt);
	static void Handle_C_GAME_CLEAR(std::shared_ptr<GameSession> session, C_GAME_CLEAR_PACKET& pkt);
};