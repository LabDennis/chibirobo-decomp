#include "game/infopoint.h"

// NONMATCHING: retail packs the counters behind 0x140 bytes of the unit's other
// bss in a named storage block; this unit's bss starts with them.
// the hud's three counters, the rest of the tu's bss (InfoPointPool) is elsewhere
InfoPoint gHudMoneyCounter;
InfoPoint gHudHappyPointCounter;
InfoPoint gHudScrapCounter;
