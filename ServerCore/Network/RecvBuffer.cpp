#include "RecvBuffer.h"

RecvBuffer::RecvBuffer(int32_t bufferSize)
{
    // 기본 크기의 10배 크기로 메모리 확보.
    capacity = bufferSize * BUFFER_COUNT;
    buffer.resize(capacity);
}

RecvBuffer::~RecvBuffer()
{
}

void RecvBuffer::Clean()
{
    int32_t dataSize = DataSize();

    // 1. 읽을 데이터가 없으면 커서를 맨 앞으로 원상복구.
    if (dataSize == 0)
    {
        readPos = writePos = 0;
    }
    else
    {
        // 2. 남은 데이터가 없다면, 남은 조각을 버퍼 맨 앞으로 당겨오기.
        if (FreeSize() < bufferSize)
        {
            ::memmove(&buffer[0], &buffer[readPos], dataSize);
            readPos = 0;
            writePos = dataSize;
        }
    }
}

bool RecvBuffer::OnRead(int32_t numOfBytes)
{
    if (numOfBytes > DataSize())
    {
        return false;
    }
    readPos += numOfBytes;
    return true;
}

bool RecvBuffer::OnWrite(int32_t numOfBytes)
{
    if (numOfBytes > FreeSize())
    {
        return false;
    }
    
    writePos += numOfBytes;
    return true;
}

