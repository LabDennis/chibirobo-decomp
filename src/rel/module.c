#include "game/relmodule.h"
#include "dolphin/os.h"

typedef void (*RelCtorFunc)(void);

extern RelCtorFunc _ctors[];
extern RelCtorFunc _dtors[];

static inline void RunFuncs(RelCtorFunc* funcs)
{
	while (*funcs != NULL) {
		(*funcs)();
		funcs++;
	}
}

/**
 * @note Address: stage08 .text 0x8
 * @note Size: 0xB8
 */
void _unresolved(void)
{
	u32 i;
	u32* p;

	OSReport("\nError: A called an unlinked function.\n");
	OSReport("Address:      Back Chain    LR Save\n");
	for (i = 0, p = (u32*)OSGetStackPointer(); p != NULL && (u32)p != 0xFFFFFFFF && i++ < 16; p = (u32*)*p) {
		OSReport("0x%08x:   0x%08x    0x%08x\n", p, p[0], p[1]);
	}
	OSReport("\n");
}

/**
 * @note Address: stage08 .text 0xC0
 * @note Size: 0x48
 */
void _epilog(void)
{
	RunFuncs(_dtors);
}

/**
 * @note Address: stage08 .text 0x108
 * @note Size: 0xB8
 */
s32 _prolog(s32 cmd, s32 count, s32* args)
{
	s32 ret = 0;

	if (cmd < 0) {
		RunFuncs(_ctors);
	} else if (cmd < 7) {
		if (_prolog_slots[cmd] != NULL) {
			_prolog_slots[cmd](count);
		}
	} else if (cmd >= 7) {
		ret = _prolog_cmds[cmd - 7](cmd, count, args);
	}
	return ret;
}
