#include "NetworkManager.h"
#include "ServerPacketHandler.h"

#include <iostream>

NetworkManager::~NetworkManager()
{
	Disconnect();
}

bool NetworkManager::Connect(const wchar_t* ip, uint16_t port)
{
	// 이미 연결되어 있다면 재연결 없이 통과
	if (connected)
		return true;

	Disconnect();

	// 1. Winsock 초기화.
	WSADATA wsaData;
	if (::WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		return false;
	}

	// 2. 소켓 생성.
	socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (socket == INVALID_SOCKET)
	{
		return false;
	}

	// 3. 서버 주소 설정.
	SOCKADDR_IN serverAddr = {};
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = ::htons(port);
	::InetPtonW(AF_INET, ip, &serverAddr.sin_addr);

	// 4. 서버에 연결 시도.
	if (::connect(socket, reinterpret_cast<SOCKADDR*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR)
	{
		::closesocket(socket);
		socket = INVALID_SOCKET;
		return false;
	}

	connected = true;

	// 5. 백그라운드 수신 스레드 가동.
	recvThread = std::thread(&NetworkManager::RecvThread, this);

	return true;
}

void NetworkManager::Disconnect()
{
	connected = false;

	if (socket != INVALID_SOCKET)
	{
		::closesocket(socket);
		socket = INVALID_SOCKET;
	}

	if (recvThread.joinable())
	{
		recvThread.join();
	}

	::WSACleanup();
}

void NetworkManager::Send(BYTE* buffer, int32_t len)
{
	if (!connected || socket == INVALID_SOCKET)
	{
		return;
	}

	// 서버로 패킷 전송.
	::send(socket, reinterpret_cast<const char*>(buffer), len, 0);
}

void NetworkManager::Update()
{
	// 메인 스레드에서 매 프레임 호출: 큐에 쌓인 패킷들을 싹 꺼내오기.
	std::vector<std::vector<BYTE>> packets;
	{
		std::lock_guard<std::mutex> lock(queueLock);
		while (!packetQueue.empty())
		{
			packets.emplace_back(std::move(packetQueue.front()));
			packetQueue.pop();
		}
	}

	// 꺼내온 패킷들을 순서대로 처리 -> ServerPacketHandler에게 전달.
	for (auto& packet : packets)
	{
		ServerPacketHandler::HandlePacket(packet.data(), static_cast<int32_t>(packet.size()));
	}
}

void NetworkManager::RecvThread()
{
	BYTE buffer[65536];
	int32_t readPos = 0;
	int32_t writePos = 0;

	while (connected)
	{
		// 빈 공간에 서버로부터 데이터 수신.
		int32_t freeSize = sizeof(buffer) - writePos;
		if (freeSize <= 0)
		{
			// 버퍼 가득차는 것을 방지: 남은 데이터 맨 앞으로 당기기.
			int32_t dataSize = writePos - readPos;
			::memmove(&buffer[0], &buffer[readPos], dataSize);
			readPos = 0;
			writePos = dataSize;
			freeSize = sizeof(buffer) - writePos;
		}

		int32_t numOfBytes = ::recv(socket, reinterpret_cast<char*>(&buffer[writePos]), freeSize, 0);
		if (numOfBytes <= 0)
		{
			// 서버 연결 끊김.
			connected = false;
			break;
		}

		writePos += numOfBytes;

		// 패킷 조립 루프
		int32_t dataSize = writePos - readPos;
		int32_t processLen = 0;

		while (true)
		{
			int32_t currentSize = dataSize - processLen;
			if (currentSize < sizeof(PacketHeader))
			{
				break;
			}

			PacketHeader* header = reinterpret_cast<PacketHeader*>(&buffer[readPos + processLen]);
			if (currentSize < header->size)
			{
				// 덜 왔으면 다음 수신 대기.
				break;
			}

			// 온전한 패킷을 패킷 큐에 저장.
			{
				std::vector<BYTE> packetData(header->size);
				::memcpy(packetData.data(), &buffer[readPos + processLen], header->size);
				std::lock_guard<std::mutex> lock(queueLock);
				packetQueue.push(std::move(packetData));
			}
			processLen += header->size;
		}
		readPos += processLen;

		// 데이터를 다 소비했으면 커서 리셋.
		if (readPos == writePos)
		{
			readPos = writePos = 0;
		}
	}
}
