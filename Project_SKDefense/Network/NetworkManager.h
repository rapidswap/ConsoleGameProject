#pragma once

#include "Common/Protocol.h"

#define WIN32_LEAN_AND_MEAN
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <thread>
#include <mutex>
#include <queue>
#include <vector>
#include <memory>

#pragma comment(lib, "ws2_32.lib")

class NetworkManager
{
public:
	// 싱글톤 인스턴스 접근.
	static NetworkManager* Get()
	{
		static NetworkManager instance;
		return &instance;
	}

	NetworkManager() = default;
	~NetworkManager();

	// 1. 서버 접속 및 백그라운드 수신 스레드 시작.
	bool Connect(const wchar_t* ip, uint16_t port);
	void Disconnect();

	// 2. 서버로 패킷 전송.
	void Send(BYTE* buffer, int32_t len);

	// 3. 매 프레임(Tick)마다 메인 스레드에서 호출되어 쌓인 패킷들을 처리하는 함수.
	void Update();

	// 레디 패킷 전송 헬퍼
	void SendReady(bool forceStart = false)
	{
		C_READY_PACKET pkt;
		pkt.forceStart = forceStart;
		Send(reinterpret_cast<BYTE*>(&pkt), pkt.size);
	}


	bool IsConnected() const { return connected; }
	uint32_t GetMyPlayerId() const { return myPlayerId; }
	void SetMyPlayerId(uint32_t id) { myPlayerId = id; }
	int32_t GetPlayerCount() const { return playerCount; }
	void SetPlayerCount(int32_t count) { playerCount = count; }

	int32_t GetReadyPlayerCount() const { return readyPlayerCount; }
	void SetReadyPlayerCount(int32_t count) { readyPlayerCount = count; }

	bool IsGameStartTriggered()const { return gameStartTriggered; }
	void SetGameStartTriggered(bool flag) { gameStartTriggered = flag; }

	int32_t GetStartGold() const { return startGold; }
	void SetStartGold(int32_t gold) { startGold = gold; }

private:
	// 백그라운드에서 패킷을 감시하는 수신 스레드 루프.
	void RecvThread();

private:
	SOCKET socket = INVALID_SOCKET;
	bool connected = false;
	std::thread recvThread;

	// 서버가 발급해주는 고유 플레이어 번호.
	uint32_t myPlayerId = 0;

	// 멀티스레드 패킷 큐 (수신 스레드가 넣기 -> 메인 게임 스레드가 꺼냄).
	std::mutex queueLock;
	std::queue<std::vector<BYTE>> packetQueue;

	int32_t playerCount = 1;

	int32_t readyPlayerCount = 0;

	bool gameStartTriggered = false;

	int32_t startGold = 100;
};
