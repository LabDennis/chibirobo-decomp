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

// Retail keeps both depop helpers out of line. The same SDK dont_inline region is in
// pik1wii/pik2wii src/ax/AXSPB.c and PrimeDecomp-sdk src/revolution/ax/AXSPB.c.
#pragma dont_inline on
void __AXDepopFadeMain(s32* all, s32* value, s16* depop)
{
	if (*all / AX_SAMPLES_PER_FRAME != 0) {
		s32 average = *all / AX_SAMPLES_PER_FRAME;
		if (average > DEPOP_MAX)
			average = DEPOP_MAX;
		if (average < DEPOP_MIN)
			average = DEPOP_MIN;
		*value = *all;
		*all -= average * AX_SAMPLES_PER_FRAME;
		*depop = -average;
	} else {
		*all   = 0;
		*value = 0;
		*depop = 0;
	}
}

void __AXDepopFadeRmt(s32* all, s32* value, s16* depop)
{
	if (*all / AX_SAMPLES_PER_FRAME_RMT != 0) {
		s32 average = *all / AX_SAMPLES_PER_FRAME_RMT;
		if (average > DEPOP_MAX)
			average = DEPOP_MAX;
		if (average < DEPOP_MIN)
			average = DEPOP_MIN;
		*value = *all;
		*all -= average * AX_SAMPLES_PER_FRAME_RMT;
		*depop = -average;
	} else {
		*all   = 0;
		*value = 0;
		*depop = 0;
	}
}
#pragma dont_inline off

void __AXPrintStudio(void)
{
	__AXDepopFadeMain(&__AXSpbAL, &__AXStudio.L, &__AXStudio.dL);
	__AXDepopFadeMain(&__AXSpbAR, &__AXStudio.R, &__AXStudio.dR);
	__AXDepopFadeMain(&__AXSpbAS, &__AXStudio.S, &__AXStudio.dS);

	__AXDepopFadeMain(&__AXSpbAAL, &__AXStudio.AuxAL, &__AXStudio.dAuxAL);
	__AXDepopFadeMain(&__AXSpbAAR, &__AXStudio.AuxAR, &__AXStudio.dAuxAR);
	__AXDepopFadeMain(&__AXSpbAAS, &__AXStudio.AuxAS, &__AXStudio.dAuxAS);

	__AXDepopFadeMain(&__AXSpbABL, &__AXStudio.AuxBL, &__AXStudio.dAuxBL);
	__AXDepopFadeMain(&__AXSpbABR, &__AXStudio.AuxBR, &__AXStudio.dAuxBR);
	__AXDepopFadeMain(&__AXSpbABS, &__AXStudio.AuxBS, &__AXStudio.dAuxBS);

	__AXDepopFadeMain(&__AXSpbACL, &__AXStudio.AuxCL, &__AXStudio.dAuxCL);
	__AXDepopFadeMain(&__AXSpbACR, &__AXStudio.AuxCR, &__AXStudio.dAuxCR);
	__AXDepopFadeMain(&__AXSpbACS, &__AXStudio.AuxCS, &__AXStudio.dAuxCS);

	__AXDepopFadeRmt(&__AXSpbMain0, &__AXStudio.Main0, &__AXStudio.dMain0);
	__AXDepopFadeRmt(&__AXSpbMain1, &__AXStudio.Main1, &__AXStudio.dMain1);
	__AXDepopFadeRmt(&__AXSpbMain2, &__AXStudio.Main2, &__AXStudio.dMain2);
	__AXDepopFadeRmt(&__AXSpbMain3, &__AXStudio.Main3, &__AXStudio.dMain3);

	__AXDepopFadeRmt(&__AXSpbAux0, &__AXStudio.Aux0, &__AXStudio.dAux0);
	__AXDepopFadeRmt(&__AXSpbAux1, &__AXStudio.Aux1, &__AXStudio.dAux1);
	__AXDepopFadeRmt(&__AXSpbAux2, &__AXStudio.Aux2, &__AXStudio.dAux2);
	__AXDepopFadeRmt(&__AXSpbAux3, &__AXStudio.Aux3, &__AXStudio.dAux3);

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
