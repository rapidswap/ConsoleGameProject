#include "Player.h"
#include <Engine/Engine.h>
#include <Input/Input.h>
#include <Level/Level.h>
#include <Actor/PlayerBullet.h>
#include <Actor/DestroyEffect.h>
#include <Actor/Enemy.h>
#include <Level/GameLevel.h>
#include <Actor/DestroyEXP.h>
#include <Actor/EliteBoss.h>
#include <Actor/Demon.h>
#include <Actor/EnemyBullet.h>
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

	// 무적 타이머 초기화
	invincibilityTimer.SetTargetTime(1.0f);
	blinkTimer.SetTargetTime(0.1f);

	sortingOrder = 12;
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
	 
	if (Input::Get().GetKeyDown('R'))
	{
		bulletMode = 1 - bulletMode;
	}

	// 무적 타이머 및 깜빡임 처리
	if (isInvincible)
	{
		invincibilityTimer.Tick(deltaTime);
		blinkTimer.Tick(deltaTime);

		if (blinkTimer.IsTimeOut())
		{
			isVisible = !isVisible; // 깜빡임 토글
			blinkTimer.Reset();
		}

		if (invincibilityTimer.IsTimeOut())
		{
			isInvincible = false;
			isVisible = true; // 무적 끝나면 확실히 보이게
		}
	}

	// 발사 타이머 업데이트 및 자동 발사.
	timer.Tick(deltaTime);

	if (timer.IsTimeOut()) 
	{
		std::shared_ptr<Level> owner = GetOwner();
		if (owner)
		{
			// 데몬(최종 보스)이 있는지 가장 먼저 확인합니다.
			std::shared_ptr<Actor> targetEnemy = owner->FindActor<Demon>();
			if (!targetEnemy)
			{
				// 데몬이 없다면 엘리트 보스가 있는지 확인합니다.
				targetEnemy = owner->FindActor<EliteBoss>();
				if (!targetEnemy)
				{
					// 엘리트 보스가 없다면 일반 적을 찾습니다.
					targetEnemy = owner->FindActor<Enemy>();
				}
			}

			if (targetEnemy)
			{
				Vector2 pPos = GetPosition();
				Vector2 ePos = targetEnemy->GetPosition();
				
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

				if (bulletMode == 1)
				{
					// 일직선 발사 모드: 연사를 위해 대기 총알 수 설정
					// 첫 발은 즉시 나가도록 타이머를 0.0f로 설정합니다.
					pendingBullets = projectileCount;
					burstTimer.SetTargetTime(0.0f);
					burstTimer.Reset();
				}
				else 
				{
					// 기존 부채꼴 발사 모드 (동시 발사)
					float spreadAngle = 15.0f;
					for (int i = 0; i < projectileCount; ++i)
					{
						float offsetAngle = 0.0f;
						if (i > 0)
						{
							float sign = (i % 2 != 0) ? 1.0f : -1.0f;
							int pairIndex = (i + 1) / 2;
							offsetAngle = sign * (pairIndex * spreadAngle);
						}
						float rad = offsetAngle * 3.14f / 180.0f;
						float finalDx = baseDx * std::cos(rad) - baseDy * std::sin(rad);
						float finalDy = baseDx * std::sin(rad) + baseDy * std::cos(rad);

						owner->SpawnActor<PlayerBullet>(bulletPosition, finalDx, finalDy);
					}
				}
				
				timer.Reset();
			}
		}
	}

	// 일직선 연사 모드 처리 (매 프레임마다 검사)
	if (pendingBullets > 0)
	{	
		burstTimer.Tick(deltaTime);
		
		// 대기 시간이 지났으면 한 발 발사.
		if (burstTimer.IsTimeOut())
		{
			std::shared_ptr<Level> owner = GetOwner();
			if (owner)
			{
				// 데몬(최종 보스)이 있는지 가장 먼저 확인합니다.
				std::shared_ptr<Actor> targetEnemy = owner->FindActor<Demon>();
				if (!targetEnemy)
				{
					targetEnemy = owner->FindActor<EliteBoss>();
					if (!targetEnemy)
					{
						targetEnemy = owner->FindActor<Enemy>();
					}
				}

				if (targetEnemy)
				{
					Vector2 pPos = GetPosition();
					Vector2 ePos = targetEnemy->GetPosition();
					
					// 적을 향하는 방향 계산.
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
					
					Vector2 bulletPosition(pPos.x + (width / 2), pPos.y);
					owner->SpawnActor<PlayerBullet>(bulletPosition, baseDx, baseDy);
				}
			}
			
			pendingBullets--;
			
			// 다음 발사는 0.1초 뒤에 나가도록 타이머 재설정.
			burstTimer.SetTargetTime(0.1f);
			burstTimer.Reset();
		}
	}
}

void Player::Draw()
{
	if (!isVisible) return;
	super::Draw();
}

void Player::OnCollision(const std::shared_ptr<Actor>& other)
{
	super::OnCollision(other);

	// 무적 상태일 때는 충돌 무시 (적/보스 통과),
	if (isInvincible) return;

	// 부딪힌 액터가 적 기체(Enemy)이거나 엘리트 보스(EliteBoss)이면 처리.
	if (other->IsTypeOf<Enemy>() || other->IsTypeOf<EliteBoss>() || other->IsTypeOf<Demon>())
	{
		// 제안서의 기본 방향에 맞춰 무적 통과 적용 후 맞았을 때만 일반 적 파괴.
		// 엘리트와 보스는 체력이 따로 있으므로 즉시 파괴되지 않고 플레이어만 데미지를 입음.
		if (other->IsTypeOf<Enemy>())
		{
			other->Destroy();
			std::shared_ptr<Level> owner = GetOwner();
			if (owner)
			{
				owner->SpawnActor<DestroyEffect>(other->GetPosition());
			}
		}

		// 피격 즉시 무적 상태로 돌입.
		isInvincible = true;
		invincibilityTimer.Reset();
		blinkTimer.Reset();

		std::shared_ptr<Level> owner = GetOwner();
		if (owner)
		{
			// 레벨을 GameLevel로 변환한 뒤 체력 깎기 처리.
			std::shared_ptr<GameLevel> gameLevel = Cast<GameLevel>(owner);
			if (gameLevel)
			{
				gameLevel->TakeDamage();
			}
		}
	}

	// 부딪힌 액터가 적의 총알이면 데미지 받는 처리.
	if (other->IsTypeOf<EnemyBullet>())
	{

		other->Destroy();

		// 무적 상태일 때는 충돌 무시 (적/보스 통과),
		if (isInvincible) return;
		

		std::shared_ptr<Level> owner = GetOwner();
		std::shared_ptr<GameLevel> gameLevel = Cast<GameLevel>(owner);
		
		gameLevel->TakeDamage();

		// 피격 즉시 무적 상태로 돌입.
		isInvincible = true;
		invincibilityTimer.Reset();
		blinkTimer.Reset();
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
