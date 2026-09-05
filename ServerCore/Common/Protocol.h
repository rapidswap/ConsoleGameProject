#pragma once
#include "Main/pch.h"
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

// 클라 -> 서버: 준비 완료/시작 요청.
struct C_READY_PACKET : public PacketHeader
{
	C_READY_PACKET()
	{
		size = sizeof(C_READY_PACKET);
		id = static_cast<uint16_t>(PacketType::C_READY);
	}

	// 혼자 플레이할 때는 강제 시작 플래그.
	bool forceStart = false;
};

// 서버 -> 클라: 전원 준비 완료, 동시 게임 시작 명령.
struct S_GAME_START_PACKET : public PacketHeader
{
	S_GAME_START_PACKET()
	{
		size = sizeof(S_GAME_START_PACKET);
		id = static_cast<uint16_t>(PacketType::S_GAME_START);
	}

	int32_t totalPlayers = 0;
	// 첫 웨이브 준비 시간. 
	float prepTime = 30.0f;
};

// 서버 -> 클라: 대기실 현황 정보.
struct S_ROOM_INFO_PACKET : public PacketHeader
{
	S_ROOM_INFO_PACKET()
	{
		size = sizeof(S_ROOM_INFO_PACKET);
		id = static_cast<uint16_t>(PacketType::S_ROOM_INFO);
	}

	// 실제 접속한 실제 플레이어 수.
	int32_t totalPlayers = 0;
	// 현재 엔터를 눌러 준비 완료한 플레이어 수.
	int32_t readyPlayers = 0;
};

// 클라 -> 서버: 게임 실패(패배) 알림
struct C_GAME_OVER_PACKET : public PacketHeader
{
	C_GAME_OVER_PACKET()
	{
		size = sizeof(C_GAME_OVER_PACKET);
		id = static_cast<uint16_t>(PacketType::C_GAME_OVER);
	}
};

// 서버 -> 클라: 게임 오버 및 대기실 레디 초기화
struct S_GAME_OVER_PACKET : public PacketHeader
{
	S_GAME_OVER_PACKET()
	{
		size = sizeof(S_GAME_OVER_PACKET);
		id = static_cast<uint16_t>(PacketType::S_GAME_OVER);
	}
};

// 클라 -> 서버: 게임 방 나가기(로비 복귀) 요청
struct C_LEAVE_ROOM_PACKET : public PacketHeader
{
	C_LEAVE_ROOM_PACKET()
	{
		size = sizeof(C_LEAVE_ROOM_PACKET);
		id = static_cast<uint16_t>(PacketType::C_LEAVE_ROOM);
	}
};

// 클리어 플레이어 전적/랭킹 레코드
struct ClearRecord
{
	uint32_t playerId = 0;
	char playerName[32] = {};
	int32_t totalGoldSpent = 0;
	int32_t rank = 1;
};

// 클라 -> 서버: 게임 클리어 요청 (웨이브 전원 클리어 또는 치트 단축키)
struct C_GAME_CLEAR_PACKET : public PacketHeader
{
	C_GAME_CLEAR_PACKET()
	{
		size = sizeof(C_GAME_CLEAR_PACKET);
		id = static_cast<uint16_t>(PacketType::C_GAME_CLEAR);
	}

	int32_t totalGoldSpent = 0;
};

// 서버 -> 클라: 게임 클리어 및 전체 플레이어 소비 골드 랭킹 정산 방송
struct S_GAME_CLEAR_PACKET : public PacketHeader
{
	S_GAME_CLEAR_PACKET()
	{
		size = sizeof(S_GAME_CLEAR_PACKET);
		id = static_cast<uint16_t>(PacketType::S_GAME_CLEAR);
	}

	int32_t playerCount = 0;
	ClearRecord records[4] = {};
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

// 클라 -> 서버: 타워 판매 요청.
struct C_SELL_TURRET_PACKET : public PacketHeader
{
	C_SELL_TURRET_PACKET()
	{
		size = sizeof(C_SELL_TURRET_PACKET);
		id = static_cast<uint16_t>(PacketType::C_SELL_TURRET);
	}

	// 판매할 타워의 좌표.
	int32_t posX = 0;
	int32_t posY = 0;
	// 타워의 성급 (환불액 산정용)
	int32_t starTier = 1;
};

// 서버 -> 클라: 타워 판매 결과 동기화.
struct S_SELL_TURRET_PACKET : public PacketHeader
{
	S_SELL_TURRET_PACKET()
	{
		size = sizeof(S_SELL_TURRET_PACKET);
		id = static_cast<uint16_t>(PacketType::S_SELL_TURRET);
	}

	bool success = false;
	// 판매한 플레이어 ID.
	uint32_t playerId = 0;
	// 판매한 위치.
	int32_t posX = 0;
	int32_t posY = 0;
	// 환불받은 골드.
	int32_t refundGold = 0;
};

// 클라 -> 서버: 골드 사용량(누적 사용 금액) 실시간 동기화
struct C_SPEND_GOLD_PACKET : public PacketHeader
{
	C_SPEND_GOLD_PACKET()
	{
		size = sizeof(C_SPEND_GOLD_PACKET);
		id = static_cast<uint16_t>(PacketType::C_SPEND_GOLD);
	}

	int32_t totalGoldSpent = 0;
};

// 서버 -> 클라: 몬스터 소환 명령.
struct S_SPAWN_MONSTER_PACKET : public PacketHeader
{
	S_SPAWN_MONSTER_PACKET()
	{
		size = sizeof(S_SPAWN_MONSTER_PACKET);
		id = static_cast<uint16_t>(PacketType::S_SPAWN_MONSTER);
	}

	// 몬스터 소환 입구.
	int32_t spawnIndex = 0;
	// 몬스터 체력.
	int32_t maxHP = 1;
	// 몬스터 이동속도.
	float speed = 2.0f;
};

#pragma pack(pop)
