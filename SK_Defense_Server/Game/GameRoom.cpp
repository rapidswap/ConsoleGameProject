#include "GameRoom.h"
#include "Network/GameSession.h"
#include <iostream>

// 전역 게임 룸 인스턴스 생성.
std::shared_ptr<GameRoom> GGameRoom = std::make_shared<GameRoom>();

void GameRoom::Enter(std::shared_ptr<GameSession> session, const char* playerName)
{
	std::lock_guard<std::mutex> guard(lock);

	// 1. 고유 플레이어 번호 부여
	uint32_t newId = playerIdGenerator++;
	session->playerId = newId;

	// 2. 방 목록에 추가.
	sessions[newId] = session;

	std::cout << "[GameRoom] Player Enter -> ID: " << newId
		<< ", Name: " << playerName
		<< " (Current Players: " << sessions.size() << ")\n";

	// 3. 본인에게 로그인 성공 답장 보내기.
	S_LOGIN_OK_PACKET loginOkPkt;
	loginOkPkt.playerId = newId;
	loginOkPkt.currentGold = 100;
	session->Send(reinterpret_cast<BYTE*>(&loginOkPkt), loginOkPkt.size);

	// 4. 방에 있는 다른 모든 사람에게 시스템 채팅 방송.
	S_CHAT_PACKET alertPkt;
	alertPkt.playerId = 0;
	sprintf_s(alertPkt.msg, "[System] %s joined the room!", playerName);
	Broadcast(reinterpret_cast<BYTE*>(&alertPkt), alertPkt.size);

}

void GameRoom::Leave(std::shared_ptr<GameSession> session)
{
	std::lock_guard<std::mutex> guard(lock);
	uint32_t targetId = session->playerId;
	if (targetId == 0)
	{
		return;
	}

	// 방 목록에서 제거.
	sessions.erase(targetId);

	std::cout << "[GameRoom] Player Leave -> ID: " << targetId
		<< " (Current Players: " << sessions.size() << ")\n";

	// 방에 남은 사람들에게 퇴장 알림 방송.
	S_CHAT_PACKET alertPkt;
	alertPkt.playerId = 0;
	sprintf_s(alertPkt.msg, "[System] Player %d left the room.", targetId);
	Broadcast(reinterpret_cast<BYTE*>(&alertPkt), alertPkt.size);
}

void GameRoom::Broadcast(BYTE* buffer, int32_t len)
{
	// 방에 있는 모든 플레이어의 소켓으로 일제히 전송.
	for (auto& pair : sessions)
	{
		pair.second->Send(buffer, len);
	}
}

void GameRoom::HandleChat(std::shared_ptr<GameSession> session, C_CHAT_PACKET& pkt)
{
	std::lock_guard<std::mutex> guard(lock);

	std::cout << "[GameRoom Chat] Player " << session->playerId << ": " << pkt.msg << "\n";

	// 나한테만 보내는게 아닌 방에 모든 사람에게 방송.
	S_CHAT_PACKET sendPkt;
	sendPkt.playerId = session->playerId;
	strcpy_s(sendPkt.msg, pkt.msg);

	Broadcast(reinterpret_cast<BYTE*>(&sendPkt), sendPkt.size);
}

void GameRoom::HandleBuildTurret(std::shared_ptr<GameSession> session, C_BUILD_TURRET_PACKET& pkt)
{
	std::lock_guard<std::mutex> guard(lock);

	std::cout << "[GameRoom Build] Player " << session->playerId
		<< " built Turret at (" << pkt.posX << ", " << pkt.posY << ")\n";

	// 누군가 타워를 지었으면, 방 안의 모든 화면에 타워를 그려주도록 방송.
	S_BUILD_TURRET_PACKET sendPkt;
	sendPkt.success = true;
	sendPkt.playerId = session->playerId;
	sendPkt.posX = pkt.posX;
	sendPkt.posY = pkt.posY;
	sendPkt.turretType = pkt.turretType;
	sendPkt.remainingGold = 50;

	Broadcast(reinterpret_cast<BYTE*>(&sendPkt), sendPkt.size);
}
