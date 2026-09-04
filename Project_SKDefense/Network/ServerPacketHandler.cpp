#include "ServerPacketHandler.h"
#include "NetworkManager.h"
#include "Level/DefenseLevel.h"

#include <iostream>

void ServerPacketHandler::HandlePacket(BYTE* buffer, int32_t len)
{
	PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);

	switch (static_cast<PacketType>(header->id))
	{
	case PacketType::S_LOGIN_OK:
		Handle_S_LOGIN_OK(*reinterpret_cast<S_LOGIN_OK_PACKET*>(buffer));
		break;

	case PacketType::S_CHAT:
		Handle_S_CHAT(*reinterpret_cast<S_CHAT_PACKET*>(buffer));
		break;

	case PacketType::S_BUILD_TURRET:
		Handle_S_BUILD_TURRET(*reinterpret_cast<S_BUILD_TURRET_PACKET*>(buffer));
		break;

	case PacketType::S_SELL_TURRET:
		Handle_S_SELL_TURRET(*reinterpret_cast<S_SELL_TURRET_PACKET*>(buffer));
		break;

	case PacketType::S_SPAWN_MONSTER:
		Handle_S_SPAWN_MONSTER(*reinterpret_cast<S_SPAWN_MONSTER_PACKET*>(buffer));
		break;

	case PacketType::S_ROOM_INFO:
	{
		auto pkt = reinterpret_cast<S_ROOM_INFO_PACKET*>(buffer);
		NetworkManager::Get()->SetPlayerCount(pkt->totalPlayers);
		NetworkManager::Get()->SetReadyPlayerCount(pkt->readyPlayers);
		break;
	}

	case PacketType::S_GAME_START:
		// 서버로부터 동시 시작 신호 수신.
		NetworkManager::Get()->SetGameStartTriggered(true);
		break;

	case PacketType::S_GAME_OVER:
		Handle_S_GAME_OVER(*reinterpret_cast<S_GAME_OVER_PACKET*>(buffer));
		break;

	default:
		break;
	}
}

void ServerPacketHandler::Handle_S_GAME_OVER(S_GAME_OVER_PACKET& pkt)
{
	std::cout << "[Client] Received S_GAME_OVER from server!\n";
	if (DefenseLevel::Get())
	{
		DefenseLevel::Get()->GameOver();
	}
}

void ServerPacketHandler::Handle_S_LOGIN_OK(S_LOGIN_OK_PACKET& pkt)
{
	// 서버가 발급해 준 고유 번호 및 시작 골드 저장.
	NetworkManager::Get()->SetMyPlayerId(pkt.playerId);
	NetworkManager::Get()->SetStartGold(pkt.currentGold);
	std::cout << "[Client] Login Success! My PlayerID: " << pkt.playerId
		<< ", Start Gold: " << pkt.currentGold << "\n";
}

void ServerPacketHandler::Handle_S_CHAT(S_CHAT_PACKET& pkt)
{
	if (pkt.playerId == 0)
	{
		std::cout << "[System Alert] " << pkt.msg << "\n";
	}
	else
	{
		std::cout << "[Chat] Player " << pkt.playerId << ": " << pkt.msg << "\n";
	}
}

void ServerPacketHandler::Handle_S_BUILD_TURRET(S_BUILD_TURRET_PACKET& pkt)
{
	if (!pkt.success)
	{
		std::cout << "[Client] Turret Build Failed (Not enough gold or invalid position)\n";
		return;
	}

	if (DefenseLevel* level = DefenseLevel::Get())
	{
		level->BuildTurretFromNetwork(pkt.posX, pkt.posY, pkt.turretType);

		// 내가 건설한 경우 서버에서 검증된 최신 잔여 골드로 동기화
		if (pkt.playerId == NetworkManager::Get()->GetMyPlayerId())
		{
			level->SetGold(pkt.remainingGold);
		}

		std::cout << "[Client] Turret Built by Player " << pkt.playerId
			<< " at (" << pkt.posX << ", " << pkt.posY << ")"
			<< " Type: " << pkt.turretType
			<< " Remaining Gold: " << pkt.remainingGold << "\n";
	}
}

void ServerPacketHandler::Handle_S_SELL_TURRET(S_SELL_TURRET_PACKET& pkt)
{
	if (!pkt.success)
		return;

	if (DefenseLevel* level = DefenseLevel::Get())
	{
		level->SellTurretFromNetwork(pkt.posX, pkt.posY, pkt.playerId, pkt.refundGold);
		std::cout << "[Client] Turret Sold by Player " << pkt.playerId
			<< " at (" << pkt.posX << ", " << pkt.posY << ")"
			<< " Refund: " << pkt.refundGold << "G\n";
	}
}

void ServerPacketHandler::Handle_S_SPAWN_MONSTER(S_SPAWN_MONSTER_PACKET& pkt)
{
	if (DefenseLevel* level = DefenseLevel::Get())
	{
		level->SpawnMonsterFromNetwork(pkt.spawnIndex, pkt.maxHP, pkt.speed);
	}
	
}
