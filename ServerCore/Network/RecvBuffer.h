#pragma once

#include "Main/Type.h"
#include <vector>

class RecvBuffer
{
	enum { BUFFER_COUNT = 10 };

public:
	RecvBuffer(int32_t bufferSize);
	~RecvBuffer();

	// 버퍼 청소: 이미 처리한 데이터는 날리고, 남은 데이터를 맨 앞으로 당기기.
	void Clean();

	// 커서 이동 함수.
	bool OnRead(int32_t numOfBytes);
	bool OnWrite(int32_t numOfBytes);

	// 데이터 위치 및 크기 확인.
	BYTE* ReadPos() { return &buffer[readPos]; }
	BYTE* WritePos() { return &buffer[writePos]; }
	int32_t DataSize() const { return writePos - readPos; }
	int32_t FreeSize() const { return capacity - writePos; }

private:
	int32_t capacity = 0;
	int32_t bufferSize = 0;
	int32_t readPos = 0;
	int32_t writePos = 0;
	std::vector<BYTE> buffer;
};

