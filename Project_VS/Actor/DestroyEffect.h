#pragma once

#include <Actor/Actor.h>
#include <Util/Timer.h>

class DestroyEffect : public Craft::Actor
{
	// 커스텀 RTTI 등록.
	TYPE_DECLARATIONS(DestroyEffect, Actor)

		// 애니메이션 이펙트 프레임 구조체.
		struct EffectFrame
	{
		EffectFrame(
			const std::string& frame,
			float playTime = 0.05f,
			Craft::Color color = Craft::Color::Red)
			: frame(frame), playTime(playTime), color(color)
		{
		}

		~EffectFrame() = default;

		// 문자열 변수 (화면에 보여줄 문자열).
		std::string frame;

		// 재생 시간.
		float playTime = 0.0f;

		// 색상.
		Craft::Color color = Craft::Color::White;
	};

public:

	// 위치를 전달받아 생성 가능하도록 생성자 구성.
	DestroyEffect(const Craft::Vector2& position);
	~DestroyEffect() = default;

private:
	// 효과 재생을 위한 Tick 이벤트 함수 오버라이드.
	virtual void Tick(float deltaTime) override;

private:

	// 시퀀스 문자열 수(배열 원소 수).
	int effectSequenceCount = 0;

	// 현재 보여지는 시퀀스 인덱스.
	int currentSequenceIndex = 0;

	// 애니메이션 재생에 사용할 타이머.
	// 시퀀스 사이에 시간 계산용.
	Timer timer;
};