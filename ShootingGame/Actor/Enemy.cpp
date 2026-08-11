#include "Enemy.h"
#include <Util/Util.h>
#include <Engine/Engine.h>
#include <Actor/EnemyBullet.h>
#include <Level/Level.h>
#include <Actor/PlayerBullet.h>
#include <Actor/DestroyEffect.h>

using namespace Craft;
Enemy::Enemy(const std::string& image, int yPosition)
	: Actor(image)
{
	// 랜덤 (오른쪽 또는 왼쪽으로 이동할지 결정).
	int random = Util::RandomRange(1, 10);

	// 랜덤으로 선택된 수가 짝수/홀수 여부에 따라 방향 결정.
	if (random % 2 == 0)
	{
		// 화면 오른쪽에 생성(이동은 왼쪽 방향).
		direction = MoveDirection::Left;
		xPosition = static_cast<float>(Engine::Get().GetWidth() - width - 1);
	}
	else
	{
		// 화면 왼쪽에 생성(이동은 오른쪽 방향).
		direction = MoveDirection::Right;
		xPosition = 0.0f;
	}

	// 위치 설정.
	SetPosition(Vector2(static_cast<int>(xPosition), yPosition));

	// 발사 타이머 시간 설정 (1초에서 3초 사이의 시간을 랜덤으로).
	timer.SetTargetTime(Util::RandomRange(1.0f, 3.0f));
}

void Enemy::Tick(float deltaTime)
{
	super::Tick(deltaTime);

	// 이동.
	float dir = direction == MoveDirection::Left ? -1.0f : 1.0f;
	xPosition += dir * moveSpeed * deltaTime;

	// 좌표 검사 (왼쪽으로 벗어나는 경우).
	if (xPosition + width < 0)
	{
		Destroy();
		return;
	}

	// 오른쪽으로 벗어나는 경우.
	if (xPosition > Engine::Get().GetWidth() - 1)
	{
		Destroy();
		return;
	}

	// 위치 설정.
	SetPosition(Vector2(static_cast<int>(xPosition), GetPosition().y));

	// 발사 처리.

	// 타이머 시간 업데이트.
	timer.Tick(deltaTime);
	if (!timer.IsTimeOut())
	{
		return;
	}

	// 발사 가능하다면 타이머 리셋.
	timer.Reset();

	// 탄약 생성.
	// 생성 위치 설정.
	Vector2 bulletPosition(GetPosition().x + (width / 2), GetPosition().y);

	// 액터 생성.
	std::shared_ptr<Level> owner = GetOwner();
	if (owner)
	{
		owner->SpawnActor<EnemyBullet>(
			bulletPosition, Util::RandomRange(10.0f, 20.0f)
		);
	}
}

void Enemy::OnCollision(const std::shared_ptr<Actor>& other)
{
	super::OnCollision(other);

	// 충돌한 다른 액터가 플레이어 탄약이면 삭제.
	// 커스텀 타입 활용.
	if (other->IsTypeOf<PlayerBullet>())
	{
		//  플레이어 탄약 제거.
		other->Destroy();

		// 적 액터 제거.
		Destroy();

		// 적 파괴 이펙트 생성.
		if (GetOwner())
		{
			GetOwner()->SpawnActor<DestroyEffect>(GetPosition());
		}
	}
}
