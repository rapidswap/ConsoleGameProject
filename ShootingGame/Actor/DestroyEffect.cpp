#include "DestroyEffect.h"
#include <Engine/Engine.h>
#include <Level/Level.h>

using namespace Craft;
using EffectFrame = DestroyEffect::EffectFrame;

// 효과 재생에 사용할 문자열 시퀀스 (일종의 애니메이션 프레임).
static const EffectFrame sequence[] =
{
	{"  @  ", 0.08f, Color::Red },
	{" @@  ", 0.08f, Color::Blue },
	{" @@@ ", 0.08f, Color::Green },
	{"@@@@ ", 0.08f, Color::White },
	{"@@@@@", 0.08f, Color::Red }
};

DestroyEffect::DestroyEffect(const Vector2& position)
	: Actor(sequence[0].frame, position, sequence[0].color)
{
	int effectFrameImageLength = 5;

	Vector2 tempPosition = position;

	// x 위치 고정.
	tempPosition.x = tempPosition.x < 0 ? effectFrameImageLength + tempPosition.x : tempPosition.x;
	tempPosition.x = tempPosition.x + effectFrameImageLength > Engine::Get().GetWidth() ?
		tempPosition.x - effectFrameImageLength : tempPosition.x;

	SetPosition(tempPosition);

	// 애니메이션 시퀀스 개수 구하기.
	effectSequenceCount = sizeof(sequence) / sizeof(sequence[0]);

	// 다음 애니메이션까지 대기할 시간.
	timer.SetTargetTime(sequence[0].playTime);
}

void DestroyEffect::Tick(float deltaTime)
{
	Actor::Tick(deltaTime);

	// 애니메이션 재생을 위한 타이머 업데이트.
	timer.Tick(deltaTime);
	if (!timer.IsTimeOut())
	{
		return;
	}

	// 애니메이션 재생 끝났는지 확인.
	// 끝났으면 삭제.
	if (currentSequenceIndex == effectSequenceCount - 1)
	{
		Destroy();
		return;
	}

	// 타이머 리셋.
	timer.Reset();

	// 이펙트 프레임 업데이트.
	++currentSequenceIndex;

	// 다음 시퀀스에서 재생할 시간으로 타이머 재설정.
	timer.SetTargetTime(sequence[currentSequenceIndex].playTime);

	// 애니메이션 프레임에 사용할 문자열을 액터에 복사.
	ChangeImage(sequence[currentSequenceIndex].frame);

	// 색상 설정.
	color = sequence[currentSequenceIndex].color;
};