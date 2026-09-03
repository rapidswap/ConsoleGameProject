#pragma once
#include <cstdint>

// 1바이트 부호 없는 정수.
using BYTE = unsigned char;

// 고정 크기 부호 있는 정수.
using int8		= __int8;				// 1 바이트.
using int16		= __int16;				// 2 바이트.
using int32		= __int32;				// 4 바이트.
using int64		= __int64;				// 8 바이트.

// 고정 크기 부호 없는 정수.
using uint8		= unsigned __int8;		// 1 바이트.
using uint16	= unsigned __int16;		// 2 바이트.
using uint32	= unsigned __int32;		// 4 바이트.
using uint64	= unsigned __int64;		// 8 바이트.