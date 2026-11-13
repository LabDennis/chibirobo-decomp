#ifndef _REVOSDK_AX_STUDIO_H
#define _REVOSDK_AX_STUDIO_H

#include "revolution/ax_voice.h"

#ifdef __cplusplus
extern "C" {
#endif

// DSP commands address 16-bit words. Each channel stores a 32-bit sample followed by its 16-bit delta.
typedef struct AXStudioChannel {
	u16 value[2];
	s16 delta;
} AXStudioChannel;

typedef struct _AXSTUDIO {
	AXStudioChannel L;
	AXStudioChannel R;
	AXStudioChannel S;
	AXStudioChannel AuxAL;
	AXStudioChannel AuxAR;
	AXStudioChannel AuxAS;
	AXStudioChannel AuxBL;
	AXStudioChannel AuxBR;
	AXStudioChannel AuxBS;
	AXStudioChannel AuxCL;
	AXStudioChannel AuxCR;
	AXStudioChannel AuxCS;
	AXStudioChannel Main0;
	AXStudioChannel Aux0;
	AXStudioChannel Main1;
	AXStudioChannel Aux1;
	AXStudioChannel Main2;
	AXStudioChannel Aux2;
	AXStudioChannel Main3;
	AXStudioChannel Aux3;
} AXSTUDIO;

AXSTUDIO* __AXGetStudio(void);
void __AXDepopFadeMain(s32* level, u16* valueWords, s16* delta);
void __AXDepopFadeRmt(s32* level, u16* valueWords, s16* delta);
void __AXPrintStudio(void);
void __AXDepopVoice(AXPB* pb);

#ifdef __cplusplus
}
#endif
#endif
