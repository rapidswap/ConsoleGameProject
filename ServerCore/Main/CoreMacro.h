#pragma once

// 잘못된 주소(0xDEADBEEF)에 강제로 값을 써서 고의로 크래시를 유발하는 매크로.
#define CRASH(cause)						\
{											\
	uint32* crashAddr = nullptr;			\
	__analysis_assume(crashAddr != NULL);	\
	*crashAddr = 0xDEADBEEF;				\
}

// 조건이 거짓이면 즉시 크래시를 내서 버그 지점 잡아내는 assert.
#define ASSERT_CRASH(expr)					\
{											\
	if (!(expr))							\
	{										\
		CRASH("ASSERT_CRASH");				\
		__analysis_assume(expr);			\
	}										\
}