#include "types.h"
#if defined(VERSION_R24J01)
extern void OSReport(const char* message, ...);
extern u32 OSGetStackPointer(void);
#else
#include "dolphin/os.h"
#endif

typedef void (*RelCtorFunc)(void);
#if defined(VERSION_R24J01)
typedef s32 (*StageHook)(s32 count);
typedef s32 (*StageCommand)(s32 cmd, s32 count, s32* args);

extern struct CGame gGame;
extern StageHook* gStageHooks[40];
extern StageCommand* gStageCommands[40];
#else
typedef void (*RelSlotFunc)(void* arg);
typedef s32 (*RelCmdFunc)(s32 cmd, void* arg, void* arg2);

// defined by each module
extern RelSlotFunc _prolog_slots[7];
extern RelCmdFunc _prolog_cmds[];
#endif

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
#if defined(VERSION_R24J01)
s32 _prolog(s32 cmd, s32 count, s32* args)
{
	s32 stage;
	if (cmd < 0) {
		RunFuncs(_ctors);
		return 0;
	}

	// Wii links the stage handlers into the DOL. CGame::mStage is at 0x42FC.
	stage = *(const s32*)((const u8*)&gGame + 0x42FC);
	if (cmd < 7) {
		StageHook* hooks = gStageHooks[stage];
		return hooks && hooks[cmd] ? hooks[cmd](count) : 1;
	} else {
		StageCommand* commands = gStageCommands[stage];
		return commands && commands[cmd - 7] ? commands[cmd - 7](cmd, count, args) : 0;
	}
}
#else
s32 _prolog(s32 cmd, void* arg, void* arg2)
{
	s32 ret = 0;

	if (cmd < 0) {
		RunFuncs(_ctors);
	} else if (cmd < 7) {
		if (_prolog_slots[cmd] != NULL) {
			_prolog_slots[cmd](arg);
		}
	} else if (cmd >= 7) {
		ret = _prolog_cmds[cmd - 7](cmd, arg, arg2);
	}
	return ret;
}
#endif
