// The Wii's text drawing unit, .text 0x802AD310-0x802AEBB0: the GameCube
// cgs.cpp pair (wii_gc_pairs.txt) plus the Wii's own helpers in between.

#include "game/wii/cgs.h"
#include <stdarg.h>
#include <stdio.h>

/**
 * @note Address: 0x802AE0E4
 * @note Size: 0xF4
 */
extern "C" void DrawGsStrokeTextFormat(CGs* gs, u32 color, const char* fmt, f32 x, f32 y, f32 z, f32 w, f32 h, ...)
{
	char buf[256];
	va_list args;
	va_start(args, h);
	vsprintf(buf, fmt, args);
	DrawGsStrokeText(gs, color, buf, x, y, z, w, h);
}
