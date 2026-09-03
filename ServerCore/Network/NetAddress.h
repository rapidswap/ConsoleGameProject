#pragma once

#include "pch.h"

class NetAddress
{
public:
	NetAddress() = default;
	NetAddress(SOCKADDR_IN sockAddr);
	NetAddress(std::wstring ip, uint16 port);

	SOCKADDR_IN& GetSockAddr() { return sockAddr; }
	std::wstring GetIpAddress();
	uint16 GetPort() { return ::ntohs(sockAddr.sin_port); }

public:
	// IP 문자열을 32비트 정수(빅엔디안)로 변환해주는 유틸 함수.
	static IN_ADDR Ip2Address(const WCHAR* ip);

private:
	SOCKADDR_IN sockAddr = {};

};