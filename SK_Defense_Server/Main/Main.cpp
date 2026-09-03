#include "Network/IocpCore.h"
#include "Network/Listener.h"
#include "Network/GameSession.h"

#include <iostream>
#include <memory>
#include <thread>
#include <vector>

#include <set>

std::set<std::shared_ptr<GameSession>> g_sessions;
std::mutex g_sessionLock;


LONG WINAPI ExceptionFilter(EXCEPTION_POINTERS* pException)
{
	printf("CRASH! Code: 0x%08X, Address: %p\n",
		pException->ExceptionRecord->ExceptionCode,
		pException->ExceptionRecord->ExceptionAddress);
	fflush(stdout);
	return EXCEPTION_EXECUTE_HANDLER;
}

int main()
{
	::SetUnhandledExceptionFilter(ExceptionFilter);

	// 1. 윈도우 소켓 초기화.
	WSAData wsaData;
	if (::WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		std::cout << "WSAStartup Failed!\n";
		return 0;
	}

	// 2.IOCP 코어 생성.
	auto iocpCore = std::make_shared<IocpCore>();

	// 3. 포트 7777로 리스너 생성 및 가동.
	auto listener = std::make_shared<Listener>();
	NetAddress listenAddr(L"127.0.0.1", 7777);

	// 새 손님이 올 때마다 GameSession을 만들어주도록 람다 함수 전달.
	if (!listener->StartAccept(listenAddr, iocpCore, []()
		{return std::make_shared<GameSession>();}))
	{
		std::cout << "Failed to StartAccept on port 7777\n";
	}

	std::cout << "========================================\n";
	std::cout << "  SK Defense IOCP Server Started (Port: 7777)\n";
	std::cout << "========================================\n";

	// 4. 일꾼 스레드 4개 가동.
	std::vector<std::thread> workerThreads;
	for (int i = 0;i < 4;++i)
	{
		workerThreads.emplace_back([iocpCore]()
			{
				while (true)
				{
					iocpCore->Dispatch();
				}
			});
	}

	// 메인 스레드는 콘솔 종료 방지 대기.
	for (auto& t : workerThreads)
	{
		if (t.joinable())
		{
			t.join();
		}
	}

	::WSACleanup();
	return 0;

}