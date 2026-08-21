#include "GameLevel.h"
#include <Actor/Player.h>
#include <Actor/EnemySpawner.h>
#include <Actor/EliteBoss.h>
#include <Actor/DestroyEffect.h>
#include <Actor/DestroyEXP.h>
#include <Actor/Demon.h>
#include <Render/Renderer.h>
#include <Engine/Engine.h>
#include <Input/Input.h>
#include <Actor/DestroyAugmentPoint.h>
#include <Actor/DestroyMagnet.h>
#include <Util/Util.h>
#include <Util/Timer.h>
#include <Actor/Enemy.h>
#include <Level/GameFailed.h>
#include <Level/GameClear.h>
#include <Level/MainMenuLevel.h>


#include <string>
#include <algorithm>

// 조건 없이 등장하는 증강.
#define ADD_AUGMENT(Name,Desc,Logic,Weight,Color)								\
	augmentList.push_back({											\
		Name, Desc,													\
			[this]() {auto player = FindActor<Player>();if (player)	\
		{															\
			Logic;}},												\
			nullptr,												\
			Weight, Color											\
	});

// 특정 조건으로 등장하는 증강.
#define ADD_COND_AUGMENT(Name, Desc, Logic, Cond, Weight, Color) \
	augmentList.push_back({ \
		Name, Desc, \
		[this]() { auto player = FindActor<Player>(); if(player) { Logic; } }, \
		[this]() { auto player = FindActor<Player>(); return player && (Cond); }, \
		Weight, Color \
	});


using namespace Craft;
void GameLevel::OnInitialized()
{
	super::OnInitialized();

	// 플레이어 액터 추가.
	SpawnActor<Player>();

	// 적 생성기 액터 추가.
	SpawnActor<EnemySpawner>();

	// 증강 목록에 증강 추가.
	
	// 1. 조건부 증강 (체력이 덜 찼을 때만 뜸).
	ADD_COND_AUGMENT(
		"Heal",
		"Heal your Hp 1",
		player->hpUp(),
		player->GetHp() < player->GetMaxHp(), 100, Color::White)

	// 2. 조건부 증강.
	ADD_COND_AUGMENT(
		"Attack Speed Up",
		"Attack Speed Up -0.2sec",
		player->AttackSpeedUp(),
		player->GetAttackSpeed() > 0.5f, 50, Color::White)

	ADD_COND_AUGMENT(
		"Bouncing Bullet",
		"Bullets bounce off walls",
		player->EnableBouncingBullet(),
		!player->HasBouncingBullet(), 5, Color::Yellow)

	ADD_COND_AUGMENT(
		"Death Nova",
		"Explode into 4 bullets",
		player->EnableDeathNova(),
		!player->HasDeathNova(), 5, Color::Yellow)

	// 3. 무조건 뜨는 일반 증강들.
	ADD_AUGMENT("Max Hp Up", "Max Hp +1 & Heal +1", player->MaxHpUp(), 100, Color::White)
	ADD_AUGMENT("Player Speed Up", "Speed + 1", player->PlayerSpeedUp(), 100, Color::White)
	ADD_AUGMENT("extra EXP", "extra EXP +25%", player->PlayerExpUp(), 100, Color::White)
	ADD_AUGMENT("extra Bullet", "Bullet +1", player->AddProjectile(), 50, Color::White)
}

 void GameLevel::ShowLevelUpMenu(int times)
{
	if (times > 1)
	{
		pendingAugmentCount += (times - 1);
	}

	// 레벨업 메뉴 오픈.
	isLevelUpMenuOpen = true;
	seletedAugmentIndex = 0;

	if (augmentList.empty()) return;

	currentChoices.clear();

	std::vector<Augment> validAugments;
	// 전체 증강 목록 중에서 조건이 없거나(!aug.canShow), 
	// 조건을 통과한(aug.canShow()) 증강들만 유효한 후보에 넣습니다.
	for (const auto& aug : augmentList)
	{
		if (!aug.canShow || aug.canShow())
		{
			validAugments.push_back(aug);
		}
	}

	// 가중치 기반 랜덤 뽑기 (최대 3개).
	int pickCount = (std::min)(3, static_cast<int>(validAugments.size()));
	
	for (int i = 0; i < pickCount; ++i)
	{
		int totalWeight = 0;
		for (const auto& aug : validAugments)
		{
			totalWeight += aug.weight;
		}

		if (totalWeight <= 0) break;

		int randomValue = Util::RandomRange(1, totalWeight);
		int currentWeight = 0;
		
		for (auto it = validAugments.begin(); it != validAugments.end(); ++it)
		{
			currentWeight += it->weight;
			if (randomValue <= currentWeight)
			{
				// 중복 선택 방지.
				currentChoices.push_back(*it);
				validAugments.erase(it); 
				break;
			}
		}
	}
}

void GameLevel::Tick(float deltaTime)
{
	auto player = FindActor<Player>();

#ifdef _DEBUG
	// 디버그 전용 치트키
	if (Input::Get().GetKeyDown(VK_F1))
	{
		auto spawner = FindActor<EnemySpawner>();
		if (spawner) spawner->ForceSpawnDemon();
	}
	if (Input::Get().GetKeyDown(VK_F2))
	{
		if (player) SpawnActor<DestroyAugmentPoint>(player->GetPosition());
	}
	if (Input::Get().GetKeyDown(VK_F3))
	{
		if (player) SpawnActor<DestroyMagnet>(player->GetPosition());
	}
#endif

	// ESC 일시정지 처리
	if (Input::Get().GetKeyDown(VK_ESCAPE))
	{
		if (isPaused)
		{
			isPaused = false; // 게임으로 돌아가기.
		}
		else if (!isLevelUpMenuOpen)
		{
			isPaused = true;
			selectedPauseMenuIndex = 0; // 초기 커서는 돌아가기.
		}
	}

	// 일시정지 상태일 때 메뉴 처리.
	if (isPaused)
	{
		if (Input::Get().GetKeyDown(VK_UP))
		{
			selectedPauseMenuIndex--;
			if (selectedPauseMenuIndex < 0) selectedPauseMenuIndex = 2;
		}
		if (Input::Get().GetKeyDown(VK_DOWN))
		{
			selectedPauseMenuIndex++;
			if (selectedPauseMenuIndex > 2) selectedPauseMenuIndex = 0;
		}
		if (Input::Get().GetKeyDown(VK_RETURN))
		{
			if (selectedPauseMenuIndex == 0)
			{
				// 게임으로 돌아가기.
				isPaused = false; 
			}
			else if (selectedPauseMenuIndex == 1)
			{
				// 재시작.
				Engine::Get().AddNewLevel<GameLevel>(); 
				return;
			}
			else if (selectedPauseMenuIndex == 2)
			{
				// 메인 메뉴로 나가기.
				Engine::Get().AddNewLevel<MainMenuLevel>(); 
				return;
			}
		}
		// 일시정지 중에는 아래 게임 로직(Tick)을 실행하지 않음.
		return; 
	}

	// 증강 선택 창이 켜져있으면(프리즈 상태).
	if (isLevelUpMenuOpen)
	{
		// 왼쪽 방향키를 눌렀을 때.
		if (Craft::Input::Get().GetKeyDown(VK_LEFT))
		{
			seletedAugmentIndex--;
			
			// Index가 0 보다 작아지면 끝으로 보냄.
			if (seletedAugmentIndex < 0)
			{
				seletedAugmentIndex = static_cast<int>(currentChoices.size()) - 1;
			}
		}

		// 오른쪽 방향키를 눌렀을 때
		if (Input::Get().GetKeyDown(VK_RIGHT))
		{
			seletedAugmentIndex++;

			// Index가 0 보다 작아지면 끝으로 보냄.
			if (seletedAugmentIndex >= currentChoices.size())
			{
				seletedAugmentIndex = 0;
			}
		}

		if (Input::Get().GetKeyDown(VK_RETURN))
		{
			if (!currentChoices.empty())
			{
				// 선택된 증강의 효과 함수.
				currentChoices[seletedAugmentIndex].onSelected();
			}
			
			// 남은 증강 선택 횟수가 있다면 메뉴를 다시 띄움
			if (pendingAugmentCount > 0)
			{
				pendingAugmentCount--;
				ShowLevelUpMenu(1);
			}
			else
			{
				// 메뉴 닫기.
				isLevelUpMenuOpen = false;
			}
		}

		// Level::Tick을 호출하지 않으면 게임 내 액터들이 멈춤(Freeze).
		return;
	}
	
	if (endlessMessageTime > 0.0f)
	{
		endlessMessageTime -= deltaTime;
	}

	playTime += deltaTime;
	// 증강 선택 창이 꺼져있을 때만 정상 게임 진행.
	super::Tick(deltaTime);

	// 카메라 갱신 (액터 이동 연산이 모두 끝난 후 세팅해야 화면 떨림 현상이 사라짐).
	auto playerAfterMove = FindActor<Player>();
	if (playerAfterMove)
	{
		SetCameraPosition(playerAfterMove->GetPosition());
	}
}

void GameLevel::Draw()
{
	super::Draw();

	// 현재 게임 플레이 타임.
	int totalSeconds = static_cast<int>(playTime);
	int minutes = totalSeconds / 60;
	int seconds = totalSeconds % 60;

	// 화면에 나올 포맷.
	char timeBuf[16];
	sprintf_s(timeBuf, "%02d:%02d", minutes, seconds);

	// 화면 상단에 표시.
	int screenWidth = Engine::Get().GetWidth();
	Renderer::Get().Submit(timeBuf, Vector2(screenWidth / 2,0), Color::White);

#ifdef _DEBUG
	// 디버그 치트키 HUD 우측 상단 표시
	std::string debugText = "[F1]Boss [F2]Augment [F3]Magnet";
	Renderer::Get().Submit(debugText, Vector2(screenWidth - (int)debugText.length() - 2, 0), Color::Yellow);
#endif

	// 현재 맵에 있는 플레이어를 찾아서 정보를 가져옴.
	auto player = FindActor<Player>();
	if (player)
	{
		// 체력 텍스트 구성 및 출력 (최상단 좌측).
		int playerHP = player->GetHp();
		std::string hpText = "HP: " + std::to_string(playerHP)
			+"/"+std::to_string(player->GetMaxHp());
		Renderer::Get().Submit(hpText, Vector2(0, 0), Color::White);
		
		// 플레이어 스탯 정보 출력 (체력 바로 아래줄).
		char statsBuf[128];
		sprintf_s(statsBuf, "ATK Spd: %.1fs | Move: %.0f | Bullets: %d", 
			player->GetAttackSpeed(), player->GetMoveSpeed(), player->GetBullets());
		std::string statsText = statsBuf;
		Renderer::Get().Submit(statsText, Vector2(0, 1), Color::White);
	
		// "Mode: " 텍스트는 하얀색 고정.
		std::string modeLabel = "Mode: ";
		Renderer::Get().Submit(modeLabel, Vector2(0, 2), Color::White);

		// 실제 모드 이름(Pistol/Shotgun)과 색상 결정.
		std::string modeStr = player->GetMode();
		Color modeColor = (modeStr == "Pistol") ? Color::Green : Color::Red;
		
		// "Mode: " 글자 길이만큼 X좌표를 띄워서 바로 옆에 컬러로 출력.
		Renderer::Get().Submit(modeStr, Vector2(static_cast<int>(modeLabel.length()), 2), modeColor);

		// 대시(텔레포트) 쿨타임 렌더링 (Mode 밑줄).
		float dashCooldown = player->GetDashCooldown();
		if (dashCooldown <= 0.0f)
		{
			// 쿨타임이 0초 이하(다 찼을 때).
			Renderer::Get().Submit("Dash: ON", Vector2(0, 3), Color::Green);
		}
		else
		{
			// 쿨타임이 남았을 때 (소수점 1자리 출력).
			char dashBuf[32];
			sprintf_s(dashBuf, "Dash: %.1fs", dashCooldown);
			Renderer::Get().Submit(dashBuf, Vector2(0, 3), Color::Red);
		}

		// 플레이어의 정보 읽어오기.
		int level = player->GetLevel();
		float exp = player->GetEXP();
		float targetExp = player->GetTargetEXP();

		// 소수점 1자리까지 포맷팅 (snprintf 사용).
		char expBuf[32];
		sprintf_s(expBuf, "%.1f", exp);
		char targetExpBuf[32];
		sprintf_s(targetExpBuf, "%.1f", targetExp);

		// 텍스트 구성 (예: Level: 1 | EXP: 0.0 / 10.0).
		std::string statText = 
			"Level: " 
			+ std::to_string(level) 
			+ " | EXP: " 
			+ std::string(expBuf) 
			+ " / " 
			+ std::string(targetExpBuf);
		
		// 화면 최상단 중앙 즈음에 출력 (예시 좌표: x=10, y=0).
		Renderer::Get().Submit(statText, Vector2(10, 0), Color::Cyan);
	}

	// 무한 루프 진입 텍스트 렌더링.
	if (endlessMessageTime > 0.0f)
	{
		int screenWidth = Engine::Get().GetWidth();
		int screenHeight = Engine::Get().GetHeight();
		
		std::string endlessText1 = "LOOP " + std::to_string(endlessLoopCount) + " COMPLETED!";
		std::string endlessText2 = "ENDLESS MODE START - ENEMY HP/SPEED MULTIPLIED";
		
		Renderer::Get().Submit(endlessText1, Vector2((screenWidth - endlessText1.length()) / 2, screenHeight / 2 - 2), Color::Red, 300);
		Renderer::Get().Submit(endlessText2, Vector2((screenWidth - endlessText2.length()) / 2, screenHeight / 2), Color::Yellow, 300);
	}

	// 증강 선택 창이 켜져있을 때(프리즈 상태) 화면 중앙에 안내 문구 출력.
	if (isLevelUpMenuOpen)
	{
		int screenWidth = Engine::Get().GetWidth();
		int screenHeight = Engine::Get().GetHeight();

		std::string title = "=== CHOOSE AN AUGMENT ===";
		Renderer::Get().Submit(
			title,
			Vector2(
				(screenWidth / 2) - (title.length() / 2),
				screenHeight / 2 - 5),
			Color::Yellow,
			200
		);

		// 증강 카드의 가로 크기와 세로 크기.
		int cardWidth = 30; // 너비를 24에서 30으로 넓힘
		int cardHeight = 9;
		// 카드 간격.
		int spacing = 34; // 간격을 28에서 34로 넓힘
		
		// 카드들이 중앙에 오도록 기준점 계산.
		int startX = (screenWidth / 2) - (spacing * (currentChoices.size() - 1)) / 2;
		int startY = screenHeight / 2 - (cardHeight / 2);

		for (size_t i = 0; i < currentChoices.size(); ++i)
		{
			Color cardColor = currentChoices[i].borderColor; 
			Color textColor = currentChoices[i].borderColor;

			// 현재 선택된 증강 카드는 초록색으로 하이라이트.
			if (i == seletedAugmentIndex)
			{
				cardColor = Color::Green;
				textColor = Color::Green;
			}

			// 1. 카드 박스 그리기.
			int cardLeftX = startX + (i * spacing) - (cardWidth / 2);
			
			// 윗면. (28칸의 - 사용).
			Renderer::Get().Submit("+----------------------------+", Vector2(cardLeftX, startY), cardColor, 200);
			// 옆면(내부 빈 공간). (28칸의 공백 사용).
			for (int j = 1; j < cardHeight - 1; ++j)
			{
				Renderer::Get().Submit("|                            |", Vector2(cardLeftX, startY + j), cardColor, 200);
			}
			// 아랫면
			Renderer::Get().Submit("+----------------------------+", Vector2(cardLeftX, startY + cardHeight - 1), cardColor, 200);


			// 2. 증강 이름 텍스트 렌더링 (카드 안쪽 상단).
			std::string nameStr = "[" + currentChoices[i].name + "]";
			Renderer::Get().Submit(
				nameStr,
				Vector2(startX + (i * spacing) - (nameStr.length() / 2), startY + 2),
				textColor,
				201 
			);

			// 3. 증강 설명 텍스트 렌더링 (카드 안쪽 하단).
			Renderer::Get().Submit(
				currentChoices[i].description,
				Vector2(startX + (i * spacing) - (currentChoices[i].description.length() / 2), startY + 5),
				textColor,
				201
			);
		}
	}

	auto demon = FindActor<Demon>();
	if (demon)
	{
		// 비율 계산.
		float hpRatio = static_cast<float>(demon->GetHp()) / demon->GetMaxHp();

		// 체력바 문자열 만들기.
		int barLength = 40;
		int filledLength = static_cast<int>(hpRatio * barLength);
		
		std::string hpBar = "BOSS HP [";
		for (int i = 0;i < barLength;++i)
		{
			if (i < filledLength) hpBar += "=";
			else hpBar += " ";
		}
		hpBar += "]" 
			+ std::to_string(demon->GetHp()) 
			+ "/" + std::to_string(demon->GetMaxHp());

		int screenWidth = Engine::Get().GetWidth();
		int screendHeight = Engine::Get().GetHeight();

		Vector2 barPos((screenWidth - hpBar.length()) / 2, screendHeight - 2);
		Renderer::Get().Submit(hpBar, barPos, Color::Red);
		
	}

	// ESC 일시정지 메뉴 렌더링
	if (isPaused)
	{
		int screenWidth = Engine::Get().GetWidth();
		int screenHeight = Engine::Get().GetHeight();

		// 일시정지 박스 크기 및 좌표
		int boxWidth = 30;
		int boxHeight = 11;
		int startX = (screenWidth - boxWidth) / 2;
		int startY = (screenHeight - boxHeight) / 2;

		// 1. 박스 그리기
		Renderer::Get().Submit("+----------------------------+", Vector2(startX, startY), Color::White, 200);
		for (int j = 1; j < boxHeight - 1; ++j)
		{
			Renderer::Get().Submit("|                            |", Vector2(startX, startY + j), Color::White, 200);
		}
		Renderer::Get().Submit("+----------------------------+", Vector2(startX, startY + boxHeight - 1), Color::White, 200);

		// 2. 제목 그리기
		std::string title = "PAUSE";
		Renderer::Get().Submit(title, Vector2(startX + (boxWidth - title.length()) / 2, startY + 2), Color::Yellow, 201);

		// 3. 메뉴 항목 그리기
		std::string menus[3] = { "Return to Game", "Restart Game", "Main Menu" };
		for (int i = 0; i < 3; ++i)
		{
			Color textColor = (i == selectedPauseMenuIndex) ? Color::Green : Color::White;
			std::string menuStr = (i == selectedPauseMenuIndex) ? "-[ " + menus[i] + " ]-" : menus[i];
			Renderer::Get().Submit(menuStr, Vector2(startX + (boxWidth - menuStr.length()) / 2, startY + 5 + i * 2), textColor, 201);
		}
	}
}

void GameLevel::TakeDamage()
{
	auto player = FindActor<Player>();
	if (player)
	{
		player->hpDown();
		CheckGameFailed();
	}
}

bool GameLevel::CheckGameFailed()
{
	auto player = FindActor<Player>();
	if (player && player->GetHp() <= 0)
	{
		Engine::Get().AddNewLevel<GameFailed>(playTime);
		return true;
	}

	return false;
}

void GameLevel::OnBossDefeated()
{
	auto spawner = FindActor<EnemySpawner>();
	if (spawner)
	{
		spawner->NextLoop();
		endlessLoopCount++;
		endlessMessageTime = 5.0f; 
	}
}

void GameLevel::WipeOutEnemies()
{
	for (auto& actor : actorList)
	{
		if (actor->IsTypeOf<Enemy>() || actor->IsTypeOf<EliteBoss>())
		{
			actor->Destroy();
			
			// 파괴 이펙트와 경험치 스폰.
			SpawnActor<DestroyEffect>(actor->GetPosition());
			SpawnActor<DestroyEXP>(actor->GetPosition());
		}
	}
}