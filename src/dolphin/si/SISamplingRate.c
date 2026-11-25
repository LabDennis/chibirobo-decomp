#include "types.h"
#include "Dolphin/OS.h"
#include "Dolphin/vi.h"
#include "Dolphin/hw_regs.h"
#include "Dolphin/si.h"

typedef struct SIPollTiming {
	u16 x;
	u8 y;
} SIPollTiming;

static u32 SamplingRate;
static SIPollTiming XYNTSC[12] = {
	{ 0x00F6, 0x02 }, { 0x000E, 0x13 }, { 0x001E, 0x09 }, { 0x002C, 0x06 },
	{ 0x0034, 0x05 }, { 0x0041, 0x04 }, { 0x0057, 0x03 }, { 0x0057, 0x03 },
	{ 0x0057, 0x03 }, { 0x0083, 0x02 }, { 0x0083, 0x02 }, { 0x0083, 0x02 },
};
static SIPollTiming XYPAL[12] = {
	{ 0x0128, 0x02 }, { 0x000F, 0x15 }, { 0x001D, 0x0B }, { 0x002D, 0x07 },
	{ 0x0034, 0x06 }, { 0x003F, 0x05 }, { 0x004E, 0x04 }, { 0x0068, 0x03 },
	{ 0x0068, 0x03 }, { 0x0068, 0x03 }, { 0x0068, 0x03 }, { 0x009C, 0x02 },
};

/**
 * @brief Sets the controller port sampling rate in milliseconds.
 *
 * All the attached controller devices share the same sampling rate setting.
 * Since the sampling rate is controlled by the Flipper's video clock, the actual sampling rate set differs from the one specified.
 *
 * @param msec The desired sampling rate in milliseconds. If the value is greater than 11, it will be set to 11.
 *
 * @note Address: 0x80161048
 * @note Size: 0xE4
 */
void SISetSamplingRate(u32 msec)
{
	BOOL interruptState;
	u32 xScale;
	const SIPollTiming* format;

	if (msec > 11) {
		msec = 11;
	}

	interruptState = OSDisableInterrupts();
	SamplingRate   = msec;

	switch (VIGetTvFormat()) {
	case VI_NTSC:
	case VI_MPAL:
	case VI_EURGB60:
		format = XYNTSC;
		break;
	case VI_PAL:
		format = XYPAL;
		break;
	default:
		OSReport("SISetSamplingRate: unknown TV format. Use default.");
		msec   = 0;
		format = XYNTSC;
		break;
	}

	if ((__VIRegs[VI_CLOCK_SEL] & 1) != 0) {
		xScale = 2;
	} else {
		xScale = 1;
	}

	SISetXY(xScale * format[msec].x, format[msec].y);
	OSRestoreInterrupts(interruptState);
}

/**
 * @note Address: 0x8016112C
 * @note Size: 0x24
 */
void SIRefreshSamplingRate(void)
{
	SISetSamplingRate(SamplingRate);
}
