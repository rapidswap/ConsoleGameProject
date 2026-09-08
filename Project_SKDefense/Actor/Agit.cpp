#include "Agit.h"

Agit::Agit(const Craft::Vector2& position)
	:super("A",position)
{
	// 렌더링 우선순위 (6: 아지트는 몬스터(5) 및 경로(4) 위에 선명하게 렌더링)
	sortingOrder = 6;
}
