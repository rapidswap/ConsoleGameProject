#include "ClientPacketHandler.h"
#include "GameSession.h"


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
	std::cout << "[Server] Login Request: " << pkt.playerName << "\n";

	// 로그인 성공 응답 패킷 제작.
	S_LOGIN_OK_PACKET sendPkt;
	// Temp ID.
	sendPkt.playerId = 1;
	sendPkt.currentGold = 100;

	// 클라이언트에게 전송.
	session->Send(reinterpret_cast<BYTE*>(&sendPkt), sendPkt.size);
}

void ClientPacketHandler::Handle_C_CHAT(std::shared_ptr<GameSession> session, C_CHAT_PACKET& pkt)
{
	std::cout << "[Server] Chat Received: " << pkt.msg << "\n";

	// 채팅 에코 답장 제작.
	S_CHAT_PACKET sendPkt;
	sendPkt.playerId = 1;
	::strcpy_s(sendPkt.msg, pkt.msg);

	session->Send(reinterpret_cast<BYTE*>(&sendPkt), sendPkt.size);
}

void ClientPacketHandler::Handle_C_BUILD_TURRET(std::shared_ptr<GameSession> session, C_BUILD_TURRET_PACKET& pkt)
{
	std::cout << "[Server] Turret Build Request -> Pos("
		<< pkt.posX << ", " << pkt.posY << ") Type: " << pkt.turretType << "\n";

	// 타워 설치 성공 결과 패킷 제작.
	S_BUILD_TURRET_PACKET sendPkt;
	sendPkt.success = true;
	sendPkt.playerId = 1;
	sendPkt.posX = pkt.posX;
	sendPkt.posY = pkt.posY;
	sendPkt.turretType = pkt.turretType;
	sendPkt.remainingGold = 50;

	session->Send(reinterpret_cast<BYTE*>(&sendPkt), sendPkt.size);
}
