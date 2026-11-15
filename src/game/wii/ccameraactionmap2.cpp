#include "game/ccameraaction.h"
#if defined(VERSION_R24J01)
#include "game/wii/cgame.h"
#endif
#include "game/gmath.h"
extern "C" {
#include <dolphin/os.h>
}

// the Wii build merged the area and map camera code into this file; the retail
// rodata carries "ccameraactionmap2.cpp" as the __FILE__ string for both halves.
// CamAreaSlot's constructor is the file's first emitted function.

/**
 * @note Address: 0x80154310
 * @note Size: 0x10
 */
CamAreaSlot::CamAreaSlot()
{
}
