#include "game/xobjflags.h"

// The registration node for the first flag set follows the regional BSS prefix.
#if defined(VERSION_JP)
u8 lbl_8035C250[0x134];
#else
u8 lbl_8037DC70[0x140];
#endif

// built by the __sinit at 0x800E2520
CEvtFlags gItemEventFlags;
CEvtFlags gCoinEventFlags;
CEvtFlags gDustEventFlags;
