# Console Game Project & SK Defense 종합 분석 및 인수인계 가이드

본 문서는 `ConsoleGameProject` 저장소의 전체 아키텍처, 엔진 구조, 프로젝트 히스토리, 멀티스레드/네트워크 통신 규격, 최신 버그 수정 및 향후 개발 로드맵을 기록하여 향후 지속적인 유지보수와 기능 확장이 가능하도록 작성된 기술 인수인계 문서입니다.

---

## 1. 전체 솔루션 구성 및 아키텍처 개요

본 저장소(`ConsoleGameProject.slnx`)는 C++20 기반의 독자적인 콘솔 게임 프레임워크와 이를 응용한 다수의 게임 프로젝트, 그리고 고성능 Windows IOCP 기반 전용 멀티플레이어 서버로 구성되어 있습니다.

```
ConsoleGameProject/
├── CraftEngine/           # [Core] DLL 형태의 2D 콘솔 게임 엔진 (더블 버퍼링, RTTI, 씬, 액터)
├── ServerCore/            # [Network] IOCP 네트워크 코어 정적 라이브러리 (.lib)
├── SK_Defense_Server/     # [Server] IOCP 기반 멀티플레이어 디펜스 게임 서버 (GameRoom, Session)
├── Project_SKDefense/     # [Client] 성큰 디펜스 스타일 2D 협동 타워 디펜스 게임
├── PEACEMAKER/            # 뱀파이어 서바이벌 스타일 탄막 액션 게임
├── ShootingGame/          # 1945 스타일 탑뷰 종스크롤 슈팅 게임
├── Game/                  # 소코반(Sokoban) 스타일 타일 퍼즐 게임
├── Config/                # Setting.txt (화면 해상도 120x30, 프레임레이트 등 엔진 설정)
├── Assets/                # 맵 데이터 (*.txt), 스테이지 파일
├── Includes/              # CraftEngine 컴파일 시 자동 동기화되는 공용 헤더 모음
├── Lib/                   # 빌드 결과물 라이브러리 (.lib)
└── Docs/                  # 기획서, 개발 로그 및 아키텍처 가이드
```

---

## 2. 서브 프로젝트별 상세 명세

### 1) CraftEngine (엔진 코어 DLL)
- **역할**: 게임 루프(`Run`), 더블 버퍼링 콘솔 렌더러(`Renderer`, `ScreenBuffer`), Win32 마우스/키보드 입력(`Input`), AABB 박스 충돌 시스템(`CollisionSystem`), 씬 관리자(`Level`), 오브젝트 RTTI 및 계층 구조(`CraftObject`, `Actor`).
- **핵심 메커니즘**:
  - `ScreenBuffer`: 화면 깜빡임(Flickering)을 제거하기 위해 2개의 콘솔 스크린 버퍼(`CHAR_INFO`)를 스왑하는 더블 버퍼링.
  - `TYPE_DECLARATIONS(Type, ParentType)`: 가상 RTTI 및 단일 상속 타입 검증(`IsTypeOf<T>()`, `Cast<T>()`).
  - 해상도 설정: `Config/Setting.txt` 파싱 (`width = 120`, `height = 30`, `framerate = 120`).

### 2) ServerCore (정적 라이브러리)
- **역할**: Windows IOCP(I/O Completion Port) 커널 모델을 래핑한 고성능 비동기 네트워크 코어.
- **핵심 컴포넌트**:
  - `IocpCore`: `CreateIoCompletionPort` 및 `GetQueuedCompletionStatus` 기반 비동기 이벤트 디스패칭.
  - `Listener`: 클라이언트의 `accept` 전담 스레드 루프.
  - `Session`: `WSASend` / `WSARecv` 비동기 I/O 생명주기 관리 (`std::enable_shared_from_this`).
  - `RecvBuffer`: TCP 슬라이딩 윈도우(`readPos`, `writePos`)로 패킷 쪼개짐(Fragmentation)과 뭉침(Coalescing) 방지.
  - `Common/Protocol.h`: 바이트 정렬(`#pragma pack(push, 1)`) 패킷 구조체 정의.

### 3) SK_Defense_Server (전용 게임 서버)
- **역할**: 방 관리(`GameRoom`), 2인 동시 시작/준비 제어, 실시간 브로드캐스트, 서버 권한형(Server-Authoritative) 재화 및 타워/웨이브 동기화.
- **스레드 구조**:
  - 메인 루프 스레드 1개 (방 업데이트 주기 호출)
  - Accept 전담 스레드 1개
  - IOCP Worker 스레드 4개 (`iocpCore->Dispatch()`)

### 4) Project_SKDefense (클라이언트 게임)
- **장르**: 2D 콘솔 협동 성큰 디펜스.
- **주요 시스템**:
  - A* 최단 경로 탐색 및 실시간 장애물(터렛/벽) 회피 (`Algorithm/AStar`).
  - 모서리 뚫기 방지(`IsDiagonalBlocked`) 및 터렛 건설 시 완전 길막(Maze Block) 사전 검증.
  - 3-Merge 자동 합성 시스템 (1성 3개 -> 2성 자동 승급, 화염/얼음/전기 속성).
  - 멀티플레이 대기실(로비) 및 서버 미실행 시 오프라인 싱글플레이 완벽 지원.

---

## 3. 네트워크 통신 규격 & 트랜잭션 흐름

### 패킷 헤더 구조
```cpp
#pragma pack(push, 1)
struct PacketHeader {
    uint16 size; // 헤더 포함 패킷 전체 크기
    uint16 id;   // PacketType
};
#pragma pack(pop)
```

### 핵심 트랜잭션 흐름
1. **로비 입장 및 레디**:
   - `C_LOGIN_PACKET` -> `S_LOGIN_OK_PACKET` (초기 100G 및 고유 플레이어 ID 발급)
   - `C_READY_PACKET` -> 인원 충족 시 `S_GAME_START_PACKET` 전파로 양쪽 클라이언트 동시 레벨 진입
2. **타워 건설 (서버 권한형)**:
   - 클라이언트 `C_BUILD_TURRET_PACKET` 요청
   - 서버가 `session->gold >= 50` 검증 후 차감 및 `S_BUILD_TURRET_PACKET` 전체 브로드캐스트
   - 클라이언트는 브로드캐스트 수신 후 화면에 타워 배치 및 잔여 골드 동기화
3. **타워 판매**:
   - `C_SELL_TURRET_PACKET` 요청 -> 서버에서 성급 비례 환불액 계산 후 `S_SELL_TURRET_PACKET` 전파

---

## 4. 최근 주요 작업 내역 및 트러블슈팅

1. **플랫폼 툴셋 및 인코딩 정상화**:
   - VS2022 환경 호환을 위해 `Directory.Build.props`에 `/utf-8` 및 `v143` 툴셋 지정.
2. **싱글플레이 화면 동결(Freeze) 해결**:
   - 오프라인 상태에서 매 프레임 동기 `connect()` 호출로 1초간 UI가 멈추던 문제를 1회 접속 시도 플래그로 개선.
3. **메인 메뉴 화면 텍스트 중앙 정렬 픽스**:
   - 싱글 모드(오프라인) 상태에서 `"Press Enter Button."` 및 로비 상태 텍스트의 X 좌표가 고정 하드코딩되어 화면 정중앙에 맞지 않던 문제를, `(screenWidth / 2) - ((int)text.length() / 2)` 공식을 적용하여 문자열 길이에 따라 항상 완벽하게 화면 한가운데에 정렬되도록 수정.
4. **프로젝트 간 의존성 및 인클루드 경로 정렬**:
   - `Project_SKDefense`가 `ServerCore`의 `Common/Protocol.h`를 참조할 수 있도록 상대 경로 `..\ServerCore\` 추가.
   - `Protocol.h`에서 `pch.h` 참조를 `Main/pch.h`로 명시화.
   - `CraftEngine` 최신 빌드 후 `Includes/` 및 `Lib/` 자동 동기화.
5. **게임 클리어 화면 구현 & 누적 소비 골드 랭킹 정산 시스템**:
   - **누적 소비 골드 추적**: `DefenseLevel::SpendGold(int amount)`에서 터렛 건설(50G), 속성 업그레이드(100G), 랜덤 업그레이드(80G), 도박(300G) 등 모든 골드 지출 시 `totalGoldSpent`를 누적하고, 멀티플레이 시 `C_SPEND_GOLD_PACKET`으로 서버에 실시간 동기화.
   - **클리어 화면 UI (`GameClearLevel`)**:
     - 상단: `GAME CLEAR! ALL WAVES DEFENDED SUCCESSFULLY!` 축하 배너 출력.
     - 싱글플레이: 단독 누적 소비 골드 표시 및 지출 성향 평가(Master Tactician, Veteran Commander, Tycoon Defender) 출력.
     - 멀티플레이: 전원 소비 골드 랭킹 테이블(1위 MVP, 2위 Runner-Up, 본인 `(YOU)` 하이라이트) 출력.
     - 하단: `Restart Game`, `Quit to Main Menu` 메뉴 인터랙션(방향키 및 엔터).
   - **즉시 클리어 치트(F2) 서버 동기화**:
     - 클라이언트 단축키 `VK_F2` 입력 시 단독 클리어 대신 `C_GAME_CLEAR_PACKET(totalGoldSpent)`를 서버로 전송.
     - 서버(`GameRoom::HandleGameClear`)에서 모든 플레이어의 누적 지출을 집계하고 소비 골드가 높은 순서대로 1등/2등을 정렬한 뒤 `S_GAME_CLEAR_PACKET`을 방 전체에 브로드캐스트.
     - 모든 클라이언트가 동시에 `GameClearLevel`로 전환되며 동일한 랭킹 결과를 확인.


---

## 5. 향후 개발 및 확장 가이드라인 (Next Steps)

1. **디펜스 게임 몬스터 동기화 고도화**:
   - 현재 서버 웨이브 타이머(`S_SPAWN_MONSTER`)에 맞춰 클라이언트가 로컬 A*로 이동. 향후 몬스터 처치 및 HP 동기화 패킷 확장 가능.
2. **채팅 시스템 인게임 연동**:
   - `C_CHAT_PACKET` / `S_CHAT_PACKET` 프로토콜이 완성되어 있으므로, 인게임 UI 패널에 채팅 입력 및 로그 렌더러 추가 가능.
3. **유지보수 시 주의사항**:
   - `CraftEngine`의 헤더나 함수를 수정한 경우, 반드시 `CraftEngine`을 먼저 빌드하여 `Includes/`와 `Lib/`가 갱신된 후 클라이언트 프로젝트를 빌드해야 링크 오류(`LNK2019`)를 방지할 수 있습니다.
