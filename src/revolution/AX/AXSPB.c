// Based on projectPiki/pik1wii. DSP channel values are explicitly encoded as big-endian 16-bit words.
#include "revolution/ax.h"
#include "revolution/ax_studio.h"
#include "Dolphin/OS.h"

#define DEPOP_MIN (-20)
#define DEPOP_MAX 20

static AXSTUDIO __AXStudio ATTRIBUTE_ALIGN(32);

static s32 __AXSpbAux3 = 0;
static s32 __AXSpbAux2 = 0;
static s32 __AXSpbAux1 = 0;
static s32 __AXSpbAux0 = 0;

static s32 __AXSpbMain3 = 0;
static s32 __AXSpbMain2 = 0;
static s32 __AXSpbMain1 = 0;
static s32 __AXSpbMain0 = 0;

static s32 __AXSpbACS = 0;
static s32 __AXSpbACR = 0;
static s32 __AXSpbACL = 0;

static s32 __AXSpbABS = 0;
static s32 __AXSpbABR = 0;
static s32 __AXSpbABL = 0;

static s32 __AXSpbAAS = 0;
static s32 __AXSpbAAR = 0;
static s32 __AXSpbAAL = 0;

static s32 __AXSpbAS = 0;
static s32 __AXSpbAR = 0;
static s32 __AXSpbAL = 0;

AXSTUDIO* __AXGetStudio(void)
{
	return &__AXStudio;
}
static void WriteDepopChannel(s32* level, u16* valueWords, s16* delta, s32 samples)
{
	s32 average = *level / samples;
	if (average != 0) {
		if (average > DEPOP_MAX)
			average = DEPOP_MAX;
		if (average < DEPOP_MIN)
			average = DEPOP_MIN;
		valueWords[0] = (u32)*level >> 16;
		valueWords[1] = (u16)*level;
		*level -= average * samples;
		*delta = -average;
	} else {
		*level        = 0;
		valueWords[0] = 0;
		valueWords[1] = 0;
		*delta        = 0;
	}
}

void __AXDepopFadeMain(s32* level, u16* valueWords, s16* delta)
{
	WriteDepopChannel(level, valueWords, delta, AX_SAMPLES_PER_FRAME);
}

void __AXDepopFadeRmt(s32* level, u16* valueWords, s16* delta)
{
	WriteDepopChannel(level, valueWords, delta, AX_SAMPLES_PER_FRAME_RMT);
}

void __AXPrintStudio(void)
{
	__AXDepopFadeMain(&__AXSpbAL, __AXStudio.L.value, &__AXStudio.L.delta);
	__AXDepopFadeMain(&__AXSpbAR, __AXStudio.R.value, &__AXStudio.R.delta);
	__AXDepopFadeMain(&__AXSpbAS, __AXStudio.S.value, &__AXStudio.S.delta);

	__AXDepopFadeMain(&__AXSpbAAL, __AXStudio.AuxAL.value, &__AXStudio.AuxAL.delta);
	__AXDepopFadeMain(&__AXSpbAAR, __AXStudio.AuxAR.value, &__AXStudio.AuxAR.delta);
	__AXDepopFadeMain(&__AXSpbAAS, __AXStudio.AuxAS.value, &__AXStudio.AuxAS.delta);

	__AXDepopFadeMain(&__AXSpbABL, __AXStudio.AuxBL.value, &__AXStudio.AuxBL.delta);
	__AXDepopFadeMain(&__AXSpbABR, __AXStudio.AuxBR.value, &__AXStudio.AuxBR.delta);
	__AXDepopFadeMain(&__AXSpbABS, __AXStudio.AuxBS.value, &__AXStudio.AuxBS.delta);

	__AXDepopFadeMain(&__AXSpbACL, __AXStudio.AuxCL.value, &__AXStudio.AuxCL.delta);
	__AXDepopFadeMain(&__AXSpbACR, __AXStudio.AuxCR.value, &__AXStudio.AuxCR.delta);
	__AXDepopFadeMain(&__AXSpbACS, __AXStudio.AuxCS.value, &__AXStudio.AuxCS.delta);

	__AXDepopFadeRmt(&__AXSpbMain0, __AXStudio.Main0.value, &__AXStudio.Main0.delta);
	__AXDepopFadeRmt(&__AXSpbMain1, __AXStudio.Main1.value, &__AXStudio.Main1.delta);
	__AXDepopFadeRmt(&__AXSpbMain2, __AXStudio.Main2.value, &__AXStudio.Main2.delta);
	__AXDepopFadeRmt(&__AXSpbMain3, __AXStudio.Main3.value, &__AXStudio.Main3.delta);

	__AXDepopFadeRmt(&__AXSpbAux0, __AXStudio.Aux0.value, &__AXStudio.Aux0.delta);
	__AXDepopFadeRmt(&__AXSpbAux1, __AXStudio.Aux1.value, &__AXStudio.Aux1.delta);
	__AXDepopFadeRmt(&__AXSpbAux2, __AXStudio.Aux2.value, &__AXStudio.Aux2.delta);
	__AXDepopFadeRmt(&__AXSpbAux3, __AXStudio.Aux3.value, &__AXStudio.Aux3.delta);

	DCFlushRange(&__AXStudio, sizeof(AXSTUDIO));
}

void __AXSPBInit(void)
{
	__AXSpbAux3 = 0;
	__AXSpbAux2 = 0;
	__AXSpbAux1 = 0;
	__AXSpbAux0 = 0;

	__AXSpbMain3 = 0;
	__AXSpbMain2 = 0;
	__AXSpbMain1 = 0;
	__AXSpbMain0 = 0;

	__AXSpbACS = 0;
	__AXSpbACR = 0;
	__AXSpbACL = 0;

	__AXSpbABS = 0;
	__AXSpbABR = 0;
	__AXSpbABL = 0;

	__AXSpbAAS = 0;
	__AXSpbAAR = 0;
	__AXSpbAAL = 0;

	__AXSpbAS = 0;
	__AXSpbAR = 0;
	__AXSpbAL = 0;
}

void __AXSPBQuit(void)
{
}

void __AXDepopVoice(AXPB* pb)
{
	__AXSpbAL += pb->dpop.aL;
	__AXSpbAAL += pb->dpop.aAuxAL;
	__AXSpbABL += pb->dpop.aAuxBL;
	__AXSpbACL += pb->dpop.aAuxCL;

	__AXSpbAR += pb->dpop.aR;
	__AXSpbAAR += pb->dpop.aAuxAR;
	__AXSpbABR += pb->dpop.aAuxBR;
	__AXSpbACR += pb->dpop.aAuxCR;

	__AXSpbAS += pb->dpop.aS;
	__AXSpbAAS += pb->dpop.aAuxAS;
	__AXSpbABS += pb->dpop.aAuxBS;
	__AXSpbACS += pb->dpop.aAuxCS;

	__AXSpbMain0 += pb->rmtDpop.aMain0;
	__AXSpbMain1 += pb->rmtDpop.aMain1;
	__AXSpbMain2 += pb->rmtDpop.aMain2;
	__AXSpbMain3 += pb->rmtDpop.aMain3;

	__AXSpbAux0 += pb->rmtDpop.aAux0;
	__AXSpbAux1 += pb->rmtDpop.aAux1;
	__AXSpbAux2 += pb->rmtDpop.aAux2;
	__AXSpbAux3 += pb->rmtDpop.aAux3;
}
