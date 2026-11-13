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

#define AX_SAMPLES_PER_FRAME_RMT 18
#define AX_RMT_MAX               4
#define AX_FRAME_SIZE            (AX_SAMPLES_PER_FRAME * AX_SAMPLE_DEPTH_BYTES)
#define AX_CL_SIZE               (64 * sizeof(u16))

typedef void (*AXOutCallback)(void);
typedef void (*AXExceedCallback)(u32 cycles);

typedef struct _AXPROFILE {
	s64 timeBegin;
	s64 timeProcessAuxBegin;
	s64 timeProcessAuxEnd;
	s64 timeUserFrameBegin;
	s64 timeUserFrameEnd;
	s64 timeEnd;
	s32 numVoices;
} AXPROFILE;

AXPROFILE* __AXGetCurrentProfile(void);
AXOutCallback AXRegisterCallback(AXOutCallback callback);
s32 AXRmtGetSamplesLeft(void);
s32 AXRmtGetSamples(s32 channel, s16* out, s32 count);
s32 AXRmtAdvancePtr(s32 count);

void AXInit(void);
void AXQuit(void);
BOOL AXIsInit(void);
void AXSetMode(u32 mode);
u32 AXGetMode(void);

u16 AXGetAuxAReturnVolume(void);
u16 AXGetAuxBReturnVolume(void);
u16 AXGetAuxCReturnVolume(void);
void AXSetMasterVolume(u16 volume);
void AXSetAuxAReturnVolume(u16 volume);
void AXSetAuxBReturnVolume(u16 volume);
void AXSetAuxCReturnVolume(u16 volume);

u32 __AXGetCommandListCycles(void);
void* __AXGetCommandListAddress(void);
void __AXNextFrame(void* surround, void* lr, void* rmt);

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
