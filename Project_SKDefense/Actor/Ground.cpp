#include "Ground.h"

using namespace Craft;
Ground::Ground(const Vector2& position)
	:super(".",position)
{
	// 우선순위 설정.
	// 바닥 액터는 다른 액터랑 겹쳤을 때 덮어쓰기 되야함.
	sortingOrder = 0;
}
