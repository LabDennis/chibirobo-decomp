#ifndef _REVOSDK_AX_H
#define _REVOSDK_AX_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum { AX_OUTPUT_STEREO, AX_OUTPUT_SURROUND, AX_OUTPUT_DPL2 } AXOutputMode;

#define AX_SAMPLES_PER_FRAME  96
#define AX_SAMPLE_DEPTH_BYTES sizeof(u32)

typedef enum {
	AX_STEREO_L,
	AX_STEREO_R,
	AX_STEREO_S,

	AX_STEREO_MAX
} AXStereoChannel;

typedef enum {
	AX_DPL2_L,
	AX_DPL2_R,
	AX_DPL2_LS,
	AX_DPL2_RS,

	AX_DPL2_MAX
} AXDPL2Channel;

typedef void (*AXAuxCallback)(void* chans, void* context);

void AXInit(void);
void AXQuit(void);
BOOL AXIsInit(void);
u32 AXGetMode(void);

extern u32 __AXClMode;

void __AXAllocInit(void);
void __AXAllocQuit(void);
void __AXVPBInit(void);
void __AXVPBQuit(void);
void __AXSPBInit(void);
void __AXSPBQuit(void);
void __AXClInit(void);
void __AXClQuit(void);
void __AXOutInit(u32 mode);
void __AXOutQuit(void);

void __AXAuxInit(void);
void __AXAuxQuit(void);
void __AXGetAuxAInput(void** out);
void __AXGetAuxAOutput(void** out);
void __AXGetAuxAInputDpl2(void** out);
void __AXGetAuxAOutputDpl2R(void** out);
void __AXGetAuxAOutputDpl2Ls(void** out);
void __AXGetAuxAOutputDpl2Rs(void** out);
void __AXGetAuxBInput(void** out);
void __AXGetAuxBOutput(void** out);
void __AXGetAuxBInputDpl2(void** out);
void __AXGetAuxBOutputDpl2R(void** out);
void __AXGetAuxBOutputDpl2Ls(void** out);
void __AXGetAuxBOutputDpl2Rs(void** out);
void __AXGetAuxCInput(void** out);
void __AXGetAuxCOutput(void** out);
void __AXProcessAux(void);
void AXRegisterAuxACallback(AXAuxCallback callback, void* context);
void AXRegisterAuxBCallback(AXAuxCallback callback, void* context);
void AXRegisterAuxCCallback(AXAuxCallback callback, void* context);
void AXGetAuxACallback(AXAuxCallback* callback, void** context);
void AXGetAuxBCallback(AXAuxCallback* callback, void** context);
void AXGetAuxCCallback(AXAuxCallback* callback, void** context);

#ifdef __cplusplus
}
#endif

#endif
