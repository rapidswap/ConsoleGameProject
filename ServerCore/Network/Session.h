#pragma once

#include "pch.h"
#include "IocpEvent.h"
#include "NetAddress.h"
#include "RecvBuffer.h"
#include <atomic>
#include <deque>
#include <vector>

class Session : public IocpObject
{
public:
	Session();
	virtual ~Session();

	// IocpObject 인터페이스 구현.
	virtual HANDLE GetHandle() override { return reinterpret_cast<HANDLE>(socket); }

	virtual void Dispatch(IocpEvent* iocpEvent, int32_t numOfBytes = 0) override;

	// 소켓 및 주소 설정.
	void SetSocket(SOCKET setSocket) { socket = setSocket; }
	SOCKET GetSocket() const { return socket; }
	void SetNetAddress(NetAddress address) { netAddress = address; }
	NetAddress GetNetAddress() const { return netAddress; }

	bool IsConnected() const { return connected; }

	// 비동기 송수신.
	void Send(BYTE* buffer, int32_t len);
	void Disconnect(const WCHAR* cause);

	void ProcessConnect();

protected:
	// 비동기 IO 내부 등록 및 처리.
	void RegisterRecv();
	void ProcessRecv(int32_t numOfBytes);
	void ProcessSend(int32_t numOfBytes);
	void ProcessDisconnect();
	bool RegisterSendLocked(); // sendLock must be held.

protected:
	// 상속받는 게임 서버에서 오버라이딩한 함수.
	virtual void OnConnected() {}
	virtual int32_t OnRecv(BYTE* buffer, int32_t len) { return len; }
	virtual void OnSend(int32_t len) {}
	virtual void OnDisconnected() {}

private:
	SOCKET socket = INVALID_SOCKET;
	NetAddress netAddress = {};
	std::atomic<bool> connected = false;

	// 수신(Recv) 관련.
	IocpEvent recvEvent{ EventType::Recv };
	//BYTE recvBuffer[4096] = {};
	// 수신 버퍼 변경.
	RecvBuffer recvBuffer{ 65536 };


	// 송신(Send) 관련.
	IocpEvent sendEvent{ EventType::Send };
	std::deque<std::vector<BYTE>> sendQueue;
	size_t sendOffset = 0;
	size_t queuedBytes = 0;
	bool sendPending = false;
	static constexpr size_t maxQueuedBytes = 1024 * 1024;

	
	// 여러 스레드가 동시에 Send 호출 시 꼬임 방지.
	std::mutex sendLock;

};
