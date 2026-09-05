#include "ClientPacketHandler.h"
#include "GameSession.h"
#include "Game/GameRoom.h"

void ClientPacketHandler::HandlePacket(std::shared_ptr<GameSession> session, BYTE* buffer, int32_t len)
{
	PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);

	switch (static_cast<PacketType>(header->id))
	{
	case PacketType::C_LOGIN:
		Handle_C_LOGIN(session, *reinterpret_cast<C_LOGIN_PACKET*>(buffer));
		break;

	case PacketType::C_CHAT:
		Handle_C_CHAT(session, *reinterpret_cast<C_CHAT_PACKET*>(buffer));
		break;

	case PacketType::C_BUILD_TURRET:
		Handle_C_BUILD_TURRET(session, *reinterpret_cast<C_BUILD_TURRET_PACKET*>(buffer));
		break;

	case PacketType::C_SELL_TURRET:
		Handle_C_SELL_TURRET(session, *reinterpret_cast<C_SELL_TURRET_PACKET*>(buffer));
		break;

	case PacketType::C_READY:
		Handle_C_READY(session, *reinterpret_cast<C_READY_PACKET*>(buffer));
		break;

	case PacketType::C_GAME_OVER:
		Handle_C_GAME_OVER(session, *reinterpret_cast<C_GAME_OVER_PACKET*>(buffer));
		break;

	case PacketType::C_LEAVE_ROOM:
		Handle_C_LEAVE_ROOM(session, *reinterpret_cast<C_LEAVE_ROOM_PACKET*>(buffer));
		break;

	case PacketType::C_SPEND_GOLD:
		Handle_C_SPEND_GOLD(session, *reinterpret_cast<C_SPEND_GOLD_PACKET*>(buffer));
		break;

	case PacketType::C_GAME_CLEAR:
		Handle_C_GAME_CLEAR(session, *reinterpret_cast<C_GAME_CLEAR_PACKET*>(buffer));
		break;

	default:
		std::cout << "[Server] Unknown Packet ID: " << header->id << "\n";
		break;
	}
}

void ClientPacketHandler::Handle_C_LOGIN(std::shared_ptr<GameSession> session, C_LOGIN_PACKET& pkt)
{
	GameRoomManager::Get()->EnterRoom(session, pkt.playerName);
}

void ClientPacketHandler::Handle_C_CHAT(std::shared_ptr<GameSession> session, C_CHAT_PACKET& pkt)
{
	if (auto room = session->GetRoom())
	{
		room->HandleChat(session, pkt);
	}
}

void ClientPacketHandler::Handle_C_BUILD_TURRET(std::shared_ptr<GameSession> session, C_BUILD_TURRET_PACKET& pkt)
{
	if (auto room = session->GetRoom())
	{
		room->HandleBuildTurret(session, pkt);
	}
}

void ClientPacketHandler::Handle_C_SELL_TURRET(std::shared_ptr<GameSession> session, C_SELL_TURRET_PACKET& pkt)
{
	if (auto room = session->GetRoom())
	{
		room->HandleSellTurret(session, pkt);
	}
}

void ClientPacketHandler::Handle_C_READY(std::shared_ptr<GameSession> session, C_READY_PACKET& pkt)
{
	if (auto room = session->GetRoom())
	{
		room->HandleReady(session, pkt);
	}
}

void ClientPacketHandler::Handle_C_GAME_OVER(std::shared_ptr<GameSession> session, C_GAME_OVER_PACKET& pkt)
{
	if (auto room = session->GetRoom())
	{
		room->HandleGameOver();
	}
}

void ClientPacketHandler::Handle_C_LEAVE_ROOM(std::shared_ptr<GameSession> session, C_LEAVE_ROOM_PACKET& pkt)
{
	if (auto room = session->GetRoom())
	{
		room->HandleLeaveRoom(session);
	}
}

void ClientPacketHandler::Handle_C_SPEND_GOLD(std::shared_ptr<GameSession> session, C_SPEND_GOLD_PACKET& pkt)
{
	if (auto room = session->GetRoom())
	{
		room->HandleSpendGold(session, pkt);
	}
}

void ClientPacketHandler::Handle_C_GAME_CLEAR(std::shared_ptr<GameSession> session, C_GAME_CLEAR_PACKET& pkt)
{
	if (auto room = session->GetRoom())
	{
		room->HandleGameClear(session, pkt);
	}
}
