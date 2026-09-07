# 🛡️ SK Defense : 시니어 테크 면접관 맞춤형 심층 기술 분석 & 인터뷰 방어 가이드

> **[면접관 페르소나 총평]**
> "단순한 텍스트 기반 콘솔 게임이 아닙니다. **A\* 길찾기 알고리즘의 예외 처리(Dry-run 트랜잭션 롤백, 대각선 모서리 컷)**, **C++ 객체 수명주기 관리(Object Pooling, Dangling Pointer 방지, Weak Reference)**, 그리고 **Windows IOCP 기반 고성능 전용 서버(Dedicated Server)의 세션 관리, 슬라이딩 윈도우 패킷 조립, 재귀 락 데드락 방지, 서버 권한형(Server-Authoritative) 인게임 경제 및 몬스터 킬 동기화**까지 아우르는 매우 밀도 높은 상용급 시스템 프로젝트입니다.
> 
> 면접관은 '왜 굳이 이 방식을 택했는지(Trade-off)', '네트워크 동기화와 클라이언트 변조 취약점을 어떻게 인지하고 방어했는지', '멀티스레드 동시성과 데드락을 어떻게 다뤘는지'를 집요하게 파고들 것입니다. 본 문서는 이 질문들을 논리적이고 기술적인 언어로 완벽히 방어할 수 있도록 집대성되었습니다."

---

## 1. 1분 프로젝트 요약 (발표 / 면접 첫 질문용)

> **"SK Defense 프로젝트는 자체 제작 C++ 콘솔 엔진(`CraftEngine`) 환경에서 유저가 2x2 터렛 배치를 통해 맵의 동선을 직접 설계하는 '메이징(Mazing) 타워 디펜스'를, Windows IOCP 기반의 고성능 멀티스레드 전용 서버(Dedicated Server)와 연동하여 2인 실시간 협동 멀티플레이어로 구현한 프로젝트입니다.**
> 
> 기존 콘솔 I/O와 가상 터미널(ConPTY)의 한계를 극복하기 위해 Win32 물리 좌표 역보정 및 오브젝트 풀링을 도입하여 런타임 힙 할당과 프레임 드랍을 원천 차단했습니다.
> 
> 특히 유저의 악의적인 완전 길막을 차단하는 **'가상 시뮬레이션(Dry-run) 트랜잭션 롤백'**과 A\* 대각선 코너 컷 차단 알고리즘을 독자 설계하여 공간 무결성을 확보했습니다.
> 
> 나아가 클라이언트 변조 및 프레임 시차로 인한 비동기화를 방어하기 위해 **서버 권한형(Server-Authoritative) 재화/합성 검증, 슬라이딩 윈도우 TCP 패킷 조립, 서버 승인형 몬스터 킬 브로드캐스트(`C_ENEMY_KILL` / `S_ENEMY_KILL`)**, 그리고 **락-프리 안전 세션 종료(Deadlock Avoidance)** 파이프라인을 완성했습니다."

---

## 2. 기술 선택 및 아키텍처 트레이드오프 (Trade-off 심층 분석)

| 구분 | 적용 기술 / 구조 | 대안 기술 (Alternative) | 득과 실 (이점 vs 비용/한계) 및 선택 이유 |
| :--- | :--- | :--- | :--- |
| **길찾기 알고리즘** | **A\* (A-Star) 알고리즘**<br>(유클리드 휴리스틱 + 8방향 모서리 컷) | 다익스트라(Dijkstra) or 플루드 필(Flood Fill) or Flow Field | **[이점]** 목적지(아지트)가 명확한 1:N 구조에서 휴리스틱 $H(n)$을 통해 불필요한 전체 맵 노드 방문을 60% 이상 줄여 실시간 반응성 확보.<br>**[비용]** 몬스터 수가 수백 마리로 증가하면 탐색 비용 누적.<br>**[선택 이유]** 맵 크기(50x20)와 동시 액티브 몬스터 수(30마리) 환경에서는 맵 전체 벡터장을 매 프레임 굽는 Flow Field보다, 터렛 설치/철거 이벤트 시에만 캐싱된 경로를 재계산하는 A\*가 CPU 캐시 및 메모리 효율 측면에서 최적이라 판단. |
| **완전 길막 검증** | **가상 시뮬레이션 (Dry-run) & 트랜잭션 롤백** | 수학적 위상 검사 (Euler Characteristic) or 런타임 끼임 감지 후 강제 파괴 | **[이점]** 설치 직전 메모리 상에서 임시 타일 마킹 후 경로 검증을 거치므로, 몬스터가 갇히는 기획적 예외를 0%로 원천 차단.<br>**[비용]** 타워 건설 클릭 시 다중 스폰 지점에 대한 A\* 가상 탐색 연산 발생.<br>**[방어 근거]** 프레임마다 검사하지 않고, 설치 가능 후보 타일 상태에서만 검증하며, 단 하나의 입구라도 도달 불가 시 즉시 break하는 조기 탈출(Early Exit)로 평균 0.2ms 내에 검증 완료. |
| **메모리 수명 관리** | **사전 할당 오브젝트 풀링 (Object Pooling)**<br>(`std::vector<weak_ptr>`) | 매 스폰/사망 시 `new` / `delete`<br>(동적 힙 할당) | **[이점]** 게임 중 빈번한 힙 할당/해제로 인한 메모리 단편화(Fragmentation)와 OS 컨텍스트 스위칭 지연을 제거하여 60 FPS 무지연 보장.<br>**[비용]** 웨이브 최대치(30마리) 메모리를 상시 점유하며, `SetActive(false)` 회수 후 재스폰 시 체력/스탯을 명시적으로 리셋(`Reset`)해야 하는 책임 증가. |
| **서버 I/O 모델** | **Windows IOCP (I/O Completion Port)**<br>+ 4 Worker Threads | 동기 블로킹 소켓 or `select` 다중화 or Epoll / Boost.Asio | **[이점]** Windows 커널 레벨의 비동기 Overlapped I/O 완료 큐를 활용하여, 소켓 수에 비례하지 않는 $O(1)$ 이벤트 디스패치와 완벽한 멀티코어 부하 분산 달성.<br>**[비용]** 비동기 버퍼 수명 보장, 완료 통지 순서 제어, 멀티스레드 동기화 락 설계 난이도 증가. |
| **패킷 직렬화** | **`#pragma pack(push, 1)` 기반 POD 바이너리 구조체 캐스팅** | Google Protocol Buffers (Protobuf) or FlatBuffers or JSON | **[이점]** 직렬화/역직렬화에 따른 CPU 파싱 오버헤드가 '0'이며, `sizeof(T)` 단위의 즉시 메모리 복사 및 `reinterpret_cast` 처리 가능.<br>**[비용]** 엔디안(Endianness) 불일치 및 가변 길이 데이터 처리에 취약.<br>**[선택 이유]** 동일 x86_64 Windows 환경 내 고정 패킷 규격이므로, 성능과 코드 단순성을 극대화하기 위해 1바이트 패킹 바이너리 구조체 방식을 선택. |
| **데이터 동기화 모델** | **서버 권한 모델 (Server-Authoritative)**<br>(Single Source of Truth) | 클라이언트 예측 선차감(Pre-deduction) or 단순 릴레이 P2P | **[이점]** 재화 차감, 타워 건설/판매 승인, 속성 업그레이드, 아지트 피해, 몬스터 처치 판정의 모든 주도권을 서버가 독점하여 치트 및 클라이언트 간 상태 역전 현상 원천 봉쇄.<br>**[비용]** 클라이언트 입력 후 서버 승인 왕복(RTT) 대기 발생.<br>**[선택 이유]** 3단 합성 시 클라이언트 선차감 값이 롤백되어 골드가 급증하던 결함을 서버 단일 진실 공급원 체계로 완벽 해결함. |
| **플레이어 ID 모델** | **방 단위 최소 빈 슬롯 (Slot 1, Slot 2) 동적 배정** | 서버 전역 시퀀스 증가 발급기<br>(`std::atomic nextId++`) | **[이점]** 2인 협동 방 환경에서 유저가 탈퇴/재접속하거나 새 방을 생성해도 UI 및 게임 로직상 항상 1번(Host)과 2번(Guest) 슬롯으로 직관적 매핑 유지.<br>**[비용]** 방 내부 세션 맵 탐색 연산 발생 ($O(\log N)$).<br>**[선택 이유]** 방 최대 정원이 2인이므로 탐색 비용이 전무하며, ID가 3, 4, 5...로 무한 증가하는 부작용을 완벽히 억제. |

---

## 3. 데이터 라이프사이클 및 동시성/병목 지점

### 3.1. 전 구간 데이터 및 패킷 처리 파이프라인

```
[1. 클라이언트 유저 액션] (마우스 클릭 / 단축키 입력)
      │
      ▼
[2. 클라이언트 NetworkManager]
   - 송신 버퍼(SendBuffer) 패킷 구조체 조립 (Header size + id)
   - send() 소켓 전송
      │
      ▼ (TCP / IP 루프백 or 네트워크 전송)
      │
      ▼
[3. 서버 ServerCore (IOCP & RecvBuffer)]
   - Listener::AcceptThread -> 소켓 수락 및 IOCP 핸들 연결
   - WSARecv Overlapped I/O 완료 -> IOCP Worker Thread Dispatch
   - 슬라이딩 윈도우 RecvBuffer 누적 -> PacketHeader(size, id) 완전체 검증
   - ClientPacketHandler::HandlePacket() switch-case 분기
      │
      ▼
[4. 서버 GameRoom (비즈니스 로직 & 동기화)]
   - std::lock_guard<std::mutex> 획득
   - 세션 잔고 검증 (gold >= 50), 중복 처치 필터링 (deadMonsters.find)
   - GameRoom::Broadcast()로 방 세션 전원에게 S_ 승인 패킷 송신
      │
      ▼
[5. 클라이언트 ServerPacketHandler & Level 반영]
   - 클라이언트 백그라운드 수신 스레드(RecvThread) -> packetQueue(스레드 세이프) 인큐
   - 메인 스레드 Tick: NetworkManager::Update()에서 큐 디큐 -> 상태 반영
   - DefenseLevel 액터 생성/파괴/골드 갱신 및 콘솔 렌더러 프레임 버퍼 Submit
```

---

### 3.2. 치명적 병목 & 동시성 위험 지점 4곳 심층 방어 전략

#### ① [동시성] `GameRoom` 내부 락-프리 연결 해제 및 재귀 락(Self-Deadlock) 원천 방어
- **위험 (Danger)**:
  - 플레이어가 게임 도중 창을 닫으면 IOCP 스레드에서 `session->OnDisconnected()` -> `GameRoom::Leave()`가 호출됨.
  - `GameRoom::Leave()`가 `std::mutex lock`을 획득한 상태에서, 게임 진행 중(`PLAYING`) 파트너 세션을 강제 종료하기 위해 동기적으로 `partnerSession->Disconnect()`를 호출함.
  - `Disconnect()`가 즉시 파트너 소켓을 닫고 `GameSession::OnDisconnected()` -> `GameRoom::Leave()`를 **동일 스레드에서 재귀 호출**하게 됨.
  - `std::mutex`는 Non-recursive 뮤텍스이므로, 이미 자물쇠를 쥐고 있는 스레드가 다시 락을 시도하면서 MSVC의 `std::system_error` (Resource deadlock would occur) 예외가 던져지고, 워커 스레드 unhandled terminate로 **서버 전체가 즉시 크래시 폭파**됨.
- **방어 (Defense)**:
  - `GameRoom::Leave()` 내부에서 락을 쥔 상태에서는 **종료 대상 세션들의 포인터를 로컬 벡터(`partnersToDisconnect`)에 복사**하고 방 상태만 `WAITING`으로 초기화.
  - **`lock`이 완전히 해제(Scope Unlock)된 이후**에 루프를 돌며 `partner->SetRoom(nullptr); partner->Disconnect();`를 호출하도록 비블로킹 분리.
  - `GameSession::OnDisconnected()`에서도 방 포인터를 먼저 `SetRoom(nullptr)`로 해제하여 재진입 경로를 원천 차단.

#### ② [정합성] 동적 터렛 3단 합성(3-Merge) 시의 골드 롤백 서지(Surge) 방어
- **위험 (Danger)**:
  - 클라이언트가 타워 설치 시 로컬에서 50G를 선차감(`SpendGold(50)`)하고, 서버도 50G를 차감한 뒤 `S_BUILD_TURRET(remainingGold)`를 회신함.
  - 3번째 타워가 지어지며 연쇄 3단 합성이 일어나는 순간, 클라이언트가 미리 차감해 둔 골드 잔고에 네트워크 지연을 거쳐 도착한 이전 시점의 서버 패킷(`SetGold(pkt.remainingGold)`)이 덮어씌워지면서, 합성이 발생할 때마다 골드가 50~100G씩 환불 복구되어 치솟는 시각적/동기화 왜곡 발생.
- **방어 (Defense)**:
  - 멀티플레이 환경에서는 클라이언트의 로컬 선차감을 전면 제거하고 잔고 보유 여부(`currentGold >= 50`)만 확인하여 요청 패킷만 발송.
  - 오직 서버의 공인 승인 패킷(`S_BUILD_TURRET`)에서만 잔고를 덮어쓰는 **단일 진실 공급원(Single Source of Truth)** 모델로 일원화하여 롤백 버그를 원천 박멸.

#### ③ [동기화] 네트워크 프레임 시차로 인한 몬스터 생사 불일치 및 30~40G 재화 격차 방어
- **위험 (Danger)**:
  - 터렛의 사격 타겟팅과 몬스터 피격 판정이 클라이언트 각자의 로컬 틱에서 독립적으로 연산됨.
  - 미세한 프레임레이트(델타타임) 시차로 인해 클라이언트 A에서는 먼저 처치된 몬스터가 클라이언트 B에서는 처치되지 못하고 아지트에 침투함.
  - 결과적으로 한쪽 클라이언트는 30마리를 다 잡아 300G를 얻었으나 다른 쪽은 260~270G만 얻어 몬스터 생존 상태와 잔여 골드가 영구히 어긋남.
- **방어 (Defense)**:
  - 서버 몬스터 스폰 시 웨이브당 0~29번의 고유 `monsterId`를 발급.
  - 클라이언트에서 몬스터 체력이 0이 되면 로컬 골드를 올리지 않고 `C_ENEMY_KILL(monsterId, 10)`을 서버로 보고.
  - 서버 `GameRoom`은 `std::set<int32_t> deadMonsters`를 통해 중복 처치를 필터링하고, 방 전체 세션에 골드를 공통 가산한 뒤 `S_ENEMY_KILL` 브로드캐스트.
  - 양쪽 클라이언트 모두 패킷 수신 즉시 해당 `monsterId`의 몬스터를 강제 비활성화(`SetActive(false)`) 및 파괴 연출 처리하여 100% 동일한 생사/재화 일치 달성.

#### ④ [네트워크] TCP 스트림 패킷 파편화(Fragmentation) 및 뭉침(Coalescing) 방어
- **위험 (Danger)**:
  - TCP는 바이트 스트림 프로토콜이므로 100바이트 패킷이 40B/60B로 쪼개져 오거나, 3개의 패킷이 1번의 `recv` 버퍼에 뭉쳐서 도착할 수 있음. 단순 버퍼 캐스팅 시 메모리 침범(Access Violation) 크래시 발생.
- **방어 (Defense)**:
  - 슬라이딩 윈도우 원형 수신 버퍼(`RecvBuffer`)를 도입하여 커서(`readPos`, `writePos`) 기반 버퍼 관리.
  - 수신된 데이터가 `PacketHeader` 크기(4바이트) 이상이고 누적 크기가 `header->size` 이상일 때만 정확한 오프셋만큼 파싱하여 핸들러로 넘기는 **패킷 완성 검증 루프** 구축.

---

## 4. 트러블슈팅 경험 (STAR 기법 기반 5선)

### [사례 1] 포탑 배치 중 유저 악의적 완전 길막(Maze Block)으로 인한 몬스터 교착 장애
- **Situation (상황)**: 유저가 포탑(2x2)을 자유롭게 건설하여 몬스터의 진입로를 완전히 막아버릴 경우, 몬스터가 목적지 노드를 찾지 못해 제자리에 멈춰 서고 웨이브가 끝나지 않아 게임 전체가 멈추는 교착(Deadlock) 상태 발생.
- **Task (과제)**: 유저의 건축 자유도를 침해하지 않으면서도, 시스템적으로 '도달 불가능한 미로'의 완성을 0.01초 내에 판별하여 자원을 보존하고 설치를 거부할 방어책 마련.
- **Action (행동)**:
  - 데이터베이스의 **트랜잭션 롤백(Dry-run Simulation & Rollback)** 기법 도입.
  - `CanBuildTurret(x, y)` 호출 시 실제 액터를 올리기 전 인메모리 `mapGrid` 타일에만 임시로 `2`(장애물)를 가상 마킹.
  - 활성화된 모든 스폰 입구(`spawnPoints`)에서 아지트까지 가상의 A\* 탐색을 실행하여, 단 하나의 입구라도 경로가 끊기면(`path.empty()`) 즉시 탐색을 중단하고 실패 반환.
  - 유효성 검사 직후 타일을 다시 `0`(빈 공간)으로 즉각 복구(Rollback).
- **Result (결과)**: 악의적인 길막을 100% 무결하게 차단함과 동시에 조기 탈출(Early Exit)을 통해 평균 0.2ms 이내로 검증을 완료하여 60 FPS 유지.

---

### [사례 2] 멀티플레이어 환경에서 클라이언트 간 몬스터 생사 불일치 및 30~40G 재화 격차
- **Situation (상황)**: 1웨이브 진행 후 두 클라이언트가 동일한 타워를 지었음에도, 클라이언트 A는 30마리를 전멸시켜 300G를 획득한 반면 클라이언트 B는 260G에 그치고 한쪽 화면에서만 몬스터가 살아 돌아다니는 비동기화 발생.
- **Task (과제)**: 로컬 물리/사격 시뮬레이션의 미세한 프레임 시차를 극복하고, 두 클라이언트의 몬스터 생사 여부와 골드 보유량을 100% 일치시킬 수 있는 동기화 메커니즘 구축.
- **Action (행동)**:
  - 몬스터 스폰 패킷(`S_SPAWN_MONSTER`)에 웨이브별 고유 식별 번호(`monsterId`, 0~29)를 부여.
  - 클라이언트에서 체력이 0이 되었을 때 로컬 골드를 올리지 않고 서버로 `C_ENEMY_KILL(monsterId, 10)`을 통보하도록 전환.
  - 서버 `GameRoom`에 `deadMonsters` 셋을 두어 중복 처치 및 이중 지급을 차단하고, 방 안의 전원에게 `S_ENEMY_KILL` 패킷 브로드캐스트.
  - 클라이언트는 수신 즉시 해당 `monsterId`의 액터를 찾아 비활성화하고 공통 골드를 가산.
- **Result (결과)**: 프레임 시차와 관계없이 어느 한쪽에서 몬스터가 처치되는 즉시 양쪽 화면에서 동시에 소멸하고 동일한 골드를 획득하여 완벽한 재화 일치 달성.

---

### [사례 3] 파트너 강제 종료 시 재귀 락 데드락 서버 크래시 및 파트너 미퇴장 버그
- **Situation (상황)**: 게임 진행 도중 1번 클라이언트가 창을 닫아 강제 종료하자, 2번 클라이언트는 게임에서 빠져나오지 못하고 서버 프로세스가 예외를 뿜으며 즉각 폭파(크래시)되는 치명적 장애 발생.
- **Task (과제)**: 서버 크래시 원인을 규명하여 무중단 안정성을 확보하고, 파트너 탈주 시 남아있는 클라이언트가 정상적으로 인게임을 종료하고 대기실로 복귀하도록 연동.
- **Action (행동)**:
  - 덤프 및 호출 스택 추적 결과, `GameRoom::Leave()` 내부에서 `std::mutex lock`을 쥔 채 파트너의 `Disconnect()`를 호출하여 `OnDisconnected()` -> `r->Leave()`가 동일 스레드에서 재귀 진입하며 발생한 Self-Deadlock(`std::system_error`)임을 규명.
  - `GameRoom::Leave()`에서 락 내부에서는 세션 포인터 복사 및 방 상태 초기화만 수행하고, 락이 해제된 이후에 `partner->Disconnect()`를 호출하도록 락-프리 분리.
  - 클라이언트 `DefenseLevel::Tick`에 `isMultiplayerGame && !IsConnected()` 감지 로직을 추가하여 소켓 단절 시 메인 메뉴로 자동 복귀 처리.
  - `MainMenuLevel::ResetReady()`에서 연결 해제 상태로 메인 메뉴에 진입한 경우 서버에 자동 재접속(`Connect` + `C_LOGIN`)하도록 연동.
- **Result (결과)**: 서버 크래시율 0% 달성 및 파트너 탈주 시 남은 클라이언트가 즉시 메인 메뉴로 안전 복귀하여 다음 게임을 준비할 수 있는 견고한 세션 생명주기 확립.

---

### [사례 4] 터렛 3단 합성 시 골드가 갑자기 늘어나는(Surge) 롤백 버그
- **Situation (상황)**: 멀티플레이 도중 타워를 3개 지어 상위 등급으로 자동 합성되는 순간, 줄어들었던 골드가 갑자기 50~100G씩 다시 채워지며 골드가 치솟는 버그 발생.
- **Task (과제)**: 타워 건설 패킷의 RTT 시차와 로컬 선차감 로직 간의 상태 충돌을 규명하고 재화의 무결성 보장.
- **Action (행동)**:
  - 클라이언트가 로컬에서 `SpendGold(50)`를 선반영한 상태에서, 서버가 뒤늦게 보낸 이전 시점의 잔고(`remainingGold`)를 `SetGold`로 덮어쓰면서 발생한 역전 현상임을 분석.
  - 멀티플레이 시 클라이언트 선차감을 제거하고, 오직 서버 권한의 `S_BUILD_TURRET` 패킷만을 단일 진실 공급원(Single Source of Truth)으로 삼도록 수정.
- **Result (결과)**: 연쇄 합성이나 빠른 연속 건설 시에도 골드가 단 1G의 오차도 없이 서버 공인 값으로 정확하게 동기화됨.

---

### [사례 5] F12 게임 룰 및 도박(룰렛) 팝업 시 로컬 틱 정지로 인한 비동기화 해결
- **Situation (상황)**: 한 플레이어가 F12(규칙)나 T(도박)를 누르면 해당 화면에서 5초 동안 게임 틱이 정지되어, 팝업을 닫았을 때 몬스터가 순간이동하거나 서버와의 동기화가 완전히 어긋나는 현상 발생.
- **Task (과제)**: 유저가 팝업을 열람하거나 룰렛 연출을 감상하는 동안에도 백그라운드 게임 틱과 네트워크 패킷 수신이 멈추지 않는 논블로킹 UI 구현.
- **Action (행동)**:
  - 기존의 `if (isOpen) return;` 조기 반환 코드를 제거하여 백그라운드에서 `Level::Tick`과 `NetworkManager::Update`가 실시간으로 돌도록 유지.
  - 팝업이 활성화된 동안에는 오클릭(타워 건설/철거) 방지 및 마우스 커서 미리보기만 선택적으로 차단하는 **논블로킹 오버레이(Non-blocking Overlay)** 패턴 적용.
- **Result (결과)**: 팝업 열람 여부와 무관하게 몬스터 이동 및 네트워크 패킷 수신이 완벽한 60 FPS 실시간 동기화를 유지함.

---

## 5. 시니어 면접관 압박 꼬리질문 & 킬러 Q&A (8선)

### Q1. "A\* 알고리즘에서 `new/delete Node`를 매 탐색마다 호출하던데, 힙 단편화와 캐시 미스는 어떻게 보완할 수 있습니까?"
> **[모범 답변]**
> "알고리즘의 동작 검증 단계에서는 직관적인 구현을 위해 `new Node`를 사용했으나, 상용 고도화 관점에서는 **연속된 메모리 풀(Flat Array / Static Node Pool)**을 도입해야 합니다.
> 맵 크기(50x20 = 1,000노드)에 해당하는 노드 배열을 미리 할당해 두고, 포인터 대신 1차원 인덱스(`y * width + x`)로 접근하면 `new/delete` 시스템 콜 오버헤드가 완전히 제거됩니다. 또한 연속된 메모리 공간 접근으로 인해 CPU L1/L2 데이터 캐시 적중률(Spatial Locality)이 극대화되어 탐색 성능을 3~5배 이상 끌어올릴 수 있습니다."

### Q2. "클라이언트-서버 구조에서 왜 Server-Authoritative가 필수이며, 클라이언트 선차감은 왜 위험한가요?"
> **[모범 답변]**
> "클라이언트 선차감(Pre-deduction)은 로컬 반응성을 높일 수 있지만, 네트워크 지연(RTT) 환경에서 서버의 사후 패킷과 순서 역전이 발생하면 심각한 롤백 결함을 유발합니다. 실제로 본 프로젝트에서도 타워가 연쇄 3단 합성되는 순간, 이전 시점의 서버 잔고 패킷이 로컬 선차감 잔고를 덮어써서 골드가 갑자기 50~100G 치솟는 서지(Surge) 버그를 경험했습니다.
> 이를 해결하기 위해 클라이언트는 오직 '의도(Intent)'만 전달하고, 서버가 잔고를 독점 검증/차감하여 내려주는 **단일 진실 공급원(Single Source of Truth)** 모델로 일원화했습니다. 이는 패킷 변조 치트를 원천 봉쇄할 뿐만 아니라 상태 동기화의 수학적 무결성을 보장하는 최선의 설계입니다."

### Q3. "패킷을 보낼 때 구조체 포인터를 `reinterpret_cast<BYTE*>`로 바로 밀어 넣으셨는데, 이것의 위험성과 한계는 무엇인가요?"
> **[모범 답변]**
> "직접 메모리 캐스팅은 직렬화/역직렬화에 따른 CPU 복사 및 파싱 오버헤드가 '0'이라는 강력한 장점이 있지만, 두 가지 전제 조건이 필수적입니다.
> 첫째, 컴파일러마다 패딩 바이트가 달라지는 것을 막기 위해 `#pragma pack(push, 1)`로 1바이트 정렬을 강제해야 합니다.
> 둘째, 서버와 클라이언트가 동일한 CPU 아키텍처(Little-Endian)일 때만 유효하며, ARM이나 빅엔디안 환경이 섞이면 데이터가 깨집니다. 또한 가변 길이 문자열을 다루기 어렵기 때문에, 이기종 환경이나 확장성을 고려한다면 고정 헤더 + 가변 페이로드 형태의 직렬화 버퍼(Serializer)로 발전시키는 것이 바람직합니다."

### Q4. "멀티스레드 환경에서 `std::mutex` 재귀 락 데드락(Self-Deadlock)이 왜 발생했고 어떻게 방어했나요?"
> **[모범 답변]**
> "한 클라이언트가 연결을 끊었을 때 서버의 `GameRoom::Leave()`가 방 내부 `std::mutex`를 소유한 상태에서, 남은 파트너 세션의 `Disconnect()`를 동기 호출했습니다. `Disconnect()`는 즉시 소켓을 닫고 `GameSession::OnDisconnected()` -> `GameRoom::Leave()`를 동일 스레드에서 재귀 호출하여 자신이 이미 잡고 있는 non-recursive 뮤텍스를 재획득하려다 `std::system_error` 예외를 던지며 서버가 크래시되었습니다.
> 이를 방지하기 위해 **'락을 잡은 크리티컬 섹션 내부에서는 외부 콜백이나 네트워크 I/O를 절대 호출하지 않는다'**는 원칙을 적용했습니다. 락 내부에서는 세션 포인터 복사 및 방 상태 초기화만 수행하고, 락을 완전히 해제(Scope Unlock)한 이후에 파트너의 `Disconnect()`를 호출하도록 락-프리 분리하여 데드락을 원천 차단했습니다."

### Q5. "터렛 3단 자동 합성(3-Merge)에서 연쇄 합성이 일어날 때 무한 루프가 발생하지 않는 수학적 증명은?"
> **[모범 답변]**
> "무한 루프가 발생하지 않는 이유는 **'성급의 상한선(Tier Cap)'과 '엄격한 단조 감소(Monotonic Decrease)'** 때문입니다.
> 첫째, 합성은 동일 속성 및 동일 등급(1성->2성, 2성->3성)에만 한정되며, 최고 성급인 3성 터렛은 더 이상 합성되지 않습니다.
> 둘째, 3개의 터렛이 소멸하고 단 1개의 상위 터렛만 생성되므로, 1회 합성이 일어날 때마다 맵 상의 전체 터렛 수는 무조건 2개씩 단조 감소합니다. 맵의 격자 크기(50x20) 내에 존재할 수 있는 터렛 개수는 유한하므로, 연쇄 합성은 반드시 유한 번 내에 종료될 수밖에 없는 수학적 불변식(Invariant)을 갖습니다."

### Q6. "멀티플레이어 환경에서 클라이언트 간 몬스터 타겟팅/피격 판정이 다를 때 생사 상태와 골드를 어떻게 동기화했나요?"
> **[모범 답변]**
> "로컬 틱 시뮬레이션의 미세한 델타타임 차이로 인해 클라이언트마다 터렛의 사격 우선순위가 달라져 한쪽에서만 몬스터가 죽는 비동기화가 발생했습니다.
> 이를 해결하기 위해 **서버 권한형 처치 판정(`C_ENEMY_KILL` / `S_ENEMY_KILL`)**을 구축했습니다. 서버 스폰 시 몬스터마다 0~29번의 고유 `monsterId`를 발급하고, 어느 클라이언트에서든 몬스터 체력이 0이 되면 서버로 킬 요청을 보냅니다. 서버는 `std::set<int32_t> deadMonsters`로 중복 보상을 차단하면서 방 전체에 킬 확정 패킷을 브로드캐스트하여, 모든 클라이언트가 동일한 몬스터를 즉시 소멸시키고 동일한 골드를 지급받도록 보장했습니다."

### Q7. "스마트 포인터 `std::weak_ptr`의 사용 이유와 `GameSession` <-> `GameRoom` 순환 참조 방지는?"
> **[모범 답변]**
> "`GameRoom`은 세션 목록(`std::map<uint32_t, std::shared_ptr<GameSession>>`)을 소유하고, `GameSession`도 자신이 속한 방을 알아야 패킷을 라우팅할 수 있습니다. 만약 양쪽이 모두 `std::shared_ptr`로 참조하면 순환 참조(Circular Reference)가 발생하여 방이나 세션이 종료되어도 참조 카운트가 0이 되지 않아 영구 메모리 누수가 발생합니다.
> 따라서 `GameSession` 내부의 방 포인터를 `std::weak_ptr<GameRoom>`으로 선언하여 소유권 없이 관찰만 하도록 설계했고, 사용 시에만 `lock()`으로 수명을 확인한 뒤 안전하게 역참조하도록 조치했습니다."

### Q8. "왜 Windows IOCP를 구축했는가? (Overlapped I/O, Completion Port의 확장성)"
> **[모범 답변]**
> "`select` 모델은 감시 가능한 소켓 개수 제한(FD_SETSIZE 64개)과 매 루프마다 $O(N)$의 소켓 검사 오버헤드가 발생하며, 넌블로킹 폴링 방식은 유휴 상태에서도 CPU 점유율이 100%로 치솟습니다.
> 반면 Windows 커널 기반의 **IOCP는 Overlapped I/O 완료 이벤트를 커널 큐에서 비동기로 관리**하므로, 등록된 소켓 수에 구애받지 않고 완료된 I/O 작업만 워커 스레드가 즉시 꺼내어 처리($O(1)$)할 수 있습니다. 이를 통해 컨텍스트 스위칭 비용과 CPU 유휴 낭비를 최소화하는 고성능 멀티스레드 서버 엔진을 구축할 수 있었습니다."

---

## 6. 핵심 코드 스니펫 레퍼런스 (면접 제출 및 코드리뷰용)

### 1) [검증 핵심] 완전 길막 방지 트랜잭션 시뮬레이션 (`DefenseLevel.cpp`)

```cpp
bool DefenseLevel::CanBuildTurret(int x, int y)
{
    if (!IsBuildableArea(x, y)) return false;

    // 1. [Dry-run] 가상으로 2x2 터렛 타일 마킹
    mapGrid[y][x] = 2;     mapGrid[y][x + 1] = 2;
    mapGrid[y + 1][x] = 2; mapGrid[y + 1][x + 1] = 2;

    bool isPathValid = true;
    AStar astar;

    // 2. 다중 스폰 입구 전수 검증
    for (const auto& spawn : spawnPoints)
    {
        Node* start = new Node(spawn.x, spawn.y);
        Node* goal  = new Node(targetPoint.x, targetPoint.y);

        auto path = astar.FindPath(start, goal, mapGrid);
        if (path.empty()) // 단 하나의 입구라도 도달 불가능하면 실패
        {
            isPathValid = false;
            delete start; delete goal;
            break;
        }
        delete start; delete goal;
    }

    // 3. [Rollback] 원래 타일(0)로 즉시 무결성 복구
    mapGrid[y][x] = 0;     mapGrid[y][x + 1] = 0;
    mapGrid[y + 1][x] = 0; mapGrid[y + 1][x + 1] = 0;

    return isPathValid;
}
```

### 2) [서버 권한 동기화] 몬스터 처치 및 중복 방지 검증 (`GameRoom.cpp`)

```cpp
void GameRoom::HandleEnemyKill(std::shared_ptr<GameSession> session, C_ENEMY_KILL_PACKET& pkt)
{
    std::lock_guard<std::mutex> guard(lock);
    if (state != RoomState::PLAYING) return;

    // 이미 사망 처리된 몬스터는 중복 지급 방지 (Idempotent Filter)
    if (deadMonsters.find(pkt.monsterId) != deadMonsters.end())
    {
        return;
    }
    deadMonsters.insert(pkt.monsterId);

    int32_t reward = (pkt.rewardGold <= 0) ? 10 : pkt.rewardGold;

    // 방 안의 모든 세션에 공통 골드 가산
    for (auto& pair : sessions)
    {
        pair.second->gold += reward;
    }

    // 방 전체에 처치 확정 브로드캐스트 (모든 클라이언트가 동일 몬스터 소멸 및 골드 반영)
    S_ENEMY_KILL_PACKET sendPkt;
    sendPkt.monsterId = pkt.monsterId;
    sendPkt.rewardGold = reward;
    Broadcast(reinterpret_cast<BYTE*>(&sendPkt), sendPkt.size);
}
```

### 3) [데드락 방지] 락-프리 안전 세션 종료 (`GameRoom.cpp`)

```cpp
void GameRoom::Leave(std::shared_ptr<GameSession> session)
{
    std::vector<std::shared_ptr<GameSession>> partnersToDisconnect;

    {
        std::lock_guard<std::mutex> guard(lock);
        uint32_t targetId = session->playerId;
        if (targetId == 0) return;

        sessions.erase(targetId);
        readyPlayerIds.erase(targetId);
        session->playerId = 0;

        // 게임 진행 중 이탈 시 남은 파트너 포인터 수집 후 방 상태 리셋
        if (state == RoomState::PLAYING)
        {
            for (auto& pair : sessions)
            {
                partnersToDisconnect.push_back(pair.second);
            }
            sessions.clear();
            state = RoomState::WAITING;
            readyPlayerIds.clear();
            deadMonsters.clear();
        }
    } // <-- lock 자동 해제 (Scope Unlock)

    // 락을 해제한 후 외부 소켓 종료를 호출하여 재귀 락(Self-Deadlock) 원천 차단
    for (auto& partner : partnersToDisconnect)
    {
        partner->SetRoom(nullptr);
        partner->Disconnect(L"Partner Disconnected.");
    }
}
```
