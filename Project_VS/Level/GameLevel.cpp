#include "GameLevel.h"
#include <Actor/Player.h>
#include <Actor/EnemySpawner.h>
#include <Render/Renderer.h>
#include <Engine/Engine.h>
#include <Input/Input.h>
#include <Util/Util.h>
#include <Util/Timer.h>
#include <string>
#include <algorithm>


using namespace Craft;
void GameLevel::OnInitialized()
{
	Level::OnInitialized();

	// 플레이어 액터 추가.
	SpawnActor<Player>();

	// 적 생성기 액터 추가.
	SpawnActor<EnemySpawner>();

	// 증강 목록에 증강 추가.
	// 플레이어 치료.
	augmentList.push_back(
		{
			"Heal",
			"Heal your Hp 1 (MAX 3)",
			[this]() {
			auto player = FindActor<Player>();
			int hp = player->GetHp();
			if (hp < 3) player->hpUp();},
		[this]() {
			auto player = FindActor<Player>();
			return player->GetHp() < 3;}
		});
	// 플레이어 최대 체력 증가.
	augmentList.push_back(
		{
			"Max Hp Up",
			"extra Hp Up",
			[this]() { 
			auto player = FindActor<Player>();
			player->hpUp();}
		});
	// 플레이어 이동 속도 증가.
	augmentList.push_back(
		{
			"Player Speed Up",
			"Speed + 1",
			[this]() {
				auto player = FindActor<Player>();
				if (player)
				{
					player->PlayerSpeedUp();
				}
			}
		});
	// 플레이어 추가 경험치.
	augmentList.push_back(
		{
			"extra EXP",
			"extra EXP +10%",
			[this]() { 
				auto player = FindActor<Player>();
				if (player)
				{
				player->PlayerExpUp();
				}
			}
		});
	// 플레이어 추가 탄환.
	augmentList.push_back(
		{
			"extra Bullet",
			"Bullet +1",
			[this]() {
				auto player = FindActor<Player>();
				if (player)
				{
				player->AddProjectile();
				}
			}
		});
	// 플레이어 공격 속도 증가.
	augmentList.push_back(
		{
			"Attack Speed Up",
			"Attack Speed Up -0.1sec",
			[this]() {
				auto player = FindActor<Player>();
				if (player)
				{
				player->AttackSpeedUp();
				}
			},
			[this]()
			{
				auto player = FindActor<Player>();
				return player && player->GetAttackSpeed() < 0.5f;
			}
		});
}

void GameLevel::ShowLevelUpMenu()
{
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
			// 메뉴 닫기.
			isLevelUpMenuOpen = false;
		}

		// Level::Tick을 호출하지 않으면 게임 내 액터들이 멈춤(Freeze).
		return;
	}

	// 증강 선택 창이 꺼져있을 때만 정상 게임 진행.
	Level::Tick(deltaTime);
}

void GameLevel::Draw()
{
	Level::Draw();

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
		Renderer::Get().Submit(statText, Vector2(10, 0), Color::Gyan, 100);
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
		// 엔진의 Quit 함수를 호출하여 게임 즉시 종료.
		Craft::Engine::Get().Quit();
		return true;
	}
	return false;
}