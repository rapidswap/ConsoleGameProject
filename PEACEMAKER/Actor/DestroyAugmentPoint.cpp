#include "DestroyAugmentPoint.h"

using namespace Craft;
DestroyAugmentPoint::DestroyAugmentPoint(const Craft::Vector2& position)
	:Actor("!P!", position, Color::Red)
{
	lifeTimer.SetTargetTime(30.0f);
}

void DestroyAugmentPoint::Tick(float deltaTime)
{
	super::Tick(deltaTime);

	lifeTimer.Tick(deltaTime);
	if (lifeTimer.IsTimeOut())
	{
		Destroy();
	}
}
