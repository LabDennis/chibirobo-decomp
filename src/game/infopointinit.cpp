#include "game/infopoint.h"

// retail keeps the hud counters in one bss block behind this storage prefix; the hud
// code addresses the counters through it
extern "C" {
#if defined(VERSION_JP)
u8 gHudPointCounterStorage[0x134];
#else
u8 gHudPointCounterStorage[0x140];
#endif
}

// the hud's three counters, then the scanline history, one bss block in retail
InfoPoint gHudMoneyCounter;
InfoPoint gHudHappyPointCounter;
InfoPoint gHudScrapCounter;
extern "C" {
f32 gHudScanlineHistory[10];
}
