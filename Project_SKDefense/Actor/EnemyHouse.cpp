#include "EnemyHouse.h"


EnemyHouse::EnemyHouse(const Craft::Vector2& position)
	:super("Z",position,Craft::Color::Purple)
{
	sortingOrder = 0;
}
