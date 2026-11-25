#include <types.h>

/**
 * @note Address: 0x80012A7C
 * @note Size: 0x10
 */
extern "C" void TakeNextCounterValue(int, void* context, void*, BOOL)
{
	++*static_cast<u32*>(context);
}
