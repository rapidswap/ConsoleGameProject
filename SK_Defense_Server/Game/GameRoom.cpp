#include "GameRoom.h"
#include "Network/GameSession.h"
#include <Util/Util.h>
#include <iostream>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <cstring>

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
	if (session->playerId != 0 && sessions.find(session->playerId) != sessions.end() && sessions[session->playerId] == session)
	{
		S_LOGIN_OK_PACKET loginOkPkt;
		loginOkPkt.playerId = session->playerId;
		loginOkPkt.currentGold = 200;
		session->Send(reinterpret_cast<BYTE*>(&loginOkPkt), loginOkPkt.size);
		BroadcastRoomInfo();
		return;
	}

	// 1. 방 내 빈 슬롯 중 가장 낮은 번호(1번부터: 1, 2) 부여
	uint32_t newId = 1;
	while (sessions.find(newId) != sessions.end())
	{
		newId++;
	}

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
	loginOkPkt.currentGold = 200;
	session->Send(reinterpret_cast<BYTE*>(&loginOkPkt), loginOkPkt.size);

	// 4. 방에 있는 다른 모든 사람에게 시스템 채팅 방송.
	S_CHAT_PACKET alertPkt;
	alertPkt.playerId = 0;
	sprintf_s(alertPkt.msg, "[System] %s joined the room as Player %d!", playerName, newId);
	Broadcast(reinterpret_cast<BYTE*>(&alertPkt), alertPkt.size);

	// 5. 방에 접속한 모든 클라이언트에게 최신 플레이어 수 및 레디 상태 실시간 전송!
	BroadcastRoomInfo();
}

void GameRoom::Leave(std::shared_ptr<GameSession> session)
{
	std::vector<std::shared_ptr<GameSession>> partnersToDisconnect;

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
		session->playerId = 0;

		std::cout << "[GameRoom #" << roomId << "] Player Leave -> ID: " << targetId
			<< " (Current Players: " << sessions.size() << ")\n";

		// 플레이중에 플레이어가 한명 탈주 했다면.
		if (state == RoomState::PLAYING)
		{
			std::cout << "[GameRoom #" << roomId << "] Player " << targetId
				<< " left during PLAYING. Terminating co-op game for remaining player(s).\n";

			for (auto& pair : sessions)
			{
				partnersToDisconnect.push_back(pair.second);
			}
			sessions.clear();
			state = RoomState::WAITING;
			readyPlayerIds.clear();
			waveCount = 1;
			isWaveActive = false;
			waveTimer = 30.0f;
			spawnTimer = 0.0f;
			spawnedCount = 0;
			deadMonsters.clear();
		}
		else
		{
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
				deadMonsters.clear();
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
	}

	// 락을 해제한 후 남아있는 파트너 세션들을 안전하게 연결 해제
	for (auto& partner : partnersToDisconnect)
	{
		partner->SetRoom(nullptr);
		partner->Disconnect(L"Partner Disconnected.");
	}
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
	if (!LoadNavigationMap())
	{
		S_CHAT_PACKET error;
		strcpy_s(error.msg, "[System] Cannot start: invalid or missing SK_Defense_Map.txt.");
		Broadcast(reinterpret_cast<BYTE*>(&error), error.size);
		return;
	}
	turrets.clear();
	monsters.clear();
	deadMonsters.clear();
	pathCache.clear();
	++mapRevision;
	nextTurretOrder = 0;
	state = RoomState::PLAYING;
	waveCount = 1;
	isWaveActive = false;
	waveTimer = 30.0f;
	spawnTimer = 0.0f;
	spawnedCount = 0;
	agitHealth = 100;
	upgradeLevelFlame = 0;
	upgradeLevelIce = 0;
	upgradeLevelStorm = 0;

	int32_t startGold = (sessions.size() == 1) ? 350 : 200;

	for (auto& pair : sessions)
	{
		pair.second->gold = startGold;
		pair.second->totalGoldSpent = 0;
	}

	std::cout << "[GameRoom #" << roomId << "] *** GAME START! (30s Preparation Time) ***\n";

	// 각 플레이어마다 초기 골드 설정 및 서로다른 첫 번째 타워를 랜덤 추천하여 개별 전송.
	for (auto& pair : sessions)
	{
		auto& session = pair.second;
		session->gold = startGold;
		session->totalGoldSpent = 0;

		// 1. 서버가 이 플레이어의 첫 번째 타워를 랜덤 결정.
		session->nextTurretType = static_cast<int32_t>(Util::RandomRange(0, 2));

		// 2. 플레이어에게 보낼 시작 패킷 구성.
		S_GAME_START_PACKET startPkt;
		startPkt.totalPlayers = static_cast<int32_t>(sessions.size());
		startPkt.prepTime = 30.0f;
		startPkt.startGold = startGold;
		startPkt.initialTurretType = session->nextTurretType;

		session->Send(reinterpret_cast<BYTE*>(&startPkt), startPkt.size);

	}
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
	if (state != RoomState::PLAYING || session->gold < turretCost || !CanBuildTurret(pkt.posX, pkt.posY))
	{
		std::cout << "[GameRoom #" << roomId << " Build FAIL] Player " << session->playerId
			<< " has insufficient gold or invalid placement (Gold: " << session->gold << ")\n";

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
	session->totalGoldSpent += turretCost;

	// 클라이언트가 보낸 속성은 무시, 서버가 보관 중이던 타워로 확정.
	int32_t builtTurretType = session->nextTurretType;
	SetTurretTiles({pkt.posX, pkt.posY}, 2);
	turrets.push_back({{pkt.posX, pkt.posY}, builtTurretType, 1, nextTurretOrder++});

	session->nextTurretType = static_cast<int32_t>(Util::RandomRange(0, 2));



	std::cout << "[GameRoom #" << roomId << " Build SUCCESS] Player " << session->playerId
		<< " at (" << pkt.posX << ", " << pkt.posY << ")"
		<< " Built Type: " << builtTurretType
		<< " -> Next Type: " << session->nextTurretType
		<< " Remaining Gold: " << session->gold << "\n";


	// 방 안의 모든 플레이어에게 타워 생성 및 최신 잔여 골드 브로드캐스트
	S_BUILD_TURRET_PACKET sendPkt;
	sendPkt.success = true;
	sendPkt.playerId = session->playerId;
	sendPkt.posX = pkt.posX;
	sendPkt.posY = pkt.posY;
	sendPkt.turretType = builtTurretType;
	sendPkt.remainingGold = session->gold;
	sendPkt.nextTurretType = session->nextTurretType;

	Broadcast(reinterpret_cast<BYTE*>(&sendPkt), sendPkt.size);
	MergeTurrets();
	RefreshMonsterPaths();
}

void GameRoom::HandleSellTurret(std::shared_ptr<GameSession> session, C_SELL_TURRET_PACKET& pkt)
{
	std::lock_guard<std::mutex> guard(lock);
	if (state != RoomState::PLAYING) return;
	const auto turret = std::find_if(turrets.begin(), turrets.end(), [&](const ServerTurret& item) {
		return item.position == SKDefense::GridPoint{pkt.posX, pkt.posY};
	});
	if (turret == turrets.end()) return;

	// 터렛 기본 비용 50G의 절반(25G)에 성급 배율 적용
	const int tier = turret->tier;
	int multiplier = (tier == 3) ? 9 : (tier == 2) ? 3 : 1;
	int refund = 25 * multiplier;
	SetTurretTiles(turret->position, 0);
	turrets.erase(turret);

	// 서버 측 골드 환불 가산
	session->gold += refund;

	std::cout << "[GameRoom #" << roomId << " Sell SUCCESS] Player " << session->playerId
		<< " sold Turret at (" << pkt.posX << ", " << pkt.posY
		<< ") [Tier: " << tier << ", Refund: " << refund << "G, Total: " << session->gold << "G]\n";

	// 방 안의 모든 화면에 타워 철거 및 환불 골드 브로드캐스트
	S_SELL_TURRET_PACKET sendPkt;
	sendPkt.success = true;
	sendPkt.playerId = session->playerId;
	sendPkt.posX = pkt.posX;
	sendPkt.posY = pkt.posY;
	sendPkt.refundGold = refund;

	Broadcast(reinterpret_cast<BYTE*>(&sendPkt), sendPkt.size);
	RefreshMonsterPaths();
}

void GameRoom::Update(float deltaTime)

{
	std::lock_guard<std::mutex> guard(lock);

	// 게임 진행 중일 때만 몬스터 스폰 진행
	if (state != RoomState::PLAYING || sessions.empty())
	{
		return;
	}
	UpdateMonsters(deltaTime);
	if (state != RoomState::PLAYING || waveCount > 9) return;

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
			if (waveCount > 9)
			{
				std::cout << "[GameRoom #" << roomId << "] All 9 waves spawned.\n";
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

			int activeSpawns = 1;
			if (waveCount >= 4)
			{
				activeSpawns = 2;
			}

			if (waveCount >= 7)
			{
				activeSpawns = 3;
			}
			
			int baseHp = 3 * waveCount;

			activeSpawns = (std::min)(activeSpawns, static_cast<int>(spawnPoints.size()));
			int spawnIdx = static_cast<int>(Util::RandomRange(0, activeSpawns - 1));
			int maxHp = (sessions.size() >= 2) ? baseHp * 2 : baseHp;
			float speed = 2.0f;

			// Game-wide IDs keep previous-wave survivors and delayed reports distinct.
			int32_t monsterId = (waveCount - 1) * maxPerWave + spawnedCount - 1;
			SpawnMonster(monsterId, spawnIdx, maxHp, speed);
		}
	}
}

void GameRoom::SpawnMonster(int32_t monsterId, int32_t spawnIndex, int32_t hp, float speed)
{
	if (spawnIndex < 0 || spawnIndex >= static_cast<int32_t>(spawnPoints.size())) return;
	ServerMonster monster;
	monster.position = spawnPoints[spawnIndex];
	monster.speed = speed;
	monsters[monsterId] = std::move(monster);
	S_SPAWN_MONSTER_PACKET pkt;
	pkt.monsterId = monsterId;
	pkt.spawnIndex = spawnIndex;
	pkt.maxHP = hp;
	pkt.speed = speed;

	std::cout << "[GameRoom #" << roomId << "] Monster #" << monsterId << " SpawnPoint: " << spawnIndex << "\n";

	Broadcast(reinterpret_cast<BYTE*>(&pkt), pkt.size);
	AssignMonsterPath(monsterId, monsters.at(monsterId));
}

void GameRoom::HandleEnemyKill(std::shared_ptr<GameSession> session, C_ENEMY_KILL_PACKET& pkt)
{
	std::lock_guard<std::mutex> guard(lock);

	if (state != RoomState::PLAYING) return;

	// 이미 사망 처리된 몬스터는 중복 지급 방지
	if (monsters.find(pkt.monsterId) == monsters.end() || deadMonsters.find(pkt.monsterId) != deadMonsters.end())
	{
		return;
	}

	deadMonsters.insert(pkt.monsterId);
	monsters.erase(pkt.monsterId);

	int32_t reward = (pkt.rewardGold <= 0) ? 10 : pkt.rewardGold;

	// 방 안의 모든 플레이어에게 골드 지급
	for (auto& pair : sessions)
	{
		pair.second->gold += reward;
	}

	std::cout << "[GameRoom #" << roomId << "] Monster #" << pkt.monsterId 
		<< " Killed (Reported by Player " << session->playerId << ") -> Awarded " << reward << "G to all players!\n";

	// 방 전체에 처치 확정 및 동기화 브로드캐스트
	S_ENEMY_KILL_PACKET sendPkt;
	sendPkt.monsterId = pkt.monsterId;
	sendPkt.rewardGold = reward;
	Broadcast(reinterpret_cast<BYTE*>(&sendPkt), sendPkt.size);
}

void GameRoom::HandleSpendGold(std::shared_ptr<GameSession> session, C_SPEND_GOLD_PACKET& pkt)
{
	std::lock_guard<std::mutex> guard(lock);
	session->totalGoldSpent = pkt.totalGoldSpent;
}

void GameRoom::HandleAddGold(std::shared_ptr<GameSession> session, C_ADD_GOLD_PACKET& pkt)
{
	std::lock_guard<std::mutex> guard(lock);
	if (pkt.amount <= 0) return;

	session->gold += pkt.amount;
	std::cout << "[GameRoom #" << roomId << "] Player " << session->playerId
		<< " Earned " << pkt.amount << "G (Server Gold: " << session->gold << "G)\n";
}

void GameRoom::HandleUpgradeTurret(std::shared_ptr<GameSession> session, C_UPGRADE_TURRET_PACKET& pkt)
{
	std::lock_guard<std::mutex> guard(lock);

	if (state != RoomState::PLAYING) return;

	int32_t chosenType = pkt.upgradeType;
	// 3인 경우 랜덤 업그레이드 (0: FLAME, 1: ICE, 2: STORM 중 하나)
	if (chosenType == 3)
	{
		chosenType = static_cast<int32_t>(Util::RandomRange(0, 2));
	}

	int32_t newLevel = 0;
	if (chosenType == 0)
	{
		++upgradeLevelFlame;
		newLevel = upgradeLevelFlame;
	}
	else if (chosenType == 1)
	{
		++upgradeLevelIce;
		newLevel = upgradeLevelIce;
	}
	else if (chosenType == 2)
	{
		++upgradeLevelStorm;
		newLevel = upgradeLevelStorm;
	}
	else
	{
		return;
	}

	std::cout << "[GameRoom #" << roomId << "] Player " << session->playerId
		<< " Upgraded Type " << chosenType << " to Level " << newLevel << "\n";

	S_UPGRADE_TURRET_PACKET sendPkt;
	sendPkt.playerId = session->playerId;
	sendPkt.upgradeType = chosenType;
	sendPkt.newLevel = newLevel;

	Broadcast(reinterpret_cast<BYTE*>(&sendPkt), sendPkt.size);
}

void GameRoom::HandleAgitDamage(std::shared_ptr<GameSession> session, C_AGIT_DAMAGE_PACKET& pkt)
{
	// Arrival is decided by server movement. Client reports would double damage.
}

void GameRoom::DamageAgitFromMonster()
{
	const int32_t dmg = 1;
	agitHealth -= dmg;
	if (agitHealth < 0) agitHealth = 0;

	std::cout << "[GameRoom #" << roomId << "] Agit Damaged by " << dmg
		<< " (Remaining HP: " << agitHealth << "/100)\n";

	S_AGIT_DAMAGE_PACKET sendPkt;
	sendPkt.remainingAgitHealth = agitHealth;
	Broadcast(reinterpret_cast<BYTE*>(&sendPkt), sendPkt.size);

	// 아지트 체력이 0이 되면 방 전체 게임 오버 실행
	if (agitHealth <= 0)
	{
		std::cout << "[GameRoom #" << roomId << "] Agit Destroyed! Triggering Game Over...\n";
		
		state = RoomState::WAITING;
		readyPlayerIds.clear();
		waveCount = 1;
		isWaveActive = false;
		waveTimer = 30.0f;
		spawnTimer = 0.0f;
		spawnedCount = 0;

		S_GAME_OVER_PACKET overPkt;
		Broadcast(reinterpret_cast<BYTE*>(&overPkt), overPkt.size);
		BroadcastRoomInfo();
	}
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

bool GameRoom::LoadNavigationMap()
{
	wchar_t executable[MAX_PATH] = {};
	GetModuleFileNameW(nullptr, executable, MAX_PATH);
	const auto assetPath = std::filesystem::path(executable).parent_path().parent_path() / "Assets" / "SK_Defense_Map.txt";
	std::ifstream input(assetPath);
	if (!input.is_open()) input.open("Assets/SK_Defense_Map.txt");
	if (!input.is_open()) input.open("../Assets/SK_Defense_Map.txt");
	if (!input.is_open()) return false;
	SKDefense::Grid grid;
	std::vector<SKDefense::GridPoint> spawns;
	SKDefense::GridPoint goal;
	int goalCount = 0;
	std::string line;
	while (std::getline(input, line))
	{
		if (!line.empty() && line.back() == '\r') line.pop_back();
		if (line.empty()) return false;
		std::vector<int> row;
		for (int x = 0; x < static_cast<int>(line.size()); ++x)
		{
			const SKDefense::GridPoint point{x, static_cast<int>(grid.size())};
			switch (line[x])
			{
			case '#': row.push_back(1); break;
			case '.': row.push_back(0); break;
			case 'S': spawns.push_back(point); row.push_back(0); break;
			case 'D': goal = point; ++goalCount; row.push_back(3); break;
			default: return false;
			}
		}
		grid.push_back(std::move(row));
	}
	if (!SKDefense::AStar::IsValidGrid(grid) || spawns.empty() || goalCount != 1) return false;
	const auto reachable = SKDefense::AStar::ReachableFrom(goal, grid);
	const int width = static_cast<int>(grid.front().size());
	for (const auto point : spawns)
		if (!reachable[point.y * width + point.x]) return false;
	navigationGrid = std::move(grid);
	spawnPoints = std::move(spawns);
	targetPoint = goal;
	return true;
}

void GameRoom::SetTurretTiles(SKDefense::GridPoint position, int value)
{
	for (int dy = 0; dy < 2; ++dy)
		for (int dx = 0; dx < 2; ++dx)
			navigationGrid[position.y + dy][position.x + dx] = value;
}

bool GameRoom::CanBuildTurret(int x, int y)
{
	if (navigationGrid.empty() || x < 0 || y < 0 ||
		x >= static_cast<int>(navigationGrid.front().size()) - 1 || y >= static_cast<int>(navigationGrid.size()) - 1) return false;
	for (int dy = 0; dy < 2; ++dy)
		for (int dx = 0; dx < 2; ++dx)
			if (navigationGrid[y + dy][x + dx] != 0) return false;
	// Spawn cells and occupied monster cells cannot become obstacles.
	const auto covered = [x, y](SKDefense::GridPoint point) {
		return point.x >= x && point.x <= x + 1 && point.y >= y && point.y <= y + 1;
	};
	for (const auto point : spawnPoints) if (covered(point)) return false;
	for (const auto& [id, monster] : monsters) if (covered(monster.position)) return false;
	SetTurretTiles({x, y}, 2);
	const auto reachable = SKDefense::AStar::ReachableFrom(targetPoint, navigationGrid);
	const int width = static_cast<int>(navigationGrid.front().size());
	bool valid = !reachable.empty();
	for (const auto point : spawnPoints) valid = valid && reachable[point.y * width + point.x];
	for (const auto& [id, monster] : monsters) valid = valid && reachable[monster.position.y * width + monster.position.x];
	SetTurretTiles({x, y}, 0);
	return valid;
}

void GameRoom::MergeTurrets()
{
	for (;;)
	{
		// Match the existing type/tier grouping and oldest-turret survivor rule.
		std::map<std::pair<int, int>, std::vector<size_t>> groups;
		for (size_t i = 0; i < turrets.size(); ++i)
			if (turrets[i].tier < 3) groups[{turrets[i].type, turrets[i].tier}].push_back(i);
		bool merged = false;
		for (auto& [key, indices] : groups)
		{
			if (indices.size() < 3) continue;
			std::sort(indices.begin(), indices.end(), [&](size_t a, size_t b) { return turrets[a].order < turrets[b].order; });
			ServerTurret& survivor = turrets[indices[0]];
			++survivor.tier;
			S_MERGE_TURRET_PACKET pkt;
			pkt.survivor = {static_cast<int16_t>(survivor.position.x), static_cast<int16_t>(survivor.position.y)};
			pkt.newTier = survivor.tier;
			for (size_t i = 0; i < 2; ++i)
			{
				const auto point = turrets[indices[i + 1]].position;
				SetTurretTiles(point, 0);
				pkt.removed[i] = {static_cast<int16_t>(point.x), static_cast<int16_t>(point.y)};
			}
			const size_t first = (std::max)(indices[1], indices[2]);
			const size_t second = (std::min)(indices[1], indices[2]);
			turrets.erase(turrets.begin() + first);
			turrets.erase(turrets.begin() + second);
			Broadcast(reinterpret_cast<BYTE*>(&pkt), pkt.size);
			merged = true;
			break;
		}
		if (!merged) return;
	}
}

void GameRoom::RefreshMonsterPaths()
{
	++mapRevision;
	pathCache.clear();
	for (auto& [id, monster] : monsters) AssignMonsterPath(id, monster);
}

void GameRoom::AssignMonsterPath(int32_t id, ServerMonster& monster)
{
	const int key = monster.position.y * static_cast<int>(navigationGrid.front().size()) + monster.position.x;
	auto found = pathCache.find(key);
	if (found == pathCache.end())
	{
		bool history = false;
#ifdef _DEBUG
		history = true;
#endif
		found = pathCache.emplace(key, pathfinder.FindPath(monster.position, targetPoint, navigationGrid, history)).first;
	}
	const auto& result = found->second;
	monster.path = result.path;
	monster.pathIndex = 1;
	// Keep fractional movement progress when a tower changes the route.
	S_MONSTER_PATH_PACKET pkt;
	pkt.monsterId = id;
	pkt.mapRevision = mapRevision;
	pkt.pathCount = static_cast<uint16_t>(result.path.size());
	pkt.historyCount = static_cast<uint16_t>(result.history.size());
	pkt.size = static_cast<uint16_t>(sizeof(pkt) + (result.path.size() + result.history.size()) * sizeof(PathCoordinate));
	std::vector<BYTE> buffer(pkt.size);
	std::memcpy(buffer.data(), &pkt, sizeof(pkt));
	size_t offset = sizeof(pkt);
	for (const auto* points : {&result.path, &result.history})
	{
		for (const auto point : *points)
		{
			const PathCoordinate coordinate{static_cast<int16_t>(point.x), static_cast<int16_t>(point.y)};
			std::memcpy(buffer.data() + offset, &coordinate, sizeof(coordinate));
			offset += sizeof(coordinate);
		}
	}
	Broadcast(buffer.data(), static_cast<int32_t>(buffer.size()));
}

void GameRoom::UpdateMonsters(float deltaTime)
{
	for (auto it = monsters.begin(); it != monsters.end();)
	{
		ServerMonster& monster = it->second;
		monster.moveAccumulator += deltaTime * monster.speed;
		bool moved = false;
		while (monster.moveAccumulator >= 1.0f && monster.pathIndex < monster.path.size())
		{
			monster.position = monster.path[monster.pathIndex++];
			monster.moveAccumulator -= 1.0f;
			moved = true;
		}
		const bool arrived = monster.position == targetPoint;
		if (moved || arrived)
		{
			S_MONSTER_MOVE_PACKET pkt;
			pkt.monsterId = it->first;
			pkt.mapRevision = mapRevision;
			pkt.pathIndex = static_cast<uint16_t>(monster.pathIndex);
			pkt.position = {static_cast<int16_t>(monster.position.x), static_cast<int16_t>(monster.position.y)};
			pkt.active = !arrived;
			Broadcast(reinterpret_cast<BYTE*>(&pkt), pkt.size);
		}
		if (arrived)
		{
			deadMonsters.insert(it->first);
			it = monsters.erase(it);
			DamageAgitFromMonster();
			if (state != RoomState::PLAYING) return;
		}
		else ++it;
	}
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
