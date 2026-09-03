#pragma once

// Windows.h가 불필요하게 wiusock 1.x 를 포함해서 충돌나는 것을 방지.
#define WIN32_LEAN_AND_MEAN

// 1. 윈도우 소켓 관련 헤더 (반드시 Windows.h보다 먼저 와야함).
#include <WinSock2.h>
#include <MSWSock.h>
#include <WS2tcpip.h>
#include <Windows.h>

// 2. 자주 사용하는 C++ 표준 라이브러리.
#include <iostream>
#include <vector>
#include <list>
#include <queue>
#include <stack>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <memory>
#include <mutex>
#include <thread>
#include <functional>
#include <cassert>

// 3. 윈도우 소켓 라이브러리 링크.
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"mswsock.lib")

// 4. 만들어낸 기본 타입 및 매크로.
#include "Type.h"
#include "CoreMacro.h"