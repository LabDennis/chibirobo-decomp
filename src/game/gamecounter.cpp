#include <types.h>

/**
 * @note Address: 0x80012A7C
 * @note Size: 0x10
 */
extern "C" u32 TakeNextCounterValue(void* unused, u32* counter)
{
	return (*counter)++;
}
