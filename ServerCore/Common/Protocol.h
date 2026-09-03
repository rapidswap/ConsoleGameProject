#pragma once
#include "pch.h"
#include "Main/Type.h"
#include "PacketType.h"


// 1. 모든 패킷의 기본 명찰 헤더.
// 패킷 크기가 1바이트 단위로 들어맞도록 바이트 정렬.
#pragma pack(push, 1)
struct PacketHeader
{
	// 헤더 포함 패킷 총 크기.
	uint16 size;

	// 패킷 구분 ID.
	uint16 id;
};


// [1] 로그인 / 방 입장 관련 패킷.

// 클라 -> 서버: 접속 / 로그인 요청.
struct C_LOGIN_PACKET : public PacketHeader
{
	C_LOGIN_PACKET()
	{
		size = sizeof(C_LOGIN_PACKET);
		id = static_cast<uint16_t>(PacketType::C_LOGIN);
	}

	// 플레이어 닉네임.
	char playerName[32] = {};
};

struct S_LOGIN_OK_PACKET : public PacketHeader
{
	S_LOGIN_OK_PACKET()
	{
		size = sizeof(S_LOGIN_OK_PACKET);
		id = static_cast<uint16_t>(PacketType::S_LOGIN_OK);
	}
	
	// 서버가 발급해주는 고유 플레이어 ID.
	uint32_t playerId = 0;

	// 초기 시작 골드.
	int32_t currentGold = 100;
};


// [2] 채팅 패킷.

// 클라 -> 서버: 채팅 전송.
struct C_CHAT_PACKET : public PacketHeader
{
	C_CHAT_PACKET()
	{
		size = sizeof(C_CHAT_PACKET);
		id = static_cast<uint16_t>(PacketType::C_CHAT);
	}

	char msg[128] = {};
};

// 서버 -> 클라: coxld qkdthd.
struct S_CHAT_PACKET : public PacketHeader
{
	S_CHAT_PACKET()
	{
		size = sizeof(S_CHAT_PACKET);
		id = static_cast<uint16_t>(PacketType::S_CHAT);
	}

	uint32_t playerId = 0;
	char msg[128] = {};
};

// [3] 타워 디펜스 게임 로직 패킷.

// 클라 -> 서버: 타워 설치 요청.
struct C_BUILD_TURRET_PACKET : public PacketHeader
{
	C_BUILD_TURRET_PACKET()
	{
		size = sizeof(C_BUILD_TURRET_PACKET);
		id = static_cast<uint16_t>(PacketType::C_BUILD_TURRET);
	}


	// 설치할 맵 x 좌표.
	int32_t posX = 0;
	// 설치할 맵 y 좌표.
	int32_t posY = 0;
	// 타워 종류.
	int32_t turretType = 0;
};

// 서버 -> 클라: 타워 설치 성공 결과 전송.
struct S_BUILD_TURRET_PACKET : public PacketHeader
{
	S_BUILD_TURRET_PACKET()
	{
		size = sizeof(S_BUILD_TURRET_PACKET);
		id = static_cast<uint16_t>(PacketType::S_BUILD_TURRET);
	}

	// 타워 설치 여부.
	bool success = true;
	// 타워를 지은 플레이어 id.
	uint32_t playerId = 0;
	int32_t posX = 0;
	int32_t posY = 0;
	int32_t turretType = 0;
	// 설치 후 남은 골드 동기화.
	int32_t remainingGold = 0;
};


#pragma pack(pop)
