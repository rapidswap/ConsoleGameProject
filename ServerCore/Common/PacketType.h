#pragma once

#include "Main/Type.h"

enum class PacketType : uint16
{
	// C_ : Client to Server 
	// S_ : Server to Client

	C_LOGIN = 1001,
	S_LOGIN_OK=1002,

	C_CHAT=2001,
	S_CHAT=2002,

	// 타워 건설 요청 / 결과.
	C_BUILD_TURRET=3001,
	S_BUILD_TURRET=3002,

	// 타워 판매.
	C_SELL_TURRET=3003,
	S_SELL_TURRET=3004,


	// 서버가 몬스터 소환 명령.
	S_SPAWN_MONSTER=4001,

};