#include "SpawnDemonEffect.h"
#include <Engine/Engine.h>
#include <Level/Level.h>

using namespace Craft;
using EffectFrame = SpawnDemonEffect::EffectFrame;

// 차원문이 열리는 듯한 포탈 연출 시퀀스.
static const EffectFrame sequence[] =
{
	{"      .      ", 0.28f, Color::Magenta },
	{"     ...     ", 0.28f, Color::Magenta },
	{"    < . >    ", 0.28f, Color::Magenta },
	{"   << O >>   ", 0.28f, Color::Red },
	{"  <<< 0 >>>  ", 0.28f, Color::Red },
	{" <<<< # >>>> ", 0.28f, Color::Blue },
	{"<<<<< # >>>>>", 0.28f, Color::White }
};

SpawnDemonEffect::SpawnDemonEffect(const Vector2& position)
	: Actor(sequence[0].frame, position, sequence[0].color)
{
	int effectFrameImageLength = 13;

	Vector2 tempPosition = position;

	// x 위치를 약간 보정 (문자열이 가운데 정렬되도록).
	tempPosition.x -= effectFrameImageLength / 2;

	// x 위치 고정 (화면을 넘어가지 않도록).
	tempPosition.x = tempPosition.x < 0 ? effectFrameImageLength + tempPosition.x : tempPosition.x;
	tempPosition.x = tempPosition.x + effectFrameImageLength > Engine::Get().GetWidth() ?
		tempPosition.x - effectFrameImageLength : tempPosition.x;

	SetPosition(tempPosition);

	// 애니메이션 시퀀스 개수 구하기.
	effectSequenceCount = sizeof(sequence) / sizeof(sequence[0]);

	// 다음 애니메이션까지 대기할 시간.
	timer.SetTargetTime(sequence[0].playTime);

	// 제일 위에 그려지도록 설정.
	sortingOrder = 100;
}

void SpawnDemonEffect::Tick(float deltaTime)
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
}
