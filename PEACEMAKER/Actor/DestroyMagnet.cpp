#include "DestroyMagnet.h"

using namespace Craft;
DestroyMagnet::DestroyMagnet(const Craft::Vector2& position)
	:Actor("M", position, Color::Blue)
{
	lifeTimer.SetTargetTime(30.0f);
}

void DestroyMagnet::Tick(float deltaTime)
{
	super::Tick(deltaTime);

	lifeTimer.Tick(deltaTime);
	if (lifeTimer.IsTimeOut())
	{
		Destroy();
	}
}
