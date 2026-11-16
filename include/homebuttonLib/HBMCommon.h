#ifndef HOME_BUTTON_LIB_COMMON_H
#define HOME_BUTTON_LIB_COMMON_H
#include <types.h>

#include "revolution/mem.h"

extern "C" MEMAllocator* spAllocator;

void* HBMAllocMem(u32 size);
void HBMFreeMem(void* pBlock);

#endif
