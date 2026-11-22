#include "Dolphin/mtx.h"
#include "game/cthread.h"
#include "game/memcard.h"

CThread gMemoryCardThread;
u8 gMemoryCardThreadStack[0x1000];

// a slot's file name while reading previews
char gSaveReadFileName[0xC8];
// CARDRead's DMA target, so the buffer itself is 32-byte aligned
SaveFile gSavePreviewFileBuffer[2] ATTRIBUTE_ALIGN(32);
// a slot's file name while scanning
char gSaveScanFileName[0x80];

/**
 * @note Address: 0x8008A314
 * @note Size: 0x4C
 */
// __sinit, builds gMemoryCardThread and registers its dtor
