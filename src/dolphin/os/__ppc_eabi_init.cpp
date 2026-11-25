#include "types.h"
#include "Dolphin/PPCArch.h"

extern "C" {

typedef void (*StaticConstructor)(void);
CTORS extern const StaticConstructor _ctors[];

/**
 * @note Address: 0x8016B7CC
 * @note Size: 0x20
 */
void __init_user(void)
{
	for (const StaticConstructor* constructor = _ctors; *constructor; ++constructor) {
		(*constructor)();
	}
}

/**
 * @note Address: 0x8016B840
 * @note Size: 0x20
 */
void _ExitProcess(void)
{
	PPCHalt();
}

}
