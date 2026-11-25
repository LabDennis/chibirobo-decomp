#ifndef _DOLPHIN_AX_INTERNAL_H_
#define _DOLPHIN_AX_INTERNAL_H_

#include <dolphin/ax.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AX_SAMPLES_PER_FRAME 160
#define AX_DSP_BUFFER_ALIGNMENT 32

// Priority zero is the free-voice stack. Active voices use priorities 1 through 31.
#define AX_FREE_VOICE_PRIORITY 0

// AXAlloc
AXVPB* __AXGetStackHead(u32 priority);
void __AXServiceCallbackStack(void);
void __AXInitVoiceStacks(void);
void __AXAllocInit(void);
void __AXAllocQuit(void);
void __AXPushFreeStack(AXVPB* p);
AXVPB* __AXPopFreeStack(void);
void __AXPushCallbackStack(AXVPB* p);
AXVPB* __AXPopCallbackStack(void);
void __AXRemoveFromStack(AXVPB* p);
void __AXPushStackHead(AXVPB* p, u32 priority);
AXVPB* __AXPopStackFromBottom(u32 priority);

// AXAux
void __AXAuxInit(void);
void __AXAuxQuit(void);
void __AXGetAuxAInput(u32* p);
void __AXGetAuxAOutput(u32* p);
void __AXGetAuxBInput(u32* p);
void __AXGetAuxBOutput(u32* p);
void __AXProcessAux(void);
void __AXGetAuxAInputDpl2(u32* p);
void __AXGetAuxAOutputDpl2R(u32* p);
void __AXGetAuxAOutputDpl2Ls(u32* p);
void __AXGetAuxAOutputDpl2Rs(u32* p);
void __AXGetAuxBForDPL2(u32* p);
void __AXGetAuxBOutputDPL2(u32* p);

// AXCL
#define AX_COMMAND_LIST_WORDS 384
#define AX_CL_MODE_DPL2       2

extern u32 __AXClMode;

// AXComp
extern u16 __AXCompressorTable[3360];

u32 __AXGetCommandListCycles(void);
u32 __AXGetCommandListAddress(void);
void __AXWriteToCommandList(u16 data);
void __AXNextFrame(s32* surroundBuffer, s16* outputBuffer);
void __AXClInit(void);
void __AXClQuit(void);

// AXOut
void __AXOutNewFrame(u32 lessDspCycles);
void __AXOutAiCallback(void);
void __AXOutInitDSP(void);
void __AXOutInit(u32 outputBufferMode);
void __AXOutQuit(void);

// AXProf
AXPROFILE* __AXGetCurrentProfile(void);

// AXSPB
u32 __AXGetStudio(void);
void __AXPrintStudio(void);
void __AXSPBInit(void);
void __AXSPBQuit(void);
void __AXDepopVoice(AXPB* p);

// AXVPB
u32 __AXGetNumVoices(void);
void __AXServiceVPB(AXVPB *pvpb);
void __AXDumpVPB(AXVPB* pvpb);
void __AXSyncPBs(u32 lessDspCycles);
AXPB* __AXGetPBs(void);
void __AXSetPBDefault(AXVPB* p);
void __AXVPBInit(void);
void __AXVPBQuit(void);

#ifdef __cplusplus
}
#endif

#endif // _DOLPHIN_AX_INTERNAL_H_
