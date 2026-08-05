#include "ScreenBuffer.h"
#include <cassert>
#include <iostream>

namespace Craft
{
	ScreenBuffer::ScreenBuffer(const Vector2& screenSize)
		:size(screenSize)
	{
		// 콘솔 버퍼 생성.
		buffer = CreateConsoleScreenBuffer()
	}

	ScreenBuffer::~ScreenBuffer()
	{
	}
	void ScreenBuffer::Clear() const
	{
	}
	void ScreenBuffer::Draw(const CHAR_INFO* const charInfo) const
	{
	}
}