#ifndef _GAME_MENUHELPCHAR_H
#define _GAME_MENUHELPCHAR_H
#include "types.h"

struct MenuHelpChar {
	const char* mText;
	f32 mAngle0;
	f32 mAngle1;
};
STATIC_ASSERT(sizeof(MenuHelpChar) == 0xC);

#endif
