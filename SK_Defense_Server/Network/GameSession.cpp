#include "GameSession.h"
#include "ClientPacketHandler.h"
#include "Game/GameRoom.h"
#include <iostream>
void GameSession::OnConnected()
{
    std::wcout << L"[Server] Client Connected! IP: "
        << GetNetAddress().GetIpAddress() << L" Port: "
        << GetNetAddress().GetPort() << "\n";
}

int32_t GameSession::OnRecv(BYTE* buffer, int32_t len)
{
    // 수신한 패킷을 패킷 핸들러에게 그대로 넘겨서 분류/처리.
    ClientPacketHandler::HandlePacket(std::static_pointer_cast<GameSession>(shared_from_this()), buffer, len);
    return len;
}

void GameSession::OnSend(int32_t len)
{
    // 송신 완료 로그.
    std::cout << "[Server] Send Complete (" << len << " bytes)\n";
}

void GameSession::OnDisconnected()
{
    std::cout << "[Server] Client Disconnected! (PlayerID: " << playerId << ")\n";

    GGameRoom->Leave(std::static_pointer_cast<GameSession>(shared_from_this()));
}
