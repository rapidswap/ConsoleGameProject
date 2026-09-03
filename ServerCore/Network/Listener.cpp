#include "pch.h"
#include "Listener.h"
#include "Session.h"

Listener::~Listener()
{
    CloseSocket();
}

void Listener::CloseSocket()
{
    if (listenSocket != INVALID_SOCKET)
    {
        ::closesocket(listenSocket);
        listenSocket = INVALID_SOCKET;
    }

    if (acceptThread.joinable())
    {
        acceptThread.join();
    }
}

bool Listener::StartAccept(NetAddress netAddress, std::shared_ptr<IocpCore> core, std::function<std::shared_ptr<Session>()> factory)
{
    iocpCore = core;
    sessionFactory = factory;

    // 1. 손님 맞이용 리슨 소켓 생성.
    listenSocket = ::WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (listenSocket == INVALID_SOCKET)
    {
        return false;
    }

    // 소켓 재사용 옵션 (서버 껐다 킬 때 TIME_WAIT 방지).
    int enable = 1;
    ::setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<char*>(&enable), sizeof(enable));

    // 2. IP / 포트 바인딩(주소부여).
    if (::bind(listenSocket, reinterpret_cast<const SOCKADDR*>(&netAddress.GetSockAddr()), sizeof(SOCKADDR_IN)) == SOCKET_ERROR)
    {
        CloseSocket();
        return false;
    }

    // 3. 리슨 모드 진입 (대기 큐 크기: SOMAXCONN).
    if (::listen(listenSocket, SOMAXCONN) == SOCKET_ERROR)
    {
        CloseSocket();
        return false;
    }

    // 4. 별도 백그라운드 스레드를 띄워서 손님 수락 대기 시작.
    acceptThread = std::thread(&Listener::AcceptThread, this);

    return true;
}

void Listener::AcceptThread()
{
    while (true)
    {
        SOCKADDR_IN clientAddr = {};
        int addrLen = sizeof(clientAddr);

        // 손님이 올 때까지 대기(블로킹).
        SOCKET clientSocket = ::accept(listenSocket, reinterpret_cast<SOCKADDR*>(&clientAddr), &addrLen);
        if (clientSocket == INVALID_SOCKET)
        {
            // 리슨 소켓이 닫혔으면 루프 종료.
            break;
        }

        // 1. 공장 함수를 통해 새로운 Session 객체 생성
        std::shared_ptr<Session> session = sessionFactory();
        if (session == nullptr)
        {
            ::closesocket(clientSocket);
            continue;
        }

        // 2. 세션에 소켓과 주소 전달
        session->SetSocket(clientSocket);
        session->SetNetAddress(NetAddress(clientAddr));

        // 3. 새 클라이언트 소켓을 IOCP 완료 포트에 등록
        if (iocpCore->Register(session.get()) == false)
        {
            session->Disconnect(L"IOCP Register Failed");
            continue;
        }

        // 4. 세션 접속 완료 처리 및 첫 수신(WSARecv) 시작!
        session->ProcessConnect();
    }
}
