#include "Session.h"
#include "Common/Protocol.h"

Session::Session()
{
}

Session::~Session()
{
	if (socket != INVALID_SOCKET)
	{
		::closesocket(socket);
		socket = INVALID_SOCKET;
	}
}

void Session::Dispatch(IocpEvent* iocpEvent, int32_t numOfBytes)
{
	// IOCP에서 완료 통지가 오면 이벤트 타입에 따라 분기 처리.
	switch (iocpEvent->eventType)
	{
	case EventType::Recv:
		ProcessRecv(numOfBytes);
		break;
	case EventType::Send:
		ProcessSend(numOfBytes);
		break;
	default:
		break;
	}
}

void Session::Disconnect(const WCHAR* cause)
{
	if (!connected)
	{
		return;
	}

	connected = false;

	ProcessDisconnect();
}

void Session::ProcessConnect()
{
	connected = true;

	// 게임 로직에 접속완료.
	OnConnected();

	// 접속되자마자 첫 번째 수신을 예약.
	RegisterRecv();
}

void Session::RegisterRecv()
{
	if (!IsConnected())
	{
		return;
	}

	recvEvent.Init();
	// 완료될 떄까지 세션이 메모리에서 안날아가게 잡기.
	recvEvent.owner = shared_from_this();

	// 1. 버퍼 청소.
	recvBuffer.Clean();

	// 2. OS에게 writePos를 FreeSize만큼 채우라고 요청.
	WSABUF wsaBuf;
	wsaBuf.buf = reinterpret_cast<char*>(recvBuffer.WritePos());
	wsaBuf.len = recvBuffer.FreeSize();

	DWORD numOfBytes = 0;
	DWORD flags = 0;

	// OS에게 비동기 수신 요청(데이터 오면 IOCP 완료 큐에 넣기).
	if (::WSARecv(socket, &wsaBuf, 1, &numOfBytes, &flags, &recvEvent, nullptr) == SOCKET_ERROR)
	{
		int32_t errorCode = ::WSAGetLastError();

		// WSA_IO_PENDING: 당장 데이터가 없으니 받으면 알려줄게 -> 정상.
		if (errorCode != WSA_IO_PENDING)
		{
			Disconnect(L"WSARecv Error");
		}
	}
}

void Session::ProcessRecv(int32_t numOfBytes)
{
	auto self = shared_from_this(); // 처리 도중 세션 소멸 방지
	recvEvent.owner = nullptr;

	if (numOfBytes == 0)
	{
		// 0 바이트가 왔다는 것은 상대방이 정상적으로 연결을 끊었다는 신호.
		Disconnect(L"Recv 0 bytes (Disconnected)");
		return;
	}

	// 1. 수신한 만큼 버퍼의 쓰기 커서 전진.
	if (recvBuffer.OnWrite(numOfBytes) == false)
	{
		Disconnect(L"OnWirte Overflow");
		return;
	}

	// 2. 뭉치거나 쪼개진 패킷들을 하나의 크기로 잘라먹는 루프.
	int32_t dataSize = recvBuffer.DataSize();
	int32_t processLen = 0;

	while (true)
	{
		int32_t currentSize = dataSize - processLen;

		// 최소한 헤더 크기 만큼 도착했는지 확인.
		if (currentSize < sizeof(PacketHeader))
		{
			break;
		}

		// 헤더를 까서 패킷 전체 크기를 확인.
		PacketHeader* header = reinterpret_cast<PacketHeader*>(recvBuffer.ReadPos() + processLen);

		// 아직 패킷 내용이 다 도착안했으면 다음 수신을 기다림.
		if (currentSize < header->size)
		{
			break;
		}

		// 온전한 패킷이 완성되었으면 게임로직 전달.
		OnRecv(recvBuffer.ReadPos() + processLen, header->size);

		// 처리한 만큼 누적.
		processLen += header->size;
				
	}

	// 3. 처리 완료된 패킷들만큼 읽기 커서 전진.
	if (processLen > 0)
	{
		recvBuffer.OnRead(processLen);
	}

	// 데이터를 받았으면 다음 데이터를 받기위해 다시 수신 등록.
	RegisterRecv();
}

void Session::Send(BYTE* buffer, int32_t len)
{
	if (!IsConnected() || len > sizeof(sendBuffer))
	{
		return;
	}

	std::lock_guard<std::mutex> lock(sendLock);

	::memcpy(sendBuffer, buffer, len);

	sendEvent.Init();
	sendEvent.owner = shared_from_this();

	WSABUF wsaBuf;
	wsaBuf.buf = reinterpret_cast<char*>(sendBuffer);
	wsaBuf.len = len;

	DWORD numOfBytes = 0;
	if (::WSASend(socket, &wsaBuf, 1, &numOfBytes, 0, &sendEvent, nullptr) == SOCKET_ERROR)
	{
		int32_t errorCode = ::WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			Disconnect(L"WSASend Error");
		}

	}
}


void Session::ProcessSend(int32_t numOfBytes)
{
	auto self = shared_from_this(); // 처리 도중 세션 소멸 방지
	sendEvent.owner = nullptr;

	if (numOfBytes == 0)
	{
		Disconnect(L"Send 0 Bytes");
		return;
	}
	
	OnSend(numOfBytes);
}

void Session::ProcessDisconnect()
{
	if (socket != INVALID_SOCKET)
	{
		::closesocket(socket);
		socket = INVALID_SOCKET;
	}
	OnDisconnected();
}

