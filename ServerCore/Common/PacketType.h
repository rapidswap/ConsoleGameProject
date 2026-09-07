#pragma once

#include "Main/Type.h"

enum class PacketType : uint16
{
	// C_ : Client to Server 
	// S_ : Server to Client

	C_LOGIN = 1001,
	S_LOGIN_OK=1002,
	C_READY=1003,
	S_GAME_START=1004,
	S_ROOM_INFO=1005,
	C_GAME_OVER = 1006,
	S_GAME_OVER = 1007,
	C_LEAVE_ROOM = 1008,
	C_GAME_CLEAR = 1009,
	S_GAME_CLEAR = 1010,

	C_CHAT=2001,
	S_CHAT=2002,

	// 타워 건설 요청 / 결과.
	C_BUILD_TURRET=3001,
	S_BUILD_TURRET=3002,

	// 타워 판매.
	C_SELL_TURRET=3003,
	S_SELL_TURRET=3004,

	// 골드 사용량 동기화
	C_SPEND_GOLD=3005,
	// 골드 획득 (몬스터 처치, 도박 보상 등)
	C_ADD_GOLD=3006,

	// 속성 업그레이드 요청 / 결과 동기화
	C_UPGRADE_TURRET=3007,
	S_UPGRADE_TURRET=3008,

	// 아지트 데미지 동기화
	C_AGIT_DAMAGE=3009,
	S_AGIT_DAMAGE=3010,

	// 서버가 몬스터 소환 명령.
	S_SPAWN_MONSTER=4001,

};