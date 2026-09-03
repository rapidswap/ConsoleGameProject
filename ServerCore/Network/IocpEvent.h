#pragma once
#include "pch.h"

class IocpEvent;

// 비동기 작업의 종류를 구분하는 열거형.
enum class EventType : uint8
{
	Connect,
	Disconnect,
	Accept,
	Recv,
	Send
};

// IOCP 작업을 요청할 수 있는 주체(Session, Listner)의 공통 인터페이스.
class IocpObject : public std::enable_shared_from_this<IocpObject>
{
public:
	virtual HANDLE GetHandle() = 0;
	virtual void Dispatch(IocpEvent* iocpEvent, int32 numOfBytes = 0) = 0;
};

// OVERLAPPED를 활장하여 어떤 작업인지, 누가 요청했는지 기억하는 클래스.
class IocpEvent : public OVERLAPPED
{
public:
	IocpEvent(EventType type);

	// OVERLAPPED 메모리 초기화.
	void Init(); 

public:
	EventType eventType;
	std::shared_ptr<IocpObject> owner;
};