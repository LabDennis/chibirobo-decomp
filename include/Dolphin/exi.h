#ifndef _DOLPHIN_EXI_H
#define _DOLPHIN_EXI_H

#include "types.h"
#include "Dolphin/os.h"
#include "Dolphin/hw_regs.h"

#ifdef __cplusplus
extern "C" {
#endif // ifdef __cplusplus

/////////// EXI DEFINES //////////
#define EXI_MAX_DEV  3
#define EXI_MAX_CHAN 3

s32 __EXIProbeStartTime[2] AT_ADDRESS(OS_BASE_CACHED | 0x30C0);

//////////////////////////////////

/////////// EXI STRUCTS //////////
// Struct for handling expansion information (size 0x40).
typedef struct EXIControl {
	EXICallback exiCallback; // _00
	EXICallback tcCallback;  // _04
	EXICallback extCallback; // _08
	vu32 state;              // _0C
	int immLen;              // _10
	u8* immBuf;              // _14
	u32 dev;                 // _18
	u32 id;                  // _1C
	s32 idTime;              // _20
	int items;               // _24
	struct {
		u32 dev;
		EXICallback callback;
	} queue[EXI_MAX_DEV]; // _28
} EXIControl;

//////////////////////////////////

////////// EXI FUNCTIONS /////////
void EXIInit(void);
void EXIProbeReset(void);

BOOL EXIImm(s32 channel, void* buffer, s32 length, u32 type, EXICallback callback);
BOOL EXIImmEx(s32 channel, void* buffer, s32 length, u32 type);
BOOL EXIDma(s32 channel, void* buffer, s32 length, u32 type, EXICallback callback);
BOOL EXISync(s32 channel);
s32 EXIClearInterrupts(s32 channel, BOOL clearExiBit, BOOL clearTcBit, BOOL clearExtBit);
EXICallback EXISetExiCallback(s32 channel, EXICallback callback);
BOOL EXIAttach(s32 channel, EXICallback callback);
BOOL EXIDetach(s32 channel);
BOOL EXISelect(s32 channel, u32 device, u32 frequency);
BOOL EXIDeselect(s32 channel);

BOOL EXILock(s32 channel, u32 device, EXICallback callback);
BOOL EXIUnlock(s32 channel);
u32 EXIGetState(s32 channel);
s32 EXIGetID(s32 channel, u32 device, u32* id);

//////////////////////////////////

#ifdef __cplusplus
};
#endif // ifdef __cplusplus

#endif
