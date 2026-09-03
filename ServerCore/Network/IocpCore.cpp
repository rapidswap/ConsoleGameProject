#include "pch.h"
#include "IocpCore.h"
#include "IocpEvent.h"

IocpCore::IocpCore()
{
	// OS에게 완료 큐 하나를 요청.
	iocpHandle = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
	ASSERT_CRASH(iocpHandle != INVALID_HANDLE_VALUE);
}

IocpCore::~IocpCore()
{
	if (iocpHandle != INVALID_HANDLE_VALUE)
	{
		::CloseHandle(iocpHandle);
		iocpHandle = INVALID_HANDLE_VALUE;
	}
}

bool IocpCore::Register(IocpObject* iocpObject)
{
	// 관찰할 소켓을 기존에 만든 iocpHandle에 연결 등록.
	// 세 번째 인자는 0으로 넘겨도 Iocp안에 owner가 들어있으므로 상관없음.
	return ::CreateIoCompletionPort(iocpObject->GetHandle(), iocpHandle, 0, 0);
}

bool IocpCore::Dispatch(uint32 timeoutMs)
{
	DWORD numOfBytes = 0;
	ULONG_PTR key = 0;
	IocpEvent* iocpEvent = nullptr;

	// 비동기 작업이 끝날 때까지 스레드가 여기서 대기 하다가 완료되면 깨어남.
	if (::GetQueuedCompletionStatus(iocpHandle, &numOfBytes, &key, reinterpret_cast<LPOVERLAPPED*>(&iocpEvent), timeoutMs))
	{
		// 정상 완료: 일감의 주인을 찾아가서 처리하라고 전달.
		std::shared_ptr<IocpObject> owner = iocpEvent->owner;
		if (owner)
		{
			owner->Dispatch(iocpEvent, numOfBytes);
		}
	}
	else
	{
		int32 errCode = ::WSAGetLastError();
		switch (errCode)
		{
			// 지정한 시간이 지나도 일이 안 들어왔을 때.
		case WAIT_TIMEOUT:
			return false;
			// 소켓이 비정상 종료되었거나 에러가 났을 때도 일단 주인에게 전달해서 뒷정리하게 함.
		default:
			if (iocpEvent != nullptr)
			{
				std::shared_ptr<IocpObject> owner = iocpEvent->owner;
				if (owner)
				{
					owner->Dispatch(iocpEvent, numOfBytes);
				}
			}
			break;
		}
	}
	return true;
}
