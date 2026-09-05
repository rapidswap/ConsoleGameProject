#include "GameRoom.h"
#include "Network/GameSession.h"
#include <iostream>
#include <algorithm>

RoomState GameRoom::GetState()
{
	std::lock_guard<std::mutex> guard(lock);
	return state;
}

size_t GameRoom::GetSessionCount()
{
	std::lock_guard<std::mutex> guard(lock);
	return sessions.size();
}

bool GameRoom::IsEmpty()
{
	std::lock_guard<std::mutex> guard(lock);
	return sessions.empty();
}

void GameRoom::Enter(std::shared_ptr<GameSession> session, const char* playerName)
{
	std::lock_guard<std::mutex> guard(lock);

	// 이미 입장했던 세션이면 ID를 새로 발급하지 않고 기존 정보만 재전송.
	if (session->playerId != 0 && sessions.find(session->playerId) != sessions.end())
	{
		S_LOGIN_OK_PACKET loginOkPkt;
		loginOkPkt.playerId = session->playerId;
		loginOkPkt.currentGold = 100;
		session->Send(reinterpret_cast<BYTE*>(&loginOkPkt), loginOkPkt.size);
		BroadcastRoomInfo();
		return;
	}

	// 1. 고유 플레이어 번호 부여
	uint32_t newId = playerIdGenerator++;
	session->playerId = newId;
	strncpy_s(session->playerName, playerName, sizeof(session->playerName));
	session->totalGoldSpent = 0;

	// 2. 방 목록에 추가.
	sessions[newId] = session;

	std::cout << "[GameRoom #" << roomId << "] Player Enter -> ID: " << newId
		<< ", Name: " << playerName
		<< " (Current Players: " << sessions.size() << ")\n";

	// 3. 본인에게 로그인 성공 답장 보내기.
	S_LOGIN_OK_PACKET loginOkPkt;
	loginOkPkt.playerId = newId;
	loginOkPkt.currentGold = 100;
	session->Send(reinterpret_cast<BYTE*>(&loginOkPkt), loginOkPkt.size);

	// 4. 방에 있는 다른 모든 사람에게 시스템 채팅 방송.
	S_CHAT_PACKET alertPkt;
	alertPkt.playerId = 0;
	sprintf_s(alertPkt.msg, "[System] %s joined the room!", playerName);
	Broadcast(reinterpret_cast<BYTE*>(&alertPkt), alertPkt.size);

	// 5. 방에 접속한 모든 클라이언트에게 최신 플레이어 수 및 레디 상태 실시간 전송!
	BroadcastRoomInfo();
}

void GameRoom::Leave(std::shared_ptr<GameSession> session)
{
	std::lock_guard<std::mutex> guard(lock);
	uint32_t targetId = session->playerId;
	if (targetId == 0)
	{
		return;
	}

	// 방 목록에서 제거.
	sessions.erase(targetId);
	readyPlayerIds.erase(targetId);

	std::cout << "[GameRoom #" << roomId << "] Player Leave -> ID: " << targetId
		<< " (Current Players: " << sessions.size() << ")\n";

	// 모든 플레이어가 나갔다면 방 상태 초기화
	if (sessions.empty())
	{
		state = RoomState::WAITING;
		readyPlayerIds.clear();
		waveCount = 1;
		isWaveActive = false;
		waveTimer = 30.0f;
		spawnTimer = 0.0f;
		spawnedCount = 0;
		return;
	}

	// 방에 남은 사람들에게 퇴장 알림 방송.
	S_CHAT_PACKET alertPkt;
	alertPkt.playerId = 0;
	sprintf_s(alertPkt.msg, "[System] Player %d left the room.", targetId);
	Broadcast(reinterpret_cast<BYTE*>(&alertPkt), alertPkt.size);

	// 남은 플레이어에게 최신 인원수 및 레디 상태 전송.
	BroadcastRoomInfo();
}

void GameRoom::Broadcast(BYTE* buffer, int32_t len)
{
	// 방에 있는 모든 플레이어의 소켓으로 일제히 전송.
	for (auto& pair : sessions)
	{
		pair.second->Send(buffer, len);
	}
}

void GameRoom::HandleReady(std::shared_ptr<GameSession> session, C_READY_PACKET& pkt)
{
	std::lock_guard<std::mutex> guard(lock);

	// 이미 게임 진행 중이면 무시.
	if (state == RoomState::PLAYING)
	{
		return;
	}
	
	readyPlayerIds.insert(session->playerId);
	std::cout << "[GameRoom #" << roomId << "] Player " << session->playerId << " READY! ("
		<< readyPlayerIds.size() << "/" << sessions.size() << ")\n";

	BroadcastRoomInfo();

	// 시작 조건:
	// 1) 2명 이상 접속해 있고 모두 레디 완료 했거나
	// 2) 1명 접속 상태에서 강제 시작을 누른 경우.
	if ((sessions.size() >= 2 && readyPlayerIds.size() >= sessions.size()) || (sessions.size() == 1 && pkt.forceStart))
	{
		StartGame();
	}
}

void GameRoom::BroadcastRoomInfo()
{
	S_ROOM_INFO_PACKET pkt;
	pkt.totalPlayers = static_cast<int32_t>(sessions.size());
	pkt.readyPlayers = static_cast<int32_t>(readyPlayerIds.size());
	Broadcast(reinterpret_cast<BYTE*>(&pkt), pkt.size);
}

void GameRoom::StartGame()
{
	state = RoomState::PLAYING;
	waveCount = 1;
	isWaveActive = false;
	waveTimer = 30.0f;
	spawnTimer = 0.0f;
	spawnedCount = 0;

	for (auto& pair : sessions)
	{
		pair.second->gold = 100;
		pair.second->totalGoldSpent = 0;
	}

	std::cout << "[GameRoom #" << roomId << "] *** GAME START! (30s Preparation Time) ***\n";

	// 모든 클라이언트에게 동시에 게임 시작 신호 브로드캐스트.
	S_GAME_START_PACKET startPkt;
	startPkt.totalPlayers = static_cast<int32_t>(sessions.size());
	startPkt.prepTime = 30.0f;
	Broadcast(reinterpret_cast<BYTE*>(&startPkt), startPkt.size);
}

void GameRoom::HandleGameOver()
{
	std::lock_guard<std::mutex> guard(lock);

	if (state != RoomState::PLAYING)
	{
		return;
	}

	std::cout << "[GameRoom #" << roomId << "] *** GAME OVER! Resetting Room to WAITING... ***\n";

	state = RoomState::WAITING;
	readyPlayerIds.clear(); // 모든 플레이어의 레디 상태 초기화
	waveCount = 1;
	isWaveActive = false;
	waveTimer = 30.0f;
	spawnTimer = 0.0f;
	spawnedCount = 0;

	// 게임 오버 패킷 전송
	S_GAME_OVER_PACKET overPkt;
	Broadcast(reinterpret_cast<BYTE*>(&overPkt), overPkt.size);

	// 대기실 현황(레디 0명) 브로드캐스트
	BroadcastRoomInfo();
}

void GameRoom::HandleLeaveRoom(std::shared_ptr<GameSession> session)
{
	std::lock_guard<std::mutex> guard(lock);

	std::cout << "[GameRoom #" << roomId << "] Player " << session->playerId << " returned to Lobby (LeaveRoom)\n";

	// 1. 방 상태를 대기실(WAITING)로 리셋 및 레디 초기화
	state = RoomState::WAITING;
	readyPlayerIds.clear();
	waveCount = 1;
	isWaveActive = false;
	waveTimer = 30.0f;
	spawnTimer = 0.0f;
	spawnedCount = 0;

	// 2. 다른 플레이어들에게 알림 방송
	S_CHAT_PACKET alertPkt;
	alertPkt.playerId = 0;
	sprintf_s(alertPkt.msg, "[System] Player %d returned to lobby.", session->playerId);
	Broadcast(reinterpret_cast<BYTE*>(&alertPkt), alertPkt.size);

	// 3. 최신 방 정보(레디 0명) 브로드캐스트
	BroadcastRoomInfo();
}

void GameRoom::HandleChat(std::shared_ptr<GameSession> session, C_CHAT_PACKET& pkt)
{
	std::lock_guard<std::mutex> guard(lock);

	std::cout << "[GameRoom #" << roomId << " Chat] Player " << session->playerId << ": " << pkt.msg << "\n";

	// 나한테만 보내는게 아닌 방에 모든 사람에게 방송.
	S_CHAT_PACKET sendPkt;
	sendPkt.playerId = session->playerId;
	strcpy_s(sendPkt.msg, pkt.msg);

	Broadcast(reinterpret_cast<BYTE*>(&sendPkt), sendPkt.size);
}

void GameRoom::HandleBuildTurret(std::shared_ptr<GameSession> session, C_BUILD_TURRET_PACKET& pkt)
{
	std::lock_guard<std::mutex> guard(lock);

	const int turretCost = 50;

	// 서버 권한 검증: 보유 골드가 터렛 비용 이상인지 검사
	if (session->gold < turretCost)
	{
		std::cout << "[GameRoom #" << roomId << " Build FAIL] Player " << session->playerId
			<< " has insufficient gold (" << session->gold << " < " << turretCost << ")\n";

		S_BUILD_TURRET_PACKET sendPkt;
		sendPkt.success = false;
		sendPkt.playerId = session->playerId;
		sendPkt.posX = pkt.posX;
		sendPkt.posY = pkt.posY;
		sendPkt.turretType = pkt.turretType;
		sendPkt.remainingGold = session->gold;

		// 실패는 요청한 세션에게만 전송
		session->Send(reinterpret_cast<BYTE*>(&sendPkt), sendPkt.size);
		return;
	}

	// 검증 통과: 서버 측 골드 차감
	session->gold -= turretCost;

	std::cout << "[GameRoom #" << roomId << " Build SUCCESS] Player " << session->playerId
		<< " built Turret at (" << pkt.posX << ", " << pkt.posY << "), Remaining Gold: " << session->gold << "\n";

	// 방 안의 모든 플레이어에게 타워 생성 및 최신 잔여 골드 브로드캐스트
	S_BUILD_TURRET_PACKET sendPkt;
	sendPkt.success = true;
	sendPkt.playerId = session->playerId;
	sendPkt.posX = pkt.posX;
	sendPkt.posY = pkt.posY;
	sendPkt.turretType = pkt.turretType;
	sendPkt.remainingGold = session->gold;

	Broadcast(reinterpret_cast<BYTE*>(&sendPkt), sendPkt.size);
}

void GameRoom::HandleSellTurret(std::shared_ptr<GameSession> session, C_SELL_TURRET_PACKET& pkt)
{
	std::lock_guard<std::mutex> guard(lock);

	// 터렛 기본 비용 50G의 절반(25G)에 성급 배율 적용
	int multiplier = (pkt.starTier == 3) ? 9 : (pkt.starTier == 2) ? 3 : 1;
	int refund = 25 * multiplier;

	// 서버 측 골드 환불 가산
	session->gold += refund;

	std::cout << "[GameRoom #" << roomId << " Sell SUCCESS] Player " << session->playerId
		<< " sold Turret at (" << pkt.posX << ", " << pkt.posY
		<< ") [Tier: " << pkt.starTier << ", Refund: " << refund << "G, Total: " << session->gold << "G]\n";

	// 방 안의 모든 화면에 타워 철거 및 환불 골드 브로드캐스트
	S_SELL_TURRET_PACKET sendPkt;
	sendPkt.success = true;
	sendPkt.playerId = session->playerId;
	sendPkt.posX = pkt.posX;
	sendPkt.posY = pkt.posY;
	sendPkt.refundGold = refund;

	Broadcast(reinterpret_cast<BYTE*>(&sendPkt), sendPkt.size);
}

void GameRoom::Update(float deltaTime)
{
	std::lock_guard<std::mutex> guard(lock);

	// 게임 진행 중일 때만 몬스터 스폰 진행
	if (state != RoomState::PLAYING || sessions.empty())
	{
		return;
	}

	if (!isWaveActive)
	{
		// 1. 웨이브 준비 카운트다운 (30초 대기)
		waveTimer -= deltaTime;
		if (waveTimer <= 0.0f)
		{
			isWaveActive = true;
			spawnedCount = 0;
			spawnTimer = 0.0f;
			std::cout << "[GameRoom #" << roomId << "] Wave " << waveCount << " Started! Spawning 30 monsters...\n";
		}
	}
	else
	{
		// 2. 웨이브 진행 중: 30마리를 채울 때까지 1초마다 소환
		if (spawnedCount >= maxPerWave)
		{
			isWaveActive = false;
			++waveCount;
			if (waveCount > 3)
			{
				std::cout << "[GameRoom #" << roomId << "] All waves spawned.\n";
				return;
			}
			waveTimer = 90.0f;
			std::cout << "[GameRoom #" << roomId << "] Wave finished. Next wave in 90 seconds.\n";
			return;
		}

		spawnTimer += deltaTime;
		if (spawnTimer >= 1.0f)
		{
			spawnTimer = 0.0f;
			++spawnedCount;

			int spawnIdx = rand() % 4;
			int maxHp = 3 * waveCount;
			float speed = 2.0f;

			SpawnMonster(spawnIdx, maxHp, speed);
		}
	}
}

void GameRoom::SpawnMonster(int32_t spawnIndex, int32_t hp, float speed)
{
	S_SPAWN_MONSTER_PACKET pkt;
	pkt.spawnIndex = spawnIndex;
	pkt.maxHP = hp;
	pkt.speed = speed;

	std::cout << "[GameRoom #" << roomId << "] Monster SpawnPoint: " << spawnIndex << "\n";

	Broadcast(reinterpret_cast<BYTE*>(&pkt), pkt.size);
}

void GameRoom::HandleSpendGold(std::shared_ptr<GameSession> session, C_SPEND_GOLD_PACKET& pkt)
{
	std::lock_guard<std::mutex> guard(lock);
	session->totalGoldSpent = pkt.totalGoldSpent;
}

void GameRoom::HandleGameClear(std::shared_ptr<GameSession> session, C_GAME_CLEAR_PACKET& pkt)
{
	std::lock_guard<std::mutex> guard(lock);

	if (state != RoomState::PLAYING)
	{
		return;
	}

	session->totalGoldSpent = pkt.totalGoldSpent;

	std::cout << "[GameRoom #" << roomId << "] *** GAME CLEAR! Triggered by Player " << session->playerId << " ***\n";

	state = RoomState::WAITING;
	readyPlayerIds.clear();
	waveCount = 1;
	isWaveActive = false;
	waveTimer = 30.0f;
	spawnTimer = 0.0f;
	spawnedCount = 0;

	// 플레이어 정산 및 랭킹 산출 (소비한 골드가 많은 순서로 1등, 2등...)
	std::vector<std::shared_ptr<GameSession>> playerList;
	for (auto& pair : sessions)
	{
		playerList.push_back(pair.second);
	}

	std::sort(playerList.begin(), playerList.end(), [](const std::shared_ptr<GameSession>& a, const std::shared_ptr<GameSession>& b) {
		return a->totalGoldSpent > b->totalGoldSpent;
	});

	S_GAME_CLEAR_PACKET clearPkt;
	clearPkt.playerCount = static_cast<int32_t>(playerList.size());
	for (int i = 0; i < clearPkt.playerCount && i < 4; ++i)
	{
		clearPkt.records[i].playerId = playerList[i]->playerId;
		strncpy_s(clearPkt.records[i].playerName, playerList[i]->playerName, sizeof(clearPkt.records[i].playerName));
		clearPkt.records[i].totalGoldSpent = playerList[i]->totalGoldSpent;
		clearPkt.records[i].rank = i + 1;
		std::cout << "[GameRoom #" << roomId << "] Rank " << (i + 1) << ": Player " << clearPkt.records[i].playerId
			<< " (" << clearPkt.records[i].playerName << ") spent " << clearPkt.records[i].totalGoldSpent << "G\n";
	}

	Broadcast(reinterpret_cast<BYTE*>(&clearPkt), clearPkt.size);
	BroadcastRoomInfo();
}

// -------------------------------------------------------------
// GameRoomManager
// -------------------------------------------------------------

void GameRoomManager::EnterRoom(std::shared_ptr<GameSession> session, const char* playerName)
{
	std::lock_guard<std::mutex> guard(managerLock);

	// 이미 특정 방에 속해있는 경우
	if (auto existingRoom = session->GetRoom())
	{
		existingRoom->Enter(session, playerName);
		return;
	}

	// 대기 중(WAITING)이고 2인 미만인 방 탐색
	std::shared_ptr<GameRoom> targetRoom = nullptr;
	for (auto& room : rooms)
	{
		if (room->GetState() == RoomState::WAITING && room->GetSessionCount() < 2)
		{
			targetRoom = room;
			break;
		}
	}

	// 적합한 대기실이 없으면 새 방 생성
	if (targetRoom == nullptr)
	{
		targetRoom = std::make_shared<GameRoom>(nextRoomId++);
		rooms.push_back(targetRoom);
		std::cout << "[GameRoomManager] Created New GameRoom #" << targetRoom->GetRoomId() << "\n";
	}

	session->SetRoom(targetRoom);
	targetRoom->Enter(session, playerName);
}

void GameRoomManager::Update(float deltaTime)
{
	std::vector<std::shared_ptr<GameRoom>> activeRooms;
	{
		std::lock_guard<std::mutex> guard(managerLock);
		// 플레이어가 한 명도 없는 빈 방 정리
		rooms.erase(std::remove_if(rooms.begin(), rooms.end(), [](const std::shared_ptr<GameRoom>& room) {
			return room->IsEmpty();
		}), rooms.end());

		activeRooms = rooms;
	}

	for (auto& room : activeRooms)
	{
		room->Update(deltaTime);
	}
}
