#include "DestroyAugmentPoint.h"

using namespace Craft;
DestroyAugmentPoint::DestroyAugmentPoint(const Craft::Vector2& position)
	:Actor("!P!",position,Color::Red)
{
	sortingOrder = 10;
}
