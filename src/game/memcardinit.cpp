#include "game/cthread.h"

CThread gMemoryCardThread;
u8 gMemoryCardThreadStack[0x1000];

/**
 * @note Address: 0x8008A314
 * @note Size: 0x4C
 */
// __sinit, builds gMemoryCardThread and registers its dtor
