#include "Player.h"
#include <Engine/Engine.h>
#include <Input/Input.h>
#include <Level/Level.h>
#include <Actor/PlayerBullet.h>
#include <Actor/DestroyEffect.h>
#include <Actor/Enemy.h>
#include <Level/GameLevel.h>
#include <Actor/DestroyEXP.h>
#include <Actor/DestroyEXP.h>
#include <cmath>

using namespace Craft;
Player::Player()
	:Actor("\\-_-/",Vector2::Zero,Color::Green)
{
	// 생성 위치 설정.
	int x = (Engine::Get().GetWidth() / 2) - (width / 2);
	int y = (Engine::Get().GetHeight() / 2);

	SetPosition(Vector2(x, y));

	// 위치 변수 초기화.
	xPosition = static_cast<float>(x);
	yPosition = static_cast<float>(y);

	// 연사 타이머 시간 설정.
	timer.SetTargetTime(autoFireInterval);

}


void Player::Tick(float deltaTime)
{
	super::Tick(deltaTime);

	// ESC 키 종료 처리.
	if (Input::Get().GetKeyDown(VK_ESCAPE))
	{
		QuitGame();
	}

	// 방향키 입력에 따른 이동 방향 설정.
	// 오른쪽,위 방향:1 | 왼쪽,아래 방향: -1
	float direction = 0.0f;

	// 이동 함수 호출.
	if (Input::Get().GetKey(VK_RIGHT))
	{
		direction = 1.0f;
		xMove(direction, deltaTime);
	}
	if (Input::Get().GetKey(VK_LEFT))
	{
		direction = -1.0f;
		xMove(direction, deltaTime);
	}
	if (Input::Get().GetKey(VK_UP))
	{
		direction = -1.0f;
		yMove(direction, deltaTime);
	}
	if (Input::Get().GetKey(VK_DOWN))
	{
		direction = 1.0f;
		yMove(direction, deltaTime);
	}


	// 발사 타이머 업데이트 및 자동 발사.
	timer.Tick(deltaTime);

	if (timer.IsTimeOut()) 
	{
		std::shared_ptr<Level> owner = GetOwner();
		if (owner)
		{
			auto enemy = owner->FindActor<Enemy>();
			if (enemy)
			{
				Vector2 pPos = GetPosition();
				Vector2 ePos = enemy->GetPosition();
				
				// 적을 향하는 기본 방향.
				float baseDx = static_cast<float>(ePos.x) - pPos.x;
				float baseDy = static_cast<float>(ePos.y) - pPos.y;
				
				float length = std::sqrt(baseDx * baseDx + baseDy * baseDy);
				if (length > 0.0f)
				{
					baseDx /= length;
					baseDy /= length;
				}
				else
				{
					baseDx = 0.0f;
					baseDy = -1.0f;
				}
				
				// 총알 생성 위치.
				Vector2 bulletPosition(pPos.x + (width / 2), pPos.y);

				// 총알 개수 만큼 반복해서 쏘기.
				// 총알 간의 각도는 15도.
				float spreadAngle = 15.0f;
				
				for (int i = 0;i < projectileCount;++i)
				{
					// 중앙을 기준으로 좌우로 각도 분배.
					float offsetAngle = 0.0f;

					if (i > 0)
					{
						float sign = (i % 2 != 0) ? 1.0f : -1.0f;

						int pairIndex = (i + 1) / 2;
						offsetAngle = sign * (pairIndex * spreadAngle);
					}
			
					// 각도를 라디안으로 변환.
					float rad = offsetAngle * 3.14f / 180.0f;

					// 회전 행렬을 이용해 방향 틀어주기.
					float finalDx = baseDx * std::cos(rad) - baseDy * std::sin(rad);
					float finalDy = baseDx * std::sin(rad) + baseDy * std::cos(rad);
					
					// 틀어진 방향으로 총알 생성.
					owner->SpawnActor<PlayerBullet>(bulletPosition, finalDx, finalDy);
				}
				
				timer.Reset();
			}
		}
	}
}

void Player::OnCollision(const std::shared_ptr<Actor>& other)
{
	super::OnCollision(other);

	// 부딪힌 액터가 적 기체(Enemy)이면 처리.
	if (other->IsTypeOf<Enemy>())
	{
		// 부딪힌 적 기체 제거.
		other->Destroy();

		// 파괴 이펙트 생성.
		std::shared_ptr<Level> owner = GetOwner();
		if (owner)
		{
			// 적의 위치에 파괴 이펙트 출력.
			owner->SpawnActor<DestroyEffect>(other->GetPosition());

			// 레벨을 GameLevel로 변환한 뒤 체력 깎기 처리.
			std::shared_ptr<GameLevel> gameLevel = Cast<GameLevel>(owner);
			if (gameLevel)
			{
				gameLevel->TakeDamage();
			}
		}
	}

	// 부딪힌 액터가 경험치(DestroyEXP)이면 획득 처리.
	if (other->IsTypeOf<DestroyEXP>())
	{
		// 경험치 삭제 (획득).
		other->Destroy();

		playerEXP += EXP;
		if (playerEXP >= targetEXP)
		{
			playerLevel += 1;
			playerEXP -= targetEXP;
			targetEXP *= 1.5f;

			// 레벨업 시 GameLevel에 증강 선택 메뉴(프리즈)를 띄워달라고 요청합니다.
			std::shared_ptr<Level> owner = GetOwner();
			if (owner)
			{
				std::shared_ptr<GameLevel> gameLevel = Cast<GameLevel>(owner);
				if (gameLevel)
				{
					gameLevel->ShowLevelUpMenu();
				}
			}
		}
	}
}

	
void Player::xMove(float direction, float deltaTime)
{
	// x위치 업데이트.
	// 이동 처리 -> 이동 방향과 빠르기를 적용해서 새로운 위치를 구하는 것.
	// 이동 방향(direction) / 빠르기(moveSpeed) /  시간.
	// 동속도 운동: 이동 거리 = 기존의 위치 + 이동 방향 x 빠르기 x 시간.
	xPosition += direction * moveSpeed * deltaTime;

	// 화면 왼쪽 벗어나지 않도록 처리
	if (xPosition < 0)
	{
		xPosition = 0.0f;
	}

	// 화면 오른쪽 벗어나지 않도록 처리.
	if (xPosition + width >= Engine::Get().GetWidth())
	{
		xPosition = static_cast<float>(Engine::Get().GetWidth() - width);
	}


	// 위치 업데이트.
	Vector2 newPosition = GetPosition();
	// float 값을 int로 형변환할 때 소숫점 값은 버림 처리된다는 점 주의
	newPosition.x = static_cast<int>(xPosition);
	SetPosition(newPosition);
}

void Player::yMove(float direction, float deltaTime)
{
	// x위치 업데이트.
	// 이동 처리 -> 이동 방향과 빠르기를 적용해서 새로운 위치를 구하는 것.
	// 이동 방향(direction) / 빠르기(moveSpeed) /  시간.
	// 동속도 운동: 이동 거리 = 기존의 위치 + 이동 방향 x 빠르기 x 시간.
	yPosition += direction * moveSpeed * deltaTime;


	// 화면 위쪽 벗어나지 않도록 처리
	if (yPosition < 0)
	{
		yPosition = 0.0f;
	}

	// 화면 아래쪽 벗어나지 않도록 처리.
	if (yPosition + 1 >= Engine::Get().GetHeight())
	{
		yPosition = static_cast<float>(Engine::Get().GetHeight() - 1);
	}


	// 위치 업데이트.
	Vector2 newPosition = GetPosition();
	// float 값을 int로 형변환할 때 소숫점 값은 버림 처리된다는 점 주의
	newPosition.y = static_cast<int>(yPosition);
	SetPosition(newPosition);
}

// 기존 수동 발사 함수들은 제거.
