#pragma once

#include <Actor/Actor.h>
#include <Util/Timer.h>

class SpawnDemonEffect : public Craft::Actor
{
	// 커스텀 RTTI 등록.
	TYPE_DECLARATIONS(SpawnDemonEffect, Actor)

	// 애니메이션 이펙트 프레임 구조체.
	struct EffectFrame
	{
		EffectFrame(
			const std::string& frame,
			float playTime = 0.28f, // 7개 프레임이 총 약 2초(1.96초) 재생되도록 설정
			Craft::Color color = Craft::Color::Red)
			: frame(frame), playTime(playTime), color(color)
		{
		}

		~EffectFrame() = default;

		std::string frame;
		float playTime = 0.0f;
		Craft::Color color = Craft::Color::White;
	};

public:
	SpawnDemonEffect(const Craft::Vector2& position);
	~SpawnDemonEffect() = default;

private:
	virtual void Tick(float deltaTime) override;

private:
	int effectSequenceCount = 0;
	int currentSequenceIndex = 0;

	// 애니메이션 재생에 사용할 타이머.
	Timer timer;
};
