#include "Agit.h"

Agit::Agit(const Craft::Vector2& position)
	:super("A",position)
{
	// 몬스터(85)와 길찾기 애니메이션(80) 위에 렌더링되도록 우선순위 상향
	sortingOrder = 90;
}
