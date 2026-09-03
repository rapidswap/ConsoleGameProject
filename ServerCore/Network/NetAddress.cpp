#include "pch.h"
#include "NetAddress.h"

NetAddress::NetAddress(SOCKADDR_IN sockAddr)
	:sockAddr(sockAddr)
{
}

NetAddress::NetAddress(std::wstring ip, uint16 port)
{
	::memset(&sockAddr, 0, sizeof(sockAddr));
	sockAddr.sin_family = AF_INET;
	sockAddr.sin_addr = Ip2Address(ip.c_str());
	// 호스트 바이트 순서 -> 네트워크 바이트 순서(빅엔디안) 변환.
	sockAddr.sin_port = ::htons(port);
}

std::wstring NetAddress::GetIpAddress()
{
	WCHAR buffer[100];
	::InetNtopW(AF_INET, &sockAddr.sin_addr, buffer, sizeof(buffer) / sizeof(WCHAR));

	return std::wstring(buffer);
}

IN_ADDR NetAddress::Ip2Address(const WCHAR* ip)
{
	IN_ADDR address;
	::InetPtonW(AF_INET, ip, &address);
	return address;
}
