#pragma once

#include <Core/Core.h>
#include <Windows.h>

namespace Craft
{
	enum class CRAFT_API Color : WORD
	{
		Red=FOREGROUND_RED,
		Green=FOREGROUND_GREEN,
		Blue=FOREGROUND_BLUE,
		Yellow = Red | Green,
		Cyan = Green | Blue,
		Purple = Red | Blue,
		Black = 0,
		DarkGray = FOREGROUND_INTENSITY,
		Gray = Red | Green | Blue,
		White = Red | Green | Blue,
		Magenta = FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY,
		BrightWhite = White | FOREGROUND_INTENSITY
	};
}