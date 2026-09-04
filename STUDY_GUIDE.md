# 🛡️ SK Defense 프로젝트 종합 학습 & 아키텍처 가이드

본 문서는 **Console Game Engine 기반의 멀티플레이어 협동 타워 디펜스 게임 (`SK_Defense`)**의 핵심 아키텍처, 네트워크 통신 모델, 멀티스레딩 구조 및 실전 기술 면접 대비 포인트를 총정리한 학습서입니다.

---

## 📑 목차
1. [프로젝트 개요 및 핵심 흐름](#1-프로젝트-개요-및-핵심-흐름)
2. [멀티스레딩 아키텍처 및 스레드 분장](#2-멀티스레딩-아키텍처-및-스레드-분장)
3. [네트워크 통신 & 패킷 프로토콜 구조](#3-네트워크-통신--패킷-프로토콜-구조)
4. [서버 권한(Server-Authoritative) 검증 모델](#4-서버-권한server-authoritative-검증-모델)
5. [클라이언트 게임 엔진 및 레벨 라이프사이클](#5-클라이언트-게임-엔진-및-레벨-라이프사이클)
6. [핵심 알고리즘 & 인게임 시스템](#6-핵심-알고리즘--인게임-시스템)
7. [주요 트러블슈팅 & 기술 면접 Q&A](#7-주요-트러블슈팅--기술-면접-qa)

---

## 1. 프로젝트 개요 및 핵심 흐름

### 🎯 핵심 요약
- **프로젝트명**: SK Defense (Console Multiplayer Tower Defense)
- **사용 기술**: C++20, Windows Console API, WinSock2, Windows IOCP (I/O Completion Port)
- **특징**:
  - 자체 제작 콘솔 렌더링 엔진(`CraftEngine`) 기반 2D 그리드 디펜스.
  - Windows IOCP 기반의 고성능 멀티스레드 전용 게임 서버 탑재.
  - 서버 권한형(Server-Authoritative) 재화 및 몬스터 스폰 동기화.
  - 서버 미구동 시에도 부드럽게 동작하는 오프라인 싱글플레이 모드 완비.

---

## 2. 멀티스레딩 아키텍처 및 스레드 분장

이 프로젝트는 **I/O 블로킹이 메인 게임 루프를 방해하지 않도록 철저히 스레드를 분리**했습니다.

### 🏢 서버 스레드 구조 (총 6개)
1. **메인 루프 스레드 (1개)** - `SK_Defense_Server/Main/Main.cpp`
   - 서버의 전반적인 라이프사이클 관리.
   - `std::this_thread::sleep_for(100ms)` 주기로 `GGameRoom->Update(0.1f)`를 호출하여 웨이브 제한시간, 몬스터 스폰 딜레이를 계측.
2. **Accept 전담 스레드 (1개)** - `ServerCore/Network/Listener.cpp`
   - `Listener::AcceptThread`가 신규 클라이언트 접속(`accept`)을 전담 감시.
   - 접속 즉시 소켓을 생성하고 IOCP 핸들에 등록한 뒤 `GameSession` 생성.
3. **IOCP Worker 스레드 (4개)** - `SK_Defense_Server/Main/Main.cpp`
   - `iocpCore->Dispatch()`를 무한 대기하며 커널에서 완료된 네트워크 비동기 송수신(Recv/Send) 이벤트를 감지.
   - 완료 패킷을 언패킹하여 `ClientPacketHandler` 및 `GameRoom` 함수 호출 (타워 건설, 판매, 채팅 등 처리).

### 🎮 클라이언트 스레드 구조 (1클라이언트 당 총 2개)
1. **메인 게임 루프 스레드 (1개)** - `Project_SKDefense/Main/Main.cpp`
   - 키보드/마우스 입력 폴링, 씬 매니저 틱, 화면 버퍼 더블 버퍼링 렌더링.
   - 매 프레임 `NetworkManager::Get()->Update()`를 호출하여 백그라운드 큐에 쌓인 패킷을 메인 스레드 문맥에서 안전하게 꺼내어 처리(스레드 경합 방지).
2. **패킷 수신(Recv) 전담 스레드 (1개)** - `Project_SKDefense/Network/NetworkManager.cpp`
   - 서버와 연결된 동안 `NetworkManager::RecvThread`에서 동기 블로킹 `::recv()`를 반복.
   - 패킷 헤더의 `size`를 확인하여 온전한 1개의 패킷이 조립되면 스레드 안전 큐(`packetQueue`, `mutex` 보호)에 인큐.

---

## 3. 네트워크 통신 & 패킷 프로토콜 구조

### 📦 패킷 헤더 및 상속 구조
모든 패킷은 크기(`size`)와 타입(`id`)을 가진 `PacketHeader`로 시작합니다 (`ServerCore/Common/Protocol.h`).

```cpp
#pragma pack(push, 1)
struct PacketHeader
{
    uint16_t size; // 패킷 전체 크기 (헤더 포함)
    uint16_t id;   // 패킷 식별자 (PacketType 열거형)
};
#pragma pack(pop)
```

### 📋 주요 패킷 목록

| Packet ID | 패킷 구조체 | 방향 | 설명 |
| :--- | :--- | :---: | :--- |
| `S_LOGIN_OK` | `S_LOGIN_OK_PACKET` | S -> C | 로그인 성공, 고유 `playerId` 및 초기 골드(100G) 지급 |
| `S_ROOM_INFO` | `S_ROOM_INFO_PACKET` | S -> C | 현재 로비 접속자 수, 준비 완료 인원 수 브로드캐스트 |
| `C_READY` | `C_READY_PACKET` | C -> S | 플레이어 준비 상태 통보 (2인 모두 완료 시 게임 시작) |
| `S_GAME_START`| `S_GAME_START_PACKET`| S -> C | 두 클라이언트에게 게임 레벨 동시 진입 명령 |
| `C_BUILD_TURRET` | `C_BUILD_TURRET_PACKET` | C -> S | 타워 건설 요청 (좌표 x, y, 터렛 타입) |
| `S_BUILD_TURRET` | `S_BUILD_TURRET_PACKET` | S -> C | 서버 검증 완료된 타워 정보 및 잔여 골드 전파 |
| `C_SELL_TURRET` | `C_SELL_TURRET_PACKET` | C -> S | 타워 판매 요청 (좌표 x, y, 강화 단계 `starTier`) |
| `S_SELL_TURRET` | `S_SELL_TURRET_PACKET` | S -> C | 타워 삭제 및 환급 골드 반영 전파 |
| `S_SPAWN_MONSTER` | `S_SPAWN_MONSTER_PACKET` | S -> C | 서버 웨이브 타이머에 따른 몬스터 동기 소환 명령 |

---

## 4. 서버 권한(Server-Authoritative) 검증 모델

### 🛡️ 왜 서버 권한인가?
클라이언트가 독자적으로 `currentGold`를 차감하거나 추가하면, 메모리 변조 툴(치트엔진 등)이나 악의적인 클라이언트 변조로 무한 타워 건설/골드 복사가 가능해집니다.

### 🔄 타워 건설 트랜잭션 흐름
1. **클라이언트 요청**: 유저가 클릭하면 로컬에서 골드를 바로 깎지 않고 `C_BUILD_TURRET_PACKET` 전송.
2. **서버 검증 (`GameRoom::HandleBuildTurret`)**:
   - `if (session->gold < 50) return;` 잔고 부족 시 단칼에 거절.
   - `session->gold -= 50;` 서버가 직접 골드를 차감.
3. **결과 브로드캐스트**: 타워 위치 정보와 차감 후 잔액(`remainingGold`)을 담아 `S_BUILD_TURRET_PACKET` 브로드캐스트.
4. **클라이언트 반영**: 본인 패킷이면 `DefenseLevel::Get()->SetGold(pkt.remainingGold)`로 동기화하고 타워 객체 배치.

### 💰 타워 판매 트랜잭션 흐름
1. 타워의 강화 단계(`starTier`)를 담아 서버로 `C_SELL_TURRET_PACKET` 전송.
2. 서버는 강화 단계별 환급 공식(`30 * (1 << starTier)`)을 검증 및 계산하여 `session->gold`에 안전하게 가산.
3. 방 전체에 타워 제거 및 판매자 골드 동기화 패킷 전송.

---

## 5. 클라이언트 게임 엔진 및 레벨 라이프사이클

- **기반 엔진**: `CraftEngine` (Double Buffering 콘솔 가상 화면 렌더링)
- **레벨(Level) 구조**:
  - `MainMenuLevel`: 서버 자동 접속 시도, 2인 멀티 로비 표시, 엔터 키를 통한 레디 / 오프라인 시작 분기.
  - `DefenseLevel`: 실제 게임 플레이 공간. 타워 건설/판매, A* 길찾기 기반 몬스터 이동, 도박(Gamble), 타워 합성.
  - `GameOverLevel` / `GameClearLevel`: 게임 종료/성공 화면.
- **오프라인 싱글플레이 보장**:
  - `MainMenuLevel::OnInitialized()`에서 최초 1회만 접속 시도.
  - 서버가 꺼져 있어도 매 프레임 `connect()`를 재호출하지 않으므로 60FPS가 부드럽게 유지됨.
  - 연결 실패 상태에서 엔터를 누르면 서버 패킷 없이 클라이언트 로컬 로직으로 즉시 게임 시작.

---

## 6. 핵심 알고리즘 & 인게임 시스템

1. **A\* (A-Star) 길찾기 알고리즘 (`Actor/AStar.cpp`)**
   - 몬스터가 맵 상의 스폰 포인트(`S`)에서 목적지 아지트(`D`)까지 실시간 최단 경로를 탐색.
   - 유저가 타워를 건설하여 경로를 막으면 경로를 우회 재탐색.
   - 모든 경로가 완전히 차단되는 경우 건설 불가(`CanBuildTurret`) 처리.
2. **타워 합성(Merge) 시스템**
   - 동일한 등급(`starTier`)과 동일한 타입의 타워가 인접하거나 조건을 만족할 때 상위 타워로 승급.
3. **가챠/도박(Gamble) 애니메이션 및 룰**
   - 슬롯머신 형태의 콘솔 텍스트 애니메이션 연출.
   - 확률에 따라 대량의 골드 획득 또는 소모.

---

## 7. 주요 트러블슈팅 & 기술 면접 Q&A

### Q1. 싱글플레이 진입 시 화면이 멈추거나 렉이 걸렸던 원인과 해결책은?
- **원인**: `MainMenuLevel::Tick`에서 서버 미연결 상태일 때 매 프레임마다 동기 블로킹 함수인 `::connect()`를 호출함. 윈도우 소켓은 연결 실패 시 타임아웃까지 1~2초간 메인 스레드를 정지시키므로, 프레임레이트가 0.5fps로 곤두박질치고 엔터 키 입력을 놓침.
- **해결**: 매 틱 폴링 방식을 제거하고, 레벨 초기화 시점(`OnInitialized`)에서 최초 1회만 연결을 시도하도록 구조를 개선. 연결 실패 시 즉시 로컬 싱글플레이 모드로 진입하도록 분기 처리.

### Q2. IOCP(I/O Completion Port)를 선택한 이유와 장점은?
- **답변**: 다중 접속 처리 모델 중 `select` 모델은 감시 가능한 소켓 개수 제한(FD_SETSIZE 64개)과 매 호출마다 O(N)의 검사 비용이 발생합니다. 반면 Windows 커널 기반의 **IOCP는 Overlapped I/O 완료 이벤트를 커널 큐에서 비동기로 관리**하므로, 등록된 소켓 수에 비례하지 않고 완료된 I/O 작업만 워커 스레드가 즉시 꺼내어 처리(O(1))하므로 컨텍스트 스위칭 비용과 CPU 오버헤드가 매우 낮습니다.

### Q3. 서버와 클라이언트 간 스레드 경합(Data Race)은 어떻게 방지했는가?
- **서버**: `GameRoom` 내부에서 `std::mutex`를 사용하여 플레이어 목록(`sessions`), 레디 상태(`readyPlayerIds`), 게임 상태 갱신 시 `std::lock_guard`로 크리티컬 섹션을 보호.
- **클라이언트**: 네트워크 수신 스레드는 소켓에서 받은 패킷을 메인 스레드의 메모리에 바로 적용하지 않고, `std::mutex`로 보호되는 `packetQueue`에 삽입만 수행. 메인 루프가 자신의 `Tick()` 주기에서 안전하게 큐를 비우며 상태를 반영하는 **메시지 큐 패턴(Message Queue Pattern)**을 적용하여 렌더링과 게임 틱 간의 충돌을 원천 차단.
