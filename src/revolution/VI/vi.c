#include "revolution/hw_regs.h"
#include "revolution/os/OSReset.h"

struct _GXRenderModeObj;

#include "Dolphin/os.h"
#include "Dolphin/gx.h"
#include "Dolphin/si.h"
#include "Dolphin/vi.h"
#include "revolution/vi/vi3in1.h"

const char* __VIVersion = "<< RVL_SDK - VI \trelease build: Sep 11 2008 17:33:38 (0x4199_60831) >>";

// TV formats and modes the GameCube header does not know. Format 8 has no name in the binary.
#define VI_TV_EXTRA            8
#define VI_TVMODE_PAL_PROG     6
#define VI_TVMODE_MPAL_PROG    10
#define VI_TVMODE_EURGB60_PROG 22

#define VI_NUM_IDLE_DEVICES 10

typedef VITimingInfo VITiming;

// other libraries
s8 SCGetDisplayOffsetH(void);
u8 SCGetScreenSaverMode(void);
BOOL __OSSetVIForceDimming(BOOL dimming, u32 yShift, u32 xShift);
BOOL __DVDStopMotorAsync(DVDCommandBlock* block, DVDCBCallback callback);
void __DVDRestartMotor(void);
extern DVDCommandBlock __DVDStopMotorCommandBlock;

static BOOL VIOnShutdown(BOOL final, u32 event);
static u32 getCurrentFieldEvenOdd(void);
void __VIGetCurrentPosition(s16* x, s16* y);
void __VIDisplayPositionToXY(u32 hct, u32 vct, s16* x, s16* y);
u32 VIGetScanMode(void);

// The retrace/dimming state is written inside interrupt handlers, so the SDK
// declares it volatile; wii-ipl's RVL_SDK vi.c keeps the same declarations.
// wii-ipl: https://github.com/koopthekoopa/wii-ipl/blob/main/libs/RVL_SDK/src/vi/vi.c
// .sbss holds the variables with an initializer in this order, then the others backwards
static BOOL IsInitialized                  = FALSE;
static volatile u32 badRetraceLineCount    = 0;
static volatile BOOL checkRetraceLine      = FALSE;
static volatile BOOL needInitDimming       = FALSE;
static volatile u32 dimmingThreshold       = 0;
static volatile u32 timeToDimming          = 0;
static volatile u32 timeToDVDStop          = 0;
static volatile u32 dimmingCount           = 0;
static volatile u32 dvdStopCount           = 0;
static volatile BOOL isDimmed              = FALSE;
static VIPositionCallback PositionCallback = NULL;
static s16 displayOffsetH                  = 0;
static s16 displayOffsetV                  = 0;
static volatile u32 changeMode             = 0;
static volatile u64 changed                = 0;
static volatile u32 shdwChangeMode         = 0;
static volatile u64 shdwChanged            = 0;
static u32 FBSet                           = 0;
static VITiming* timingExtra               = NULL;

static volatile u32 retraceCount;
static volatile u32 flushFlag;
static volatile u32 flush3in1Flag;
static volatile BOOL dimmingEnabled;
static volatile BOOL dvdStopEnabled;
static volatile u32 dimmingMode;
static volatile BOOL rfIdle;
static volatile BOOL siIdle;
static OSThreadQueue retraceQueue;
static VIRetraceCallback PreCB;
static VIRetraceCallback PostCB;
static u32 encoderType;
static VITiming* CurrTiming;
static u32 CurrTvMode;
static u32 NextBufAddr;
static u32 CurrBufAddr;

static u16 shdwRegs[59];
static u16 regs[59];
static VIPositionInfo HorVer;
static volatile BOOL devIdle[VI_NUM_IDLE_DEVICES];

#define MARK_CHANGED(index) (changed |= 1LL << (63 - (index)))

static VITiming timing[11] = {
	{ 6, 240, 24, 25, 3, 2, 12, 13, 12, 13, 520, 519, 520, 519, 525, 429, 64, 71, 105, 162, 373, 122, 412 },
	{ 6, 240, 24, 24, 4, 4, 12, 12, 12, 12, 520, 520, 520, 520, 526, 429, 64, 71, 105, 162, 373, 122, 412 },
	{ 5, 287, 35, 36, 1, 0, 13, 12, 11, 10, 619, 618, 617, 620, 625, 432, 64, 75, 106, 172, 380, 133, 420 },
	{ 5, 287, 33, 33, 2, 2, 13, 11, 13, 11, 619, 621, 619, 621, 624, 432, 64, 75, 106, 172, 380, 133, 420 },
	{ 6, 240, 24, 25, 3, 2, 16, 15, 14, 13, 518, 517, 516, 519, 525, 429, 64, 78, 112, 162, 373, 122, 412 },
	{ 6, 240, 24, 24, 4, 4, 16, 14, 16, 14, 518, 520, 518, 520, 526, 429, 64, 78, 112, 162, 373, 122, 412 },
	{ 12, 480, 48, 48, 6, 6, 24, 24, 24, 24, 1038, 1038, 1038, 1038, 1050, 429, 64, 71, 105, 162, 373, 122, 412 },
	{ 12, 480, 44, 44, 10, 10, 24, 24, 24, 24, 1038, 1038, 1038, 1038, 1050, 429, 64, 71, 105, 168, 379, 122, 412 },
	{ 6, 241, 24, 25, 1, 0, 12, 13, 12, 13, 520, 519, 520, 519, 525, 429, 64, 71, 105, 159, 370, 122, 412 },
	{ 12, 480, 48, 48, 6, 6, 24, 24, 24, 24, 1038, 1038, 1038, 1038, 1050, 429, 64, 71, 105, 180, 391, 122, 412 },
	{ 10, 576, 62, 62, 6, 6, 20, 20, 20, 20, 1240, 1240, 1240, 1240, 1250, 432, 64, 75, 106, 172, 380, 122, 412 },
};

static u16 taps[25] = {
	0x01F0, 0x01DC, 0x01AE, 0x0174, 0x0129, 0x00DB, 0x008E, 0x0046, 0x000C, 0x00E2, 0x00CB, 0x00C0, 0x00C4,
	0x00CF, 0x00DE, 0x00EC, 0x00FC, 0x0008, 0x000F, 0x0013, 0x0013, 0x000F, 0x000C, 0x0008, 0x0001,
};

// PAL progressive render modes: in the dol, but nothing in it refers to them.
GXRenderModeObj GXPal528Prog = {
	(VITVMode)VI_TVMODE_PAL_PROG,
	640,
	528,
	528,
	40,
	23,
	640,
	528,
	VI_XFBMODE_SF,
	FALSE,
	FALSE,
	{ { 6, 6 }, { 6, 6 }, { 6, 6 }, { 6, 6 }, { 6, 6 }, { 6, 6 }, { 6, 6 }, { 6, 6 }, { 6, 6 }, { 6, 6 }, { 6, 6 }, { 6, 6 } },
	{ 0, 0, 21, 22, 21, 0, 0 },
};

GXRenderModeObj GXPal528ProgSoft = {
	(VITVMode)VI_TVMODE_PAL_PROG,
	640,
	528,
	528,
	40,
	23,
	640,
	528,
	VI_XFBMODE_SF,
	FALSE,
	FALSE,
	{ { 6, 6 }, { 6, 6 }, { 6, 6 }, { 6, 6 }, { 6, 6 }, { 6, 6 }, { 6, 6 }, { 6, 6 }, { 6, 6 }, { 6, 6 }, { 6, 6 }, { 6, 6 } },
	{ 8, 8, 10, 12, 10, 8, 8 },
};

GXRenderModeObj GXPal524ProgAa = {
	(VITVMode)VI_TVMODE_PAL_PROG,
	640,
	264,
	524,
	40,
	23,
	640,
	524,
	VI_XFBMODE_SF,
	FALSE,
	TRUE,
	{ { 3, 2 }, { 9, 6 }, { 3, 10 }, { 3, 2 }, { 9, 6 }, { 3, 10 }, { 9, 2 }, { 3, 6 }, { 9, 10 }, { 9, 2 }, { 3, 6 }, { 9, 10 } },
	{ 4, 8, 12, 16, 12, 8, 4 },
};

static OSShutdownFunctionInfo ShutdownFunctionInfo = { VIOnShutdown, 127 };

static s32 cntlzd(u64 bit)
{
	u32 hi;
	u32 lo;
	s32 value;

	hi    = bit >> 32;
	lo    = bit & 0xFFFFFFFF;
	value = __cntlzw(hi);
	if (value < 32) {
		return value;
	}
	return __cntlzw(lo) + 32;
}

static BOOL VIOnShutdown(BOOL final, u32 event)
{
	static BOOL first = TRUE;
	static u32 count;
	BOOL retval;

	if (!final) {
		switch (event) {
		case 1:
		case 2:
		case 3:
			if (first) {
				__VIRequestRGBMode();
				VIFlush();
				count  = retraceCount;
				first  = FALSE;
				retval = FALSE;
			} else if (count == retraceCount) {
				retval = FALSE;
			} else {
				retval = TRUE;
			}
			break;
		case 0:
		case 4:
		case 5:
		case 6:
			__VISetGamma1_0();
			retval = TRUE;
			break;
		}
	} else {
		retval = TRUE;
	}

	return retval;
}

static BOOL VISetRegs(void)
{
	s32 regIndex;

	if (shdwChangeMode != 1 || getCurrentFieldEvenOdd() != 0) {
		while (shdwChanged != 0) {
			regIndex           = cntlzd(shdwChanged);
			__VIRegs[regIndex] = shdwRegs[regIndex];
			shdwChanged &= ~((u64)1 << (63 - regIndex));
		}

		shdwChangeMode = 0;
		CurrTiming     = HorVer.timing;
		CurrTvMode     = HorVer.tv;
		CurrBufAddr    = NextBufAddr;
		return TRUE;
	}

	return FALSE;
}

static void GetCurrentDisplayPosition(u32* hct, u32* vct)
{
	u32 hcount, vcount0, vcount;
	vcount = __VIRegs[VI_VERT_COUNT] & 0x7FF;

	do {
		vcount0 = vcount;
		hcount  = __VIRegs[VI_HORIZ_COUNT] & 0x7FF;
		vcount  = __VIRegs[VI_VERT_COUNT] & 0x7FF;
	} while (vcount0 != vcount);

	*hct = hcount;
	*vct = vcount;
}

static void __VIRetraceHandler(__OSInterrupt interrupt, OSContext* context)
{
	static u32 oldDtvStatus       = 999;
	static u32 oldTvFormat        = 999;
	static BOOL oldDimmingEnabled = TRUE;
	static BOOL oldDvdStopEnabled = TRUE;
	static volatile BOOL requestDim;
	static volatile BOOL requestUndim;
	OSContext exceptionContext;
	u16 reg;
	u32 inter;
	u32 dtvStatus;
	u32 tvFormat;
	s32 bit;
	s32 i;

	inter = 0;
	reg   = __VIRegs[VI_DISP_INT_0];
	if (reg & 0x8000) {
		__VIRegs[VI_DISP_INT_0] = reg & ~0x8000;
		inter |= 1;
	}
	reg = __VIRegs[VI_DISP_INT_1];
	if (reg & 0x8000) {
		__VIRegs[VI_DISP_INT_1] = reg & ~0x8000;
		inter |= 2;
	}
	reg = __VIRegs[VI_DISP_INT_2];
	if (reg & 0x8000) {
		__VIRegs[VI_DISP_INT_2] = reg & ~0x8000;
		inter |= 4;
	}
	reg = __VIRegs[VI_DISP_INT_3];
	if (reg & 0x8000) {
		__VIRegs[VI_DISP_INT_3] = reg & ~0x8000;
		inter |= 8;
	}
	reg = __VIRegs[VI_DISP_INT_3];

	if ((inter & 4) || (inter & 8)) {
		OSClearContext(&exceptionContext);
		OSSetCurrentContext(&exceptionContext);

		if (PositionCallback != 0) {
			s16 x, y;
			__VIGetCurrentPosition(&x, &y);
			(*PositionCallback)(x, y);
		}

		OSClearContext(&exceptionContext);
		OSSetCurrentContext(context);
		return;
	}

	retraceCount++;
	OSClearContext(&exceptionContext);
	OSSetCurrentContext(&exceptionContext);

	if (PreCB) {
		PreCB(retraceCount);
	}

	if (checkRetraceLine) {
		u32 hcount, vcount;
		GetCurrentDisplayPosition(&hcount, &vcount);
		if (vcount != 1 && vcount != (u32)(CurrTiming->numHalfLines / 2) + 1) {
			badRetraceLineCount++;
		}
	}

	if (flushFlag != 0) {
		if (VISetRegs()) {
			flushFlag = 0;
			SIRefreshSamplingRate();
		}
	}

	dtvStatus = VIGetDTVStatus();
	if (dtvStatus != oldDtvStatus) {
		__VISetYUVSEL(dtvStatus);
	}
	oldDtvStatus = dtvStatus;

	tvFormat = VIGetTvFormat();
	if (tvFormat != oldTvFormat) {
		if (tvFormat == VI_EURGB60) {
			__VISetFilter4EURGB60(TRUE);
		} else {
			__VISetFilter4EURGB60(FALSE);
		}

		switch (tvFormat) {
		case VI_PAL:
			switch (dimmingMode) {
			case 1:
				timeToDimming = 30000;
				break;
			case 2:
				timeToDimming = 45000;
				break;
			default:
				timeToDimming = 15000;
				break;
			}
			timeToDVDStop = 90000;
			break;
		default:
			switch (dimmingMode) {
			case 1:
				timeToDimming = 36000;
				break;
			case 2:
				timeToDimming = 54000;
				break;
			default:
				timeToDimming = 18000;
				break;
			}
			timeToDVDStop = 108000;
			break;
		}

		dimmingCount = 0;
		dvdStopCount = 0;
	}
	oldTvFormat = tvFormat;

	if (flush3in1Flag) {
		while (__VI3in1Changed) {
			bit = 1 << (31 - __cntlzw(__VI3in1Changed));
			switch (bit) {
			case VI_3IN1_CGMS:
				__VISetCGMS();
				break;
			case VI_3IN1_WSS:
				__VISetWSS();
				break;
			case VI_3IN1_CC:
				__VISetClosedCaption();
				break;
			case VI_3IN1_MACROVISION:
				__VISetMacrovision();
				break;
			case VI_3IN1_GAMMA:
				__VISetGamma();
				break;
			case VI_3IN1_TRAP_FILTER:
				__VISetTrapFilter();
				break;
			case VI_3IN1_OVERDRIVE:
				__VISetRGBOverDrive();
				break;
			case VI_3IN1_RGB:
				__VISetRGBModeImm();
				break;
			}
			__VI3in1Changed &= ~bit;
		}
		flush3in1Flag = 0;
	}

	if (PostCB) {
		OSClearContext(&exceptionContext);
		PostCB(retraceCount);
	}

	OSWakeupThread(&retraceQueue);
	OSClearContext(&exceptionContext);
	OSSetCurrentContext(context);

	if (needInitDimming == TRUE) {
		if (__OSSetVIForceDimming(FALSE, 0, 0) == TRUE) {
			needInitDimming = FALSE;
			dimmingCount    = 0;
		}
	}

	for (i = 0; i < VI_NUM_IDLE_DEVICES; i++) {
		if (!devIdle[i]) {
			devIdle[0] = FALSE;
			break;
		}
	}

	if (rfIdle && siIdle && devIdle[0]) {
		if (dimmingEnabled == TRUE && dimmingCount < 0xFFFFFFFF) {
			dimmingCount++;
		}
		if (dvdStopEnabled == TRUE && dvdStopCount < 0xFFFFFFFF) {
			dvdStopCount++;
		}
	} else {
		if (dimmingCount >= dimmingThreshold) {
			requestUndim = TRUE;
		}
		if (dvdStopCount >= timeToDVDStop) {
			__DVDRestartMotor();
		}
		dimmingCount     = 0;
		dvdStopCount     = 0;
		dimmingThreshold = timeToDimming;
	}

	if (oldDimmingEnabled != dimmingEnabled) {
		if (!dimmingEnabled && dimmingCount >= dimmingThreshold) {
			requestUndim = TRUE;
		}
		dimmingCount     = 0;
		dimmingThreshold = timeToDimming;
	}

	if (dimmingCount == dimmingThreshold) {
		requestDim = TRUE;
	}

	if (requestUndim) {
		if (__OSSetVIForceDimming(FALSE, 2, 2) == TRUE) {
			requestUndim = FALSE;
			isDimmed     = FALSE;
		}
	}

	if (requestDim) {
		if (__OSSetVIForceDimming(TRUE, 2, 2) == TRUE) {
			requestDim = FALSE;
			isDimmed   = TRUE;
		}
	}

	if (oldDvdStopEnabled != dvdStopEnabled) {
		if (!dvdStopEnabled && dvdStopCount >= timeToDVDStop) {
			__DVDRestartMotor();
		}
		dvdStopCount = 0;
	}

	if (dvdStopCount == timeToDVDStop) {
		__DVDStopMotorAsync(&__DVDStopMotorCommandBlock, NULL);
	}

	rfIdle = TRUE;
	siIdle = TRUE;
	for (i = 0; i < VI_NUM_IDLE_DEVICES; i++) {
		devIdle[i] = TRUE;
	}

	oldDimmingEnabled = dimmingEnabled;
	oldDvdStopEnabled = dvdStopEnabled;

	if (timeToDimming > dimmingCount && !isDimmed) {
		dimmingThreshold = timeToDimming;
	}
}

VIRetraceCallback VISetPreRetraceCallback(VIRetraceCallback cb)
{
	BOOL enabled;
	VIRetraceCallback oldcb;

	oldcb   = PreCB;
	enabled = OSDisableInterrupts();
	PreCB   = cb;
	OSRestoreInterrupts(enabled);
	return oldcb;
}

VIRetraceCallback VISetPostRetraceCallback(VIRetraceCallback cb)
{
	BOOL enabled;
	VIRetraceCallback oldcb;

	oldcb   = PostCB;
	enabled = OSDisableInterrupts();
	PostCB  = cb;
	OSRestoreInterrupts(enabled);
	return oldcb;
}

static VITiming* getTiming(VITVMode mode)
{
	switch (mode) {
	case VI_TVMODE_NTSC_INT:
		return &timing[0];
	case VI_TVMODE_NTSC_DS:
		return &timing[1];
	case VI_TVMODE_PAL_INT:
		return &timing[2];
	case VI_TVMODE_PAL_DS:
		return &timing[3];
	case VI_TVMODE_EURGB60_INT:
		return &timing[0];
	case VI_TVMODE_EURGB60_DS:
		return &timing[1];
	case VI_TVMODE_MPAL_INT:
		return &timing[4];
	case VI_TVMODE_MPAL_DS:
		return &timing[5];
	case VI_TVMODE_NTSC_PROG:
	case VI_TVMODE_MPAL_PROG:
	case VI_TVMODE_EURGB60_PROG:
		return &timing[6];
	case VI_TVMODE_NTSC_3D:
		return &timing[7];
	case VI_TVMODE_DEBUG_PAL_INT:
		return &timing[2];
	case VI_TVMODE_DEBUG_PAL_DS:
		return &timing[3];
	case VI_TVMODE_GCA_INT:
		return &timing[8];
	case VI_TVMODE_GCA_PROG:
		return &timing[9];
	case VI_TVMODE_PAL_PROG:
		return &timing[10];
	case 28:
	case 29:
	case 30:
	case 34:
		return timingExtra;
	default:
		return NULL;
	}
}

void __VIInit(VITVMode mode)
{
	VITiming* tm;
	u32 nonInter;
	u32 tv;
	u32 tvForReg;
	volatile u32 a;
	u16 hct;
	u16 vct;

	nonInter                        = mode & 3;
	tv                              = (u32)mode >> 2;
	*(u32*)OSPhysicalToCached(0xCC) = tv;
	tm                              = getTiming(mode);
	__VIRegs[VI_DISP_CONFIG]        = 2;

	for (a = 0; a < 1000; a++) { }

	__VIRegs[VI_DISP_CONFIG]     = 0;
	__VIRegs[VI_HORIZ_TIMING_0U] = (u32)tm->hlw;
	__VIRegs[VI_HORIZ_TIMING_0L] = tm->hce | (tm->hcs << 8);
	__VIRegs[VI_HORIZ_TIMING_1U] = tm->hsy | ((tm->hbe640 & 0x1FF) << 7);
	__VIRegs[VI_HORIZ_TIMING_1L] = (tm->hbe640 >> 9) | ((tm->hbs640 & 0xFFFF) << 1);
	if (encoderType == 0) {
		__VIRegs[0x39] = tm->hbeCCIR656 | 0x8000;
		__VIRegs[0x3A] = (u32)tm->hbsCCIR656;
	}
	__VIRegs[VI_VERT_TIMING]        = (u32)tm->equ;
	__VIRegs[VI_VERT_TIMING_ODD_U]  = (u32)(tm->prbOdd + (tm->acv * 2) - 2);
	__VIRegs[VI_VERT_TIMING_ODD]    = (u32)(tm->psbOdd + 2);
	__VIRegs[VI_VERT_TIMING_EVEN_U] = (u32)(tm->prbEven + (tm->acv * 2) - 2);
	__VIRegs[VI_VERT_TIMING_EVEN]   = (u32)(tm->psbEven + 2);
	__VIRegs[VI_BBI_ODD_U]          = tm->bs1 | (tm->be1 << 5);
	__VIRegs[VI_BBI_ODD]            = tm->bs3 | (tm->be3 << 5);
	__VIRegs[VI_BBI_EVEN_U]         = tm->bs2 | (tm->be2 << 5);
	__VIRegs[VI_BBI_EVEN]           = tm->bs4 | (tm->be4 << 5);
	__VIRegs[VI_HSW]                = 0x2828;
	__VIRegs[VI_DISP_INT_1U]        = 1;
	__VIRegs[VI_DISP_INT_1]         = 0x1001;
	hct                             = tm->hlw + 1;
	vct                             = (tm->numHalfLines / 2) + 1;
	__VIRegs[VI_DISP_INT_0U]        = (u16)(u32)hct;
	__VIRegs[VI_DISP_INT_0]         = vct | 0x1000;

	switch (tv) {
	case VI_PAL:
	case VI_MPAL:
	case VI_DEBUG:
		tvForReg = tv;
		break;
	default:
		tvForReg = 0;
	}

	if (nonInter == VI_INTERLACE || nonInter == VI_NON_INTERLACE) {
		__VIRegs[VI_DISP_CONFIG] = ((nonInter << 2) & 4) | 1 | (tvForReg << 8);
		__VIRegs[VI_CLOCK_SEL]   = 0;
		return;
	}

	__VIRegs[VI_DISP_CONFIG] = (tvForReg << 8) | 5;
	__VIRegs[VI_CLOCK_SEL]   = 1;
}

#define CLAMP_POS(val, min, max) ((val) > (max) ? (max) : (val) < (min) ? (min) : (val))

static void AdjustPosition(u16 acv)
{
	s32 coeff;
	s32 frac;

	HorVer.adjDispPosX  = CLAMP_POS((s16)HorVer.dispPosX + displayOffsetH, 0, 0x2D0 - HorVer.dispSizeX);
	coeff               = (HorVer.xfbMode == VI_XFBMODE_SF) ? 2 : 1;
	frac                = HorVer.dispPosY & 1;
	HorVer.adjDispPosY  = MAX((s16)HorVer.dispPosY + displayOffsetV, frac);
	HorVer.adjDispSizeY = HorVer.dispSizeY + MIN((s16)HorVer.dispPosY + displayOffsetV - frac, 0)
	                    - MAX((s16)HorVer.dispPosY + (s16)HorVer.dispSizeY + displayOffsetV - (((s16)acv * 2) - frac), 0);
	HorVer.adjPanPosY  = HorVer.panPosY - (MIN((s16)HorVer.dispPosY + displayOffsetV - frac, 0) / coeff);
	HorVer.adjPanSizeY = HorVer.panSizeY + (MIN((s16)HorVer.dispPosY + displayOffsetV - frac, 0) / coeff)
	                   - (MAX((s16)HorVer.dispPosY + (s16)HorVer.dispSizeY + displayOffsetV - (((s16)acv * 2) - frac), 0) / coeff);
}

static void ImportAdjustingValues(void)
{
	displayOffsetH = SCGetDisplayOffsetH();
	displayOffsetV = 0;
}

// Not in the dol on their own: VIInit expands both, which is where their bodies are read from (both stand in
// Petari's vi.c as well).
BOOL VIEnableDimming(BOOL enable)
{
	BOOL old;

	old = dimmingEnabled;
	// the console setting wins over the caller
	if (!SCGetScreenSaverMode()) {
		enable = FALSE;
	}
	dimmingEnabled = enable;
	return old;
}

BOOL VIEnableDVDStopMotor(BOOL enable)
{
	BOOL old;

	old            = dvdStopEnabled;
	dvdStopEnabled = enable;
	return old;
}

void VIInit(void)
{
	u16 dspCfg;
	u32 value;
	u32 tv;
	u32 tvInBootrom;

	if (IsInitialized) {
		return;
	}

	OSRegisterVersion(__VIVersion);
	IsInitialized = TRUE;

	if (!(__VIRegs[VI_DISP_CONFIG] & 1)) {
		__VIInit(VI_TVMODE_NTSC_INT);
	}

	retraceCount   = 0;
	changed        = 0;
	shdwChanged    = 0;
	changeMode     = 0;
	shdwChangeMode = 0;
	flushFlag      = 0;
	flush3in1Flag  = 0;

	__VIRegs[VI_FCT_0U] = taps[0] | ((taps[1] & 0x3F) << 10);
	__VIRegs[VI_FCT_0]  = (taps[1] >> 6) | (taps[2] << 4);
	__VIRegs[VI_FCT_1U] = taps[3] | ((taps[4] & 0x3F) << 10);
	__VIRegs[VI_FCT_1]  = (taps[4] >> 6) | (taps[5] << 4);
	__VIRegs[VI_FCT_2U] = taps[6] | ((taps[7] & 0x3F) << 10);
	__VIRegs[VI_FCT_2]  = (taps[7] >> 6) | (taps[8] << 4);
	__VIRegs[VI_FCT_3U] = taps[9] | (taps[10] << 8);
	__VIRegs[VI_FCT_3]  = taps[11] | (taps[12] << 8);
	__VIRegs[VI_FCT_4U] = taps[13] | (taps[14] << 8);
	__VIRegs[VI_FCT_4]  = taps[15] | (taps[16] << 8);
	__VIRegs[VI_FCT_5U] = taps[17] | (taps[18] << 8);
	__VIRegs[VI_FCT_5]  = taps[19] | (taps[20] << 8);
	__VIRegs[VI_FCT_6U] = taps[21] | (taps[22] << 8);
	__VIRegs[VI_FCT_6]  = taps[23] | (taps[24] << 8);
	__VIRegs[VI_WIDTH]  = 0x280;
	ImportAdjustingValues();

	tvInBootrom     = *(u32*)OSPhysicalToCached(0xCC);
	dspCfg          = __VIRegs[VI_DISP_CONFIG];
	HorVer.nonInter = VIGetScanMode();
	HorVer.tv       = ((u32)(dspCfg) & 0x300) >> 8;

	if (tvInBootrom == VI_EURGB60 || (tvInBootrom == VI_PAL && HorVer.tv == VI_NTSC)) {
		HorVer.tv = VI_EURGB60;
	}

	tv                   = (HorVer.tv == VI_DEBUG) ? VI_NTSC : HorVer.tv;
	HorVer.timing        = getTiming((VITVMode)((tv << 2) + HorVer.nonInter));
	regs[VI_DISP_CONFIG] = dspCfg;

	CurrTiming = HorVer.timing;
	CurrTvMode = HorVer.tv;

	HorVer.dispSizeX = 640;
	HorVer.dispSizeY = CurrTiming->acv * 2;
	HorVer.dispPosX  = (720 - HorVer.dispSizeX) / 2;
	HorVer.dispPosY  = 0;
	AdjustPosition(CurrTiming->acv);
	HorVer.fbSizeX  = 640;
	HorVer.fbSizeY  = CurrTiming->acv * 2;
	HorVer.panPosX  = 0;
	HorVer.panPosY  = 0;
	HorVer.panSizeX = 640;
	HorVer.panSizeY = CurrTiming->acv * 2;
	HorVer.xfbMode  = VI_XFBMODE_SF;

	HorVer.wordPerLine = 40;
	HorVer.std         = 40;
	HorVer.wpl         = 40;
	HorVer.xof         = 0;
	HorVer.isBlack     = TRUE;
	HorVer.is3D        = FALSE;
	OSInitThreadQueue(&retraceQueue);
	value = __VIRegs[VI_DISP_INT_0];
	value &= ~0x8000;
	value                   = (u16)value;
	__VIRegs[VI_DISP_INT_0] = value;
	value                   = __VIRegs[VI_DISP_INT_1];
	value                   = value & ~0x8000;
	value                   = (u16)value;
	__VIRegs[VI_DISP_INT_1] = value;
	PreCB                   = NULL;
	PostCB                  = NULL;
	__OSSetInterruptHandler(0x18, __VIRetraceHandler);
	__OSUnmaskInterrupts(0x80);
	OSRegisterShutdownFunction(&ShutdownFunctionInfo);

	switch (VIGetTvFormat()) {
	case VI_PAL:
		dimmingThreshold = 15000;
		timeToDimming    = 15000;
		timeToDVDStop    = 90000;
		break;
	default:
		dimmingThreshold = 18000;
		timeToDimming    = 18000;
		timeToDVDStop    = 108000;
		break;
	}

	dimmingCount    = 0;
	dvdStopCount    = 0;
	dimmingMode     = 0;
	needInitDimming = TRUE;
	isDimmed        = FALSE;
	VIEnableDimming(TRUE);
	VIEnableDVDStopMotor(FALSE);
	__VIInit3in1();
}

void VIWaitForRetrace(void)
{
	BOOL enabled;
	u32 count;

	enabled = OSDisableInterrupts();
	count   = retraceCount;
	do {
		OSSleepThread(&retraceQueue);
	} while (count == retraceCount);
	OSRestoreInterrupts(enabled);
}

static void setInterruptRegs(VITiming* tm)
{
	u16 hct, vct;
	u16 borrow;

	vct    = tm->numHalfLines / 2;
	borrow = tm->numHalfLines % 2;
	if (borrow != 0) {
		hct = tm->hlw;
	} else {
		hct = 0;
	}
	vct++;
	hct++;
	regs[VI_DISP_INT_0U] = (u16)(u32)hct;
	MARK_CHANGED(VI_DISP_INT_0U);
	regs[VI_DISP_INT_0] = vct | 0x1000;
	MARK_CHANGED(VI_DISP_INT_0);
}

static void setPicConfig(u16 fbSizeX, VIXFBMode xfbMode, u16 panPosX, u16 panSizeX, u8* wordPerLine, u8* std, u8* wpl, u8* xof)
{
	*wordPerLine = (fbSizeX + 15) / 16;
	*std         = (xfbMode == VI_XFBMODE_SF) ? *wordPerLine : (u8)(*wordPerLine * 2);
	*xof         = panPosX % 16;
	*wpl         = (*xof + panSizeX + 15) / 16;
	regs[VI_HSW] = *std | (*wpl << 8);
	MARK_CHANGED(VI_HSW);
}

static void setBBIntervalRegs(VITiming* tm)
{
	u16 val;

	val                = tm->bs1 | (tm->be1 << 5);
	regs[VI_BBI_ODD_U] = val;
	MARK_CHANGED(VI_BBI_ODD_U);

	val              = tm->bs3 | (tm->be3 << 5);
	regs[VI_BBI_ODD] = val;
	MARK_CHANGED(VI_BBI_ODD);

	val                 = tm->bs2 | (tm->be2 << 5);
	regs[VI_BBI_EVEN_U] = val;
	MARK_CHANGED(VI_BBI_EVEN_U);

	val               = tm->bs4 | (tm->be4 << 5);
	regs[VI_BBI_EVEN] = val;
	MARK_CHANGED(VI_BBI_EVEN);
}

static void setScalingRegs(u16 panSizeX, u16 dispSizeX, BOOL threeD)
{
	u32 scale;

	panSizeX = threeD ? (panSizeX << 1) : panSizeX;
	if (panSizeX < dispSizeX) {
		scale        = (u32)(dispSizeX + (panSizeX << 8) - 1) / dispSizeX;
		regs[VI_HSR] = scale | 0x1000;
		MARK_CHANGED(VI_HSR);
		regs[VI_WIDTH] = (u32)panSizeX;
		MARK_CHANGED(VI_WIDTH);
	} else {
		regs[VI_HSR] = 0x100;
		MARK_CHANGED(VI_HSR);
	}
}

static void calcFbbs(u32 bufAddr, u16 panPosX, u16 panPosY, u8 wordPerLine, VIXFBMode xfbMode, u16 dispPosY, u32* tfbb, u32* bfbb)
{
	u32 bytesPerLine;
	u32 xoffInWords;
	u32 tmp;

	xoffInWords  = (panPosX & ~0xF) >> 4;
	bytesPerLine = (wordPerLine & 0xFF) << 5;
	*tfbb        = bufAddr + (xoffInWords << 5) + (bytesPerLine * panPosY);
	*bfbb        = (xfbMode == VI_XFBMODE_SF) ? *tfbb : *tfbb + bytesPerLine;
	if (dispPosY % 2 == 1) {
		tmp   = *tfbb;
		*tfbb = *bfbb;
		*bfbb = tmp;
	}
	*tfbb &= 0x3FFFFFFF;
	*bfbb &= 0x3FFFFFFF;
}

static void setFbbRegs(VIPositionInfo* HorVer, u32* tfbb, u32* bfbb, u32* rtfbb, u32* rbfbb)
{
	u32 shifted;

	calcFbbs(HorVer->bufAddr, HorVer->panPosX, HorVer->adjPanPosY, HorVer->wordPerLine, HorVer->xfbMode, HorVer->adjDispPosY, tfbb, bfbb);
	if (HorVer->is3D) {
		calcFbbs(HorVer->rbufAddr, HorVer->panPosX, HorVer->adjPanPosY, HorVer->wordPerLine, HorVer->xfbMode, HorVer->adjDispPosY, rtfbb,
		         rbfbb);
	}

	if (*tfbb < 0x01000000U && *bfbb < 0x01000000U && *rtfbb < 0x01000000U && *rbfbb < 0x01000000U) {
		shifted = 0;
	} else {
		shifted = 1;
	}

	if (shifted) {
		*tfbb >>= 5;
		*bfbb >>= 5;
		*rtfbb >>= 5;
		*rbfbb >>= 5;
	}

	regs[VI_TOP_FIELD_BASE_LEFT_U] = (u16)*tfbb & 0xFFFF;
	MARK_CHANGED(VI_TOP_FIELD_BASE_LEFT_U);
	regs[VI_TOP_FIELD_BASE_LEFT] = (shifted << 12) | ((*tfbb >> 16) | (HorVer->xof << 8));
	MARK_CHANGED(VI_TOP_FIELD_BASE_LEFT);
	regs[VI_BTTM_FIELD_BASE_LEFT_U] = (u16)*bfbb & 0xFFFF;
	MARK_CHANGED(VI_BTTM_FIELD_BASE_LEFT_U);
	regs[VI_BTTM_FIELD_BASE_LEFT] = (*bfbb >> 16);
	MARK_CHANGED(VI_BTTM_FIELD_BASE_LEFT);

	if (HorVer->is3D) {
		regs[VI_TOP_FIELD_BASE_RIGHT_U] = (u16)*rtfbb & 0xFFFF;
		MARK_CHANGED(VI_TOP_FIELD_BASE_RIGHT_U);
		regs[VI_TOP_FIELD_BASE_RIGHT] = *rtfbb >> 16;
		MARK_CHANGED(VI_TOP_FIELD_BASE_RIGHT);
		regs[VI_BTTM_FIELD_BASE_RIGHT_U] = (u16)*rbfbb & 0xFFFF;
		MARK_CHANGED(VI_BTTM_FIELD_BASE_RIGHT_U);
		regs[VI_BTTM_FIELD_BASE_RIGHT] = *rbfbb >> 16;
		MARK_CHANGED(VI_BTTM_FIELD_BASE_RIGHT);
	}
}

static void setHorizontalRegs(VITiming* tm, u16 dispPosX, u16 dispSizeX)
{
	u32 hbe;
	u32 hbs;
	u32 hbeLo;
	u32 hbeHi;

	regs[VI_HORIZ_TIMING_0U] = (u16)(u32)tm->hlw;
	MARK_CHANGED(VI_HORIZ_TIMING_0U);
	regs[VI_HORIZ_TIMING_0L] = tm->hce | (tm->hcs << 8);
	MARK_CHANGED(VI_HORIZ_TIMING_0L);

	if (HorVer.tv == VI_TV_EXTRA) {
		hbe = tm->hbe640 + 172;
		hbs = tm->hbs640;
	} else {
		hbe = tm->hbe640 - 40 + dispPosX;
		hbs = tm->hbs640 + 40 + dispPosX - (720 - dispSizeX);
	}

	hbeLo                    = hbe & 0x1FF;
	hbeHi                    = hbe >> 9;
	regs[VI_HORIZ_TIMING_1U] = tm->hsy | (hbeLo << 7);
	MARK_CHANGED(VI_HORIZ_TIMING_1U);
	regs[VI_HORIZ_TIMING_1L] = hbeHi | (hbs * 2);
	MARK_CHANGED(VI_HORIZ_TIMING_1L);
}

static void setVerticalRegs(u16 dispPosY, u16 dispSizeY, u8 equ, u16 acv, u16 prbOdd, u16 prbEven, u16 psbOdd, u16 psbEven, BOOL black)
{
	u16 actualPrbOdd;
	u16 actualPrbEven;
	u16 actualPsbOdd;
	u16 actualPsbEven;
	u16 actualAcv;
	u16 c;
	u16 d;

	if (HorVer.nonInter == VI_PROGRESSIVE || HorVer.nonInter == VI_3D) {
		c = 1;
		d = 2;
	} else {
		c = 2;
		d = 1;
	}

	if ((dispPosY % 2) == 0) {
		actualPrbOdd  = prbOdd + (d * dispPosY);
		actualPsbOdd  = psbOdd + (d * (((c * acv) - dispSizeY) - dispPosY));
		actualPrbEven = prbEven + (d * dispPosY);
		actualPsbEven = psbEven + (d * (((c * acv) - dispSizeY) - dispPosY));
	} else {
		actualPrbOdd  = prbEven + (d * dispPosY);
		actualPsbOdd  = psbEven + (d * (((c * acv) - dispSizeY) - dispPosY));
		actualPrbEven = prbOdd + (d * dispPosY);
		actualPsbEven = psbOdd + (d * (((c * acv) - dispSizeY) - dispPosY));
	}

	actualAcv = dispSizeY / c;

	if (black) {
		actualPrbOdd += 2 * actualAcv - 2;
		actualPsbOdd += 2;
		actualPrbEven += 2 * actualAcv - 2;
		actualPsbEven += 2;
		actualAcv = 0;
	}

	regs[VI_VERT_TIMING] = equ | (actualAcv << 4);
	MARK_CHANGED(VI_VERT_TIMING);
	regs[VI_VERT_TIMING_ODD_U] = (u16)(u32)actualPrbOdd;
	MARK_CHANGED(VI_VERT_TIMING_ODD_U);
	regs[VI_VERT_TIMING_ODD] = (u16)(u32)actualPsbOdd;
	MARK_CHANGED(VI_VERT_TIMING_ODD);
	regs[VI_VERT_TIMING_EVEN_U] = (u16)(u32)actualPrbEven;
	MARK_CHANGED(VI_VERT_TIMING_EVEN_U);
	regs[VI_VERT_TIMING_EVEN] = (u16)(u32)actualPsbEven;
	MARK_CHANGED(VI_VERT_TIMING_EVEN);
}

static void PrintDebugPalCaution(void)
{
	static u32 message;

	if (message == 0) {
		message = 1;
		OSReport("***************************************\n");
		OSReport(" ! ! ! C A U T I O N ! ! !             \n");
		OSReport("This TV format \"DEBUG_PAL\" is only for \n");
		OSReport("temporary solution until PAL DAC board \n");
		OSReport("is available. Please do NOT use this   \n");
		OSReport("mode in real games!!!                  \n");
		OSReport("***************************************\n");
	}
}

void VIConfigure(const GXRenderModeObj* rm)
{
	VITiming* tm;
	u32 regDspCfg;
	u32 regClksel;
	BOOL enabled;
	u32 newNonInter;
	u32 tvInBootrom;
	u32 tvInGame;

	enabled     = OSDisableInterrupts();
	newNonInter = rm->viTVmode & 3;

	if (HorVer.nonInter != newNonInter) {
		changeMode      = 1;
		HorVer.nonInter = newNonInter;
	}

	tvInGame    = (u32)rm->viTVmode >> 2;
	tvInBootrom = *(u32*)OSPhysicalToCached(0xCC);

	if (tvInGame == VI_DEBUG_PAL) {
		PrintDebugPalCaution();
	}

	if (((tvInBootrom != VI_PAL && tvInBootrom != VI_EURGB60) && (tvInGame == VI_PAL || tvInGame == VI_EURGB60))
	    || ((tvInBootrom == VI_PAL || tvInBootrom == VI_EURGB60) && (tvInGame != VI_PAL && tvInGame != VI_EURGB60))) {
		OSPanic(__FILE__, 2651, "VIConfigure(): Tried to change mode from (%d) to (%d), which is forbidden\n", tvInBootrom, tvInGame);
	}

	if ((tvInGame == VI_NTSC) || (tvInGame == VI_MPAL)) {
		HorVer.tv = tvInBootrom;
	} else {
		HorVer.tv = tvInGame;
	}

	HorVer.dispPosX  = rm->viXOrigin;
	HorVer.dispPosY  = (HorVer.nonInter == VI_NON_INTERLACE) ? (u16)(rm->viYOrigin * 2) : rm->viYOrigin;
	HorVer.dispSizeX = rm->viWidth;
	HorVer.fbSizeX   = rm->fbWidth;
	HorVer.fbSizeY   = rm->xfbHeight;
	HorVer.xfbMode   = rm->xFBmode;
	HorVer.panSizeX  = HorVer.fbSizeX;
	HorVer.panSizeY  = HorVer.fbSizeY;
	HorVer.panPosX   = 0;
	HorVer.panPosY   = 0;
	HorVer.dispSizeY = (HorVer.nonInter == VI_PROGRESSIVE) ? HorVer.panSizeY
	                 : (HorVer.nonInter == VI_3D)          ? HorVer.panSizeY
	                 : (HorVer.xfbMode == VI_XFBMODE_SF)   ? (u16)(HorVer.panSizeY * 2)
	                                                       : HorVer.panSizeY;
	HorVer.is3D      = (HorVer.nonInter == VI_3D) ? TRUE : FALSE;

	tm            = getTiming((VITVMode)((HorVer.tv << 2) + HorVer.nonInter));
	HorVer.timing = tm;

	AdjustPosition(tm->acv);
	setInterruptRegs(tm);

	regDspCfg = regs[VI_DISP_CONFIG];
	regClksel = regs[VI_CLOCK_SEL];
	if (HorVer.nonInter == VI_PROGRESSIVE || HorVer.nonInter == VI_3D) {
		regDspCfg = (((u32)(regDspCfg)) & ~0x00000004) | (((u32)(1)) << 2);
		if (HorVer.tv == VI_TV_EXTRA) {
			regClksel = (((u32)(regClksel)) & ~0x00000001);
		} else {
			regClksel = (((u32)(regClksel)) & ~0x00000001) | (((u32)(1)) << 0);
		}
	} else {
		regDspCfg = (((u32)(regDspCfg)) & ~0x00000004) | (((u32)(HorVer.nonInter & 1)) << 2);
		regClksel = (((u32)(regClksel)) & ~0x00000001);
	}

	regDspCfg = (((u32)(regDspCfg)) & ~0x00000008) | (((u32)(HorVer.is3D)) << 3);

	if ((HorVer.tv == VI_PAL) || (HorVer.tv == VI_MPAL) || (HorVer.tv == VI_DEBUG)) {
		regDspCfg = (((u32)(regDspCfg)) & ~0x00000300) | (((u32)(HorVer.tv)) << 8);
	} else {
		regDspCfg = (((u32)(regDspCfg)) & ~0x00000300);
	}

	regs[VI_DISP_CONFIG] = regDspCfg;
	regs[VI_CLOCK_SEL]   = (u16)regClksel;

	MARK_CHANGED(VI_DISP_CONFIG);
	MARK_CHANGED(VI_CLOCK_SEL);

	setScalingRegs(HorVer.panSizeX, HorVer.dispSizeX, HorVer.is3D);
	setHorizontalRegs(tm, HorVer.adjDispPosX, HorVer.dispSizeX);
	setBBIntervalRegs(tm);
	setPicConfig(HorVer.fbSizeX, HorVer.xfbMode, HorVer.panPosX, HorVer.panSizeX, &HorVer.wordPerLine, &HorVer.std, &HorVer.wpl,
	             &HorVer.xof);
	if (FBSet != 0) {
		setFbbRegs(&HorVer, &HorVer.tfbb, &HorVer.bfbb, &HorVer.rtfbb, &HorVer.rbfbb);
	}
	setVerticalRegs(HorVer.adjDispPosY, HorVer.adjDispSizeY, tm->equ, tm->acv, tm->prbOdd, tm->prbEven, tm->psbOdd, tm->psbEven,
	                HorVer.isBlack);
	OSRestoreInterrupts(enabled);
}

void VIConfigurePan(u16 xOrg, u16 yOrg, u16 width, u16 height)
{
	BOOL enabled;
	VITiming* tm;

	enabled          = OSDisableInterrupts();
	HorVer.panPosX   = xOrg;
	HorVer.panPosY   = yOrg;
	HorVer.panSizeX  = width;
	HorVer.panSizeY  = height;
	HorVer.dispSizeY = (HorVer.nonInter == VI_PROGRESSIVE) ? HorVer.panSizeY
	                 : (HorVer.nonInter == VI_3D)          ? HorVer.panSizeY
	                 : (HorVer.xfbMode == VI_XFBMODE_SF)   ? (u16)(HorVer.panSizeY * 2)
	                                                       : HorVer.panSizeY;
	tm               = HorVer.timing;
	AdjustPosition(tm->acv);
	setScalingRegs(HorVer.panSizeX, HorVer.dispSizeX, HorVer.is3D);
	setPicConfig(HorVer.fbSizeX, HorVer.xfbMode, HorVer.panPosX, HorVer.panSizeX, &HorVer.wordPerLine, &HorVer.std, &HorVer.wpl,
	             &HorVer.xof);
	if (FBSet != 0) {
		setFbbRegs(&HorVer, &HorVer.tfbb, &HorVer.bfbb, &HorVer.rtfbb, &HorVer.rbfbb);
	}
	setVerticalRegs(HorVer.adjDispPosY, HorVer.dispSizeY, tm->equ, tm->acv, tm->prbOdd, tm->prbEven, tm->psbOdd, tm->psbEven,
	                HorVer.isBlack);
	OSRestoreInterrupts(enabled);
}

void VIFlush(void)
{
	BOOL enabled;
	s32 regIndex;

	enabled = OSDisableInterrupts();
	shdwChangeMode |= changeMode;
	changeMode = 0;
	shdwChanged |= changed;

	while (changed != 0) {
		regIndex           = cntlzd(changed);
		shdwRegs[regIndex] = regs[regIndex];
		changed &= ~((u64)1 << (63 - regIndex));
	}

	flushFlag     = 1;
	flush3in1Flag = 1;
	NextBufAddr   = HorVer.bufAddr;
	OSRestoreInterrupts(enabled);
}

void VISetNextFrameBuffer(void* fb)
{
	BOOL enabled;

	enabled        = OSDisableInterrupts();
	HorVer.bufAddr = (u32)fb;
	FBSet          = 1;
	setFbbRegs(&HorVer, &HorVer.tfbb, &HorVer.bfbb, &HorVer.rtfbb, &HorVer.rbfbb);
	OSRestoreInterrupts(enabled);
}

void VISetBlack(BOOL black)
{
	BOOL enabled;
	VITiming* tm;

	enabled        = OSDisableInterrupts();
	HorVer.isBlack = black;
	tm             = HorVer.timing;
	setVerticalRegs(HorVer.adjDispPosY, HorVer.dispSizeY, tm->equ, tm->acv, tm->prbOdd, tm->prbEven, tm->psbOdd, tm->psbEven,
	                HorVer.isBlack);
	OSRestoreInterrupts(enabled);
}

u32 VIGetRetraceCount(void)
{
	return retraceCount;
}

static u32 getCurrentHalfLine(void)
{
	u32 hcount, vcount;
	GetCurrentDisplayPosition(&hcount, &vcount);

	return ((vcount - 1) << 1) + ((hcount - 1) / CurrTiming->hlw);
}

static u32 getCurrentFieldEvenOdd(void)
{
	return (getCurrentHalfLine() < CurrTiming->numHalfLines) ? 1 : 0;
}

u32 VIGetCurrentLine(void)
{
	u32 halfLine;
	VITiming* tm;
	BOOL enabled;

	tm       = CurrTiming;
	enabled  = OSDisableInterrupts();
	halfLine = getCurrentHalfLine();
	OSRestoreInterrupts(enabled);
	if (halfLine >= tm->numHalfLines) {
		halfLine -= tm->numHalfLines;
	}
	return halfLine >> 1U;
}

u32 VIGetTvFormat(void)
{
	u32 format;
	BOOL enabled;

	enabled = OSDisableInterrupts();

	switch (CurrTvMode) {
	case VI_NTSC:
	case VI_DEBUG:
	case 6:
	case 7:
	case VI_TV_EXTRA:
		format = VI_NTSC;
		break;
	case VI_PAL:
	case VI_DEBUG_PAL:
		format = VI_PAL;
		break;
	case VI_EURGB60:
	case VI_MPAL:
		format = CurrTvMode;
		break;
	}

	OSRestoreInterrupts(enabled);
	return format;
}

u32 VIGetScanMode(void)
{
	u32 scanMode;
	BOOL enabled = OSDisableInterrupts();

	if ((u32)(__VIRegs[VI_CLOCK_SEL] & 1) == 1) {
		scanMode = 2;
	} else if (!((__VIRegs[VI_DISP_CONFIG] & (1 << 2)) >> 2)) {
		scanMode = 0;
	} else {
		scanMode = 1;
	}

	OSRestoreInterrupts(enabled);
	return scanMode;
}

u32 VIGetDTVStatus(void)
{
	u32 dtvStatus;
	BOOL enabled = OSDisableInterrupts();

	dtvStatus = __VIRegs[VI_DTV_STAT] & 3;
	OSRestoreInterrupts(enabled);
	return dtvStatus & 1;
}

void __VIDisplayPositionToXY(u32 hct, u32 vct, s16* x, s16* y)
{
	u32 halfLine = ((vct - 1) << 1) + ((hct - 1) / CurrTiming->hlw);

	if (HorVer.nonInter == VI_INTERLACE) {
		if (halfLine < CurrTiming->numHalfLines) {
			if (halfLine < CurrTiming->equ * 3 + CurrTiming->prbOdd) {
				*y = -1;
			} else if (halfLine >= CurrTiming->numHalfLines - CurrTiming->psbOdd) {
				*y = -1;
			} else {
				*y = (s16)((halfLine - CurrTiming->equ * 3 - CurrTiming->prbOdd) & ~1);
			}
		} else {
			halfLine -= CurrTiming->numHalfLines;

			if (halfLine < CurrTiming->equ * 3 + CurrTiming->prbEven) {
				*y = -1;
			} else if (halfLine >= CurrTiming->numHalfLines - CurrTiming->psbEven) {
				*y = -1;
			} else {
				*y = (s16)(((halfLine - CurrTiming->equ * 3 - CurrTiming->prbEven) & ~1) + 1);
			}
		}
	} else if (HorVer.nonInter == VI_NON_INTERLACE) {
		if (halfLine >= CurrTiming->numHalfLines) {
			halfLine -= CurrTiming->numHalfLines;
		}

		if (halfLine < CurrTiming->equ * 3 + CurrTiming->prbOdd) {
			*y = -1;
		} else if (halfLine >= CurrTiming->numHalfLines - CurrTiming->psbOdd) {
			*y = -1;
		} else {
			*y = (s16)((halfLine - CurrTiming->equ * 3 - CurrTiming->prbOdd) & ~1);
		}
	} else if (HorVer.nonInter == VI_PROGRESSIVE) {
		if (halfLine < CurrTiming->numHalfLines) {
			if (halfLine < CurrTiming->equ * 3 + CurrTiming->prbOdd) {
				*y = -1;
			} else if (halfLine >= CurrTiming->numHalfLines - CurrTiming->psbOdd) {
				*y = -1;
			} else {
				*y = (s16)(halfLine - CurrTiming->equ * 3 - CurrTiming->prbOdd);
			}
		} else {
			halfLine -= CurrTiming->numHalfLines;

			if (halfLine < CurrTiming->equ * 3 + CurrTiming->prbEven) {
				*y = -1;
			} else if (halfLine >= CurrTiming->numHalfLines - CurrTiming->psbEven) {
				*y = -1;
			} else {
				*y = (s16)((halfLine - CurrTiming->equ * 3 - CurrTiming->prbEven) & ~1);
			}
		}
	}

	*x = (s16)(hct - 1);
}

void __VIGetCurrentPosition(s16* x, s16* y)
{
	u32 hcount, vcount;
	GetCurrentDisplayPosition(&hcount, &vcount);
	__VIDisplayPositionToXY(hcount, vcount, x, y);
}

u32 VISetTimeToDimming(u32 mode)
{
	u32 old;

	old         = dimmingMode;
	dimmingMode = mode;

	switch (VIGetTvFormat()) {
	case VI_PAL:
		switch (dimmingMode) {
		case 1:
			timeToDimming = 30000;
			break;
		case 2:
			timeToDimming = 45000;
			break;
		default:
			timeToDimming = 15000;
			break;
		}
		break;
	default:
		switch (dimmingMode) {
		case 1:
			timeToDimming = 36000;
			break;
		case 2:
			timeToDimming = 54000;
			break;
		default:
			timeToDimming = 18000;
			break;
		}
		break;
	}

	return old;
}

BOOL VIResetDimmingCount(void)
{
	devIdle[0] = FALSE;
	return TRUE;
}

BOOL __VIResetRFIdle(void)
{
	rfIdle = FALSE;
	return TRUE;
}

BOOL __VIResetSIIdle(void)
{
	siIdle = FALSE;
	return TRUE;
}
