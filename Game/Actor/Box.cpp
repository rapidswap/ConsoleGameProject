#include "Box.h"

using namespace Craft;
Box::Box(const Vector2& position)
	:super("B",position,Color::Red)
{
	// 박스는 이동이 가능하기 때문에 땅과 겹칠 수 있음.
	// 땅(Gruond) 보다 높은 우선순위로 설정.
	// 박스는 타겟(목표 지점) 위에 배치될 수 있어야 함.
	// 따라서 타겟 보다도 높은 우선순위로 설정.
	sortingOrder = 5;
}
