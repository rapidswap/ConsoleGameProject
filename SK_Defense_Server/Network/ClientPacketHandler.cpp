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

	default:
		std::cout << "[Server] Unknown Packet ID: " << header->id << "\n";
		break;
	}
}

void ClientPacketHandler::Handle_C_LOGIN(std::shared_ptr<GameSession> session, C_LOGIN_PACKET& pkt)
{
	GGameRoom->Enter(session, pkt.playerName);
}

void ClientPacketHandler::Handle_C_CHAT(std::shared_ptr<GameSession> session, C_CHAT_PACKET& pkt)
{
	GGameRoom->HandleChat(session, pkt);
}

void ClientPacketHandler::Handle_C_BUILD_TURRET(std::shared_ptr<GameSession> session, C_BUILD_TURRET_PACKET& pkt)
{
	GGameRoom->HandleBuildTurret(session, pkt);
}
