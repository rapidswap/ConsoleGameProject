#pragma once
#include "pch.h"

class IocpObject;

class IocpCore
{
public:
	IocpCore();
	~IocpCore();

	HANDLE GetHandle() { return iocpHandle; }

	// 소켓을 이 IOCP 완료 포트에 등록하는 함수.
	bool Register(class IocpObject* iocpObject);

	// WorkerThread가 호출하여 완료된 일을 꺼내 처리하는 함수.
	bool Dispatch(uint32 timeoutMs = INFINITE);
	

private:
	// OS가 만들어준 IOCP 핸들.
	HANDLE iocpHandle = INVALID_HANDLE_VALUE;
};