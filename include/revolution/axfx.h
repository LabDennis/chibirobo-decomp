#ifndef _REVOSDK_AXFX_H
#define _REVOSDK_AXFX_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif // ifdef __cplusplus

typedef struct AXFX_BUS {
	s32* left;     // _00
	s32* right;    // _04
	s32* surround; // _08

} AXFX_BUS;

typedef struct AXFX_BUFFERUPDATE {
	s32* left;     // _00
	s32* right;    // _04
	s32* surround; // _08

} AXFX_BUFFERUPDATE;

typedef struct AXFX_REVERBHI_EXP {
	f32* earlyLine[3];           // _00
	u32 earlyPos[3];             // _0C
	u32 earlyLength;             // _18
	u32 earlyMaxLength;          // _1C
	f32 earlyCoef[3];            // _20
	f32* preDelayLine[3];        // _2C
	u32 preDelayPos;             // _38
	u32 preDelayLength;          // _3C
	u32 preDelayMaxLength;       // _40
	f32* combLine[3][3];         // _44
	u32 combPos[3];              // _68
	u32 combLength[3];           // _74
	u32 combMaxLength[3];        // _80
	f32 combCoef[3];             // _8C
	f32* allpassLine[3][2];      // _98
	u32 allpassPos[2];           // _B0
	u32 allpassLength[2];        // _B8
	u32 allpassMaxLength[2];     // _C0
	f32* lastAllpassLine[3];     // _C8
	u32 lastAllpassPos[3];       // _D4
	u32 lastAllpassLength[3];    // _E0
	u32 lastAllpassMaxLength[3]; // _EC
	f32 allpassCoef;             // _F8
	f32 lastLpfOut[3];           // _FC
	f32 lpfCoef;                 // _108
	u32 active;                  // _10C
	u32 earlyMode;               // _110
	f32 preDelayTimeMax;         // _114
	f32 preDelayTime;            // _118
	u32 fusedMode;               // _11C
	f32 fusedTime;               // _120
	f32 coloration;              // _124
	f32 damping;                 // _128
	f32 crosstalk;               // _12C
	f32 earlyGain;               // _130
	f32 fusedGain;               // _134
	AXFX_BUS* busIn;             // _138
	AXFX_BUS* busOut;            // _13C
	f32 outGain;                 // _140
	f32 sendGain;                // _144
} AXFX_REVERBHI_EXP;

typedef struct AXFX_REVERBHI {
	AXFX_REVERBHI_EXP exp; // _00
	f32 coloration;        // _148
	f32 mix;               // _14C
	f32 time;              // _150
	f32 damping;           // _154
	f32 preDelay;          // _158
	f32 crosstalk;         // _15C
} AXFX_REVERBHI;

typedef void* (*AXFXAllocHook)(unsigned long size);
typedef void (*AXFXFreeHook)(void* block);

extern AXFXAllocHook __AXFXAlloc;
extern AXFXFreeHook __AXFXFree;

void AXFXSetHooks(AXFXAllocHook alloc, AXFXFreeHook free);
void AXFXGetHooks(AXFXAllocHook* alloc, AXFXFreeHook* free);

BOOL AXFXReverbHiInit(AXFX_REVERBHI* reverbHi);
BOOL AXFXReverbHiShutdown(AXFX_REVERBHI* reverbHi);
void AXFXReverbHiCallback(void* data, void* context);

u32 AXFXReverbHiExpGetMemSize(const AXFX_REVERBHI_EXP* fx);
BOOL AXFXReverbHiExpSettings(AXFX_REVERBHI_EXP* fx);
BOOL AXFXReverbHiExpSettingsUpdate(AXFX_REVERBHI_EXP* fx);
BOOL AXFXReverbHiExpInit(AXFX_REVERBHI_EXP* fx);
void AXFXReverbHiExpShutdown(AXFX_REVERBHI_EXP* fx);
void AXFXReverbHiExpCallback(AXFX_BUFFERUPDATE* update, AXFX_REVERBHI_EXP* fx);

typedef struct AXFX_BUFFERUPDATE_DPL2 {
	s32* left;           // at 0x0
	s32* right;          // at 0x4
	s32* left_surround;  // at 0x8
	s32* right_surround; // at 0xC
} AXFX_BUFFERUPDATE_DPL2;

typedef struct AXFX_REVERBHI_EXP_DPL2 {
	f32* earlyLine[4];  // at 0x0
	u32 earlyPos[3];    // at 0x10
	u32 earlyLength;    // at 0x1C
	u32 earlyMaxLength; // at 0x20
	f32 earlyCoef[3];   // at 0x24

	f32* preDelayLine[4];  // at 0x30
	u32 preDelayPos;       // at 0x40
	u32 preDelayLength;    // at 0x44
	u32 preDelayMaxLength; // at 0x48

	f32* combLine[4][3];  // at 0x4C
	u32 combPos[3];       // at 0x7C
	u32 combLength[3];    // at 0x88
	u32 combMaxLength[3]; // at 0x94
	f32 combCoef[3];      // at 0xA0

	f32* allpassLine[4][2];  // at 0xAC
	u32 allpassPos[2];       // at 0xCC
	u32 allpassLength[2];    // at 0xD4
	u32 allpassMaxLength[2]; // at 0xDC

	f32* lastAllpassLine[4];     // at 0xE4
	u32 lastAllpassPos[4];       // at 0xF4
	u32 lastAllpassLength[4];    // at 0x104
	u32 lastAllpassMaxLength[4]; // at 0x114

	f32 allpassCoef;                // at 0x124
	f32 lastLpfOut[4];              // at 0x128
	f32 lpfCoef;                    // at 0x138
	u32 active;                     // at 0x13C
	u32 earlyMode;                  // at 0x140
	f32 preDelayTimeMax;            // at 0x144
	f32 preDelayTime;               // at 0x148
	u32 fusedMode;                  // at 0x14C
	f32 fusedTime;                  // at 0x150
	f32 coloration;                 // at 0x154
	f32 damping;                    // at 0x158
	f32 crosstalk;                  // at 0x15C
	f32 earlyGain;                  // at 0x160
	f32 fusedGain;                  // at 0x164
	AXFX_BUFFERUPDATE_DPL2* busIn;  // at 0x168
	AXFX_BUFFERUPDATE_DPL2* busOut; // at 0x16C
	f32 outGain;                    // at 0x170
	f32 sendGain;                   // at 0x174
} AXFX_REVERBHI_EXP_DPL2;

u32 AXFXReverbHiExpGetMemSizeDpl2(const AXFX_REVERBHI_EXP_DPL2* fx);
BOOL AXFXReverbHiExpInitDpl2(AXFX_REVERBHI_EXP_DPL2* fx);
void AXFXReverbHiExpShutdownDpl2(AXFX_REVERBHI_EXP_DPL2* fx);
BOOL AXFXReverbHiExpSettingsDpl2(AXFX_REVERBHI_EXP_DPL2* fx);
BOOL AXFXReverbHiExpSettingsUpdateDpl2(AXFX_REVERBHI_EXP_DPL2* fx);
void AXFXReverbHiExpCallbackDpl2(AXFX_BUFFERUPDATE_DPL2* update, AXFX_REVERBHI_EXP_DPL2* fx);

#ifdef __cplusplus
}
#endif

#endif
