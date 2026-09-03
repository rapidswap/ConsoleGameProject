#include "pch.h"
#include "IocpCore.h"
#include "NetAddress.h"
#include "Session.h"

class Listener
{
public:
	Listener() = default;
	~Listener();

	// 포트를 열고 손님을 준비를 시작하는 함수.
	bool StartAccept(NetAddress netAddress, std::shared_ptr<IocpCore> core, std::function<std::shared_ptr<Session>()> factory);

	// 리슨 소켓 닫기.
	void CloseSocket();

private:
	// 백그라운드에서 손님을 기다리는 스레드 함수.
	void AcceptThread();

private:
	SOCKET listenSocket = INVALID_SOCKET;
	std::shared_ptr<IocpCore> iocpCore;
	std::function<std::shared_ptr<Session>()> sessionFactory;
	std::thread acceptThread;
};