#ifndef _GAME_CSTOPWATCH_H
#define _GAME_CSTOPWATCH_H

#include "types.h"
#ifdef VERSION_R24J01
// one of CBase's frame timers, class name from RTTI
class CStopwatch {
public:
	u8 mWatch[0x30]; // _00, the sdk's OSStopwatch
	s64 mElapsed;    // _30, OSCheckStopwatch before it was stopped
	// _38 vtable

	virtual ~CStopwatch() { } // _08
};
#else
#include "sdk/dolphin/os/OSStopwatch.h"

// one of CBase's frame timers. retail RTTI lists OSStopwatch as its base
class CStopwatch : public OSStopwatch {
public:
	OSTime mElapsed; // _30, OSCheckStopwatch before it was stopped
	// _38 vtable

	virtual ~CStopwatch() { } // _08
};
STATIC_ASSERT(sizeof(OSStopwatch) == 0x30);
#endif
STATIC_ASSERT(sizeof(CStopwatch) == 0x40);

#endif
