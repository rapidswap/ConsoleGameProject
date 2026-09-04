#include "GameRoom.h"
#include "Network/GameSession.h"
#include <iostream>

// 전역 게임 룸 인스턴스 생성.
std::shared_ptr<GameRoom> GGameRoom = std::make_shared<GameRoom>();

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

	// 2. 방 목록에 추가.
	sessions[newId] = session;

	std::cout << "[GameRoom] Player Enter -> ID: " << newId
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

	// 모든 플레이어가 나간다면 방 상태 초기화
	if (sessions.size()<=1)
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



	std::cout << "[GameRoom] Player Leave -> ID: " << targetId
		<< " (Current Players: " << sessions.size() << ")\n";



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
	std::cout << "[GameRoom] Player " << session->playerId << " READY! ("
		<< readyPlayerIds.size() << "/" << sessions.size()<<"\n";

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

	std::cout << "[GameRoom] *** GAMESTART! (30s Preparation Time) ***\n";

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

	std::cout << "[GameRoom] *** GAME OVER! Resetting Room to WAITING... ***\n";

	state = RoomState::WAITING;
	readyPlayerIds.clear(); // [핵심] 모든 플레이어의 레디 상태 초기화!
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

	std::cout << "[GameRoom] Player " << session->playerId << " returned to Lobby (LeaveRoom)\n";

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

	std::cout << "[GameRoom Chat] Player " << session->playerId << ": " << pkt.msg << "\n";

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
		std::cout << "[GameRoom Build FAIL] Player " << session->playerId
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

	std::cout << "[GameRoom Build SUCCESS] Player " << session->playerId
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

	std::cout << "[GameRoom Sell SUCCESS] Player " << session->playerId
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
			std::cout << "[GameRoom] Wave " << waveCount << " Started! Spawning 30 monsters...\n";
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
				std::cout << "[GameRoom] All waves spawned.\n";
				return;
			}
			waveTimer = 90.0f;
			std::cout << "[GameRoom] Wave finished. Next wave in 90 seconds.\n";
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

	std::cout << "[GameRoom] Moster SpawnPoint: " << spawnIndex << "\n";

	Broadcast(reinterpret_cast<BYTE*>(&pkt),pkt.size);
	

}
