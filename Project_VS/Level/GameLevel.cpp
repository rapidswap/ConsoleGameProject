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
#include <Util/Util.h>
#include <Util/Timer.h>
#include <Actor/Enemy.h>
#include <Level/GameFailed.h>
#include <Level/GameClear.h>


#include <string>
#include <algorithm>

// 조건 없이 등장하는 증강.
#define ADD_AUGMENT(Name,Desc,Logic)								\
	augmentList.push_back({											\
		Name, Desc,													\
			[this]() {auto player = FindActor<Player>();if (player)	\
		{															\
			Logic;}},												\
			nullptr													\
	});

// 특정 조건으로 등장하는 증강.
#define ADD_COND_AUGMENT(Name, Desc, Logic, Cond) \
	augmentList.push_back({ \
		Name, Desc, \
		[this]() { auto player = FindActor<Player>(); if(player) { Logic; } }, \
		[this]() { auto player = FindActor<Player>(); return player && (Cond); } \
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
		player->GetHp() < player->GetMaxHp())

	// 2. 조건부 증강 (공격 속도 제한).
	ADD_COND_AUGMENT(
		"Attack Speed Up",
		"Attack Speed Up -0.1sec",
		player->AttackSpeedUp(),
		player->GetAttackSpeed() > 0.5f)

	// 3. 무조건 뜨는 일반 증강들
	ADD_AUGMENT("Max Hp Up", "Max Hp +1 & Heal +1", player->MaxHpUp())
	ADD_AUGMENT("Player Speed Up", "Speed + 1", player->PlayerSpeedUp())
	ADD_AUGMENT("extra EXP", "extra EXP +10%", player->PlayerExpUp())
	ADD_AUGMENT("extra Bullet", "Bullet +1", player->AddProjectile())
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

	// 전체 증강 목록 중에서 조건이 없거나(!aug.canShow), 
	// 조건을 통과한(aug.canShow()) 증강들만 이번 선택지 후보에 넣습니다.
	for (const auto& aug : augmentList)
	{
		if (!aug.canShow || aug.canShow())
		{
			currentChoices.push_back(aug);
		}
	}

	// 증강 목록을 섞기.
	std::shuffle(
		currentChoices.begin(),
		currentChoices.end(),
		Util::GetRandomEngine());

	// 섞인 목록 중에서 앞의 3개만 보여주기.
	if (currentChoices.size() > 3)
	{
		currentChoices.resize(3);
	}
}

void GameLevel::Tick(float deltaTime)
{
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
	playTime += deltaTime;
	// 증강 선택 창이 꺼져있을 때만 정상 게임 진행.
	super::Tick(deltaTime);
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
	Renderer::Get().Submit(timeBuf, Vector2(screenWidth / 2,0), Color::White,10);

	// 현재 맵에 있는 플레이어를 찾아서 정보를 가져옴.
	auto player = FindActor<Player>();
	if (player)
	{
		// 체력 텍스트 구성 및 출력 (최상단 좌측).
		int playerHP = player->GetHp();
		std::string hpText = "HP: " + std::to_string(playerHP);
		Renderer::Get().Submit(hpText, Vector2(0, 0), Color::White, 100);
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
		Renderer::Get().Submit(statText, Vector2(10, 0), Color::Cyan, 0);
	}

	// 증강 선택 창이 켜져있을 때(프리즈 상태) 화면 중앙에 안내 문구 출력.
	if (isLevelUpMenuOpen)
	{
		int screenWidth = Engine::Get().GetWidth();
		int screenHeight = Engine::Get().GetHeight();

		std::string title = "=== LEVEL UP! CHOOSE AN AUGMENT ===";
		Renderer::Get().Submit(
			title,
			Vector2(
				(screenWidth / 2) - (title.length() / 2),
				screenHeight / 2 - 5),
			Color::Yellow,
			200
		);

		// 증강의 좌우 간격
		int spacing = 20;
		
		// 증강들이 중앙에 오도록 좌표 계산.
		int startX = 
			(screenWidth / 2) 
			- (spacing * (currentChoices.size() - 1)) / 2;

		for (size_t i = 0;i < currentChoices.size();++i)
		{
			Color textColor = Color::White;

			// 현재 선택된 증강은 초록색.
			if (i == seletedAugmentIndex)
			{
				textColor = Color::Green;
			}

			// 증강 출력.
			Renderer::Get().Submit(
				currentChoices[i].name,
				Vector2(startX + (i * spacing)
					- (currentChoices[i].name.length() / 2), screenHeight / 2),
				textColor,
				200
			);

			// 증강 설명 출력.
			Renderer::Get().Submit(
				currentChoices[i].description,
				Vector2(startX + (i * spacing)
					- (currentChoices[i].description.length() / 2),
					screenHeight / 2 + 2),
				Color::White,
				200
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

void GameLevel::TakeDemonDamage()
{
	auto demon = FindActor<Demon>();
	if (demon)
	{
		demon->hpDown();
		CheckGameClear();
	}
}

bool GameLevel::CheckGameFailed()
{
	// 게임 실패 시.
	// 따로 메뉴 창
	// 다시 시작, 메인 메뉴로 돌아가기, 게임 종료
	auto player = FindActor<Player>();
	if (player && player->GetHp() <= 0)
	{
		Engine::Get().AddNewLevel<GameFailed>();
		return true;
	}

	return false;
}

bool GameLevel::CheckGameClear()
{
	// 게임 클리어시.
	// 화면 게임 클리어 엔터 -> 메인 메뉴
	auto demon = FindActor<Demon>();
	if (demon->GetHp() <= 20)
	{
		demon->DemonHurt();
	}
	if (demon && demon->GetHp() <= 0)
	{
		Engine::Get().AddNewLevel<GameClear>(playTime);
		return true;
	}
	return false;
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