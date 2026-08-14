#include "Timer.h"

Timer::Timer(float targetTime)
	:elapsedTime(0.0f),targetTime(targetTime)
{
}

void Timer::Tick(float deltaTime)
{
	// 경과 시간 누적 처리.
	elapsedTime += deltaTime;

	// 경과 시간이 목표 시간을 벗어나지 않도록 고정 처리(옵션).
	elapsedTime = elapsedTime >= targetTime ? targetTime : elapsedTime;

}

void Timer::Reset()
{
	// 경과 시간 초기화.
	elapsedTime = 0.0f;
}

void Timer::SetTargetTime(float targetTime)
{
	this->targetTime = targetTime;
}
