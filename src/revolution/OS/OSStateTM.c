#include "revolution/hw_regs.h"
#include "revolution/ipc.h"

#include "Dolphin/os.h"

typedef void (*OSStateCallback)(void);

// other files of the library
BOOL VIResetDimmingCount(void);

static s32 __OSVIDimReplyHandler(s32 result, void* arg);
static void __OSDefaultResetCallback(void);
static void __OSDefaultPowerCallback(void);
static s32 __OSStateEventHandler(s32 result, void* arg);

static u32 StmEhInBuf[8] ATTRIBUTE_ALIGN(32);
static u32 StmEhOutBuf[8] ATTRIBUTE_ALIGN(32);
static u32 StmImInBuf[8] ATTRIBUTE_ALIGN(32);
static u32 StmImOutBuf[8] ATTRIBUTE_ALIGN(32);
static u32 StmVdInBuf[8] ATTRIBUTE_ALIGN(32);
static u32 StmVdOutBuf[8] ATTRIBUTE_ALIGN(32);

static OSStateCallback ResetCallback;
static OSStateCallback PowerCallback;
static BOOL StmVdInUse;
static BOOL StmEhRegistered;
static s32 StmEhDesc;
static s32 StmImDesc;
static BOOL StmReady;
static BOOL ResetDown;

static inline s32 __OSRegisterStateEvent(void)
{
	BOOL enabled;
	s32 result;

	enabled = OSDisableInterrupts();
	result
	    = IOS_IoctlAsync(StmEhDesc, 0x1000, StmEhInBuf, sizeof(StmEhInBuf), StmEhOutBuf, sizeof(StmEhOutBuf), __OSStateEventHandler, NULL);
	if (result == 0) {
		StmEhRegistered = TRUE;
	} else {
		StmEhRegistered = FALSE;
	}
	OSRestoreInterrupts(enabled);

	return result;
}

OSStateCallback OSSetPowerCallback(OSStateCallback callback)
{
	BOOL enabled;
	OSStateCallback prev;

	enabled = OSDisableInterrupts();
	prev    = PowerCallback;
	if (callback) {
		PowerCallback = callback;
	} else {
		PowerCallback = __OSDefaultPowerCallback;
	}

	if (!StmEhRegistered) {
		__OSRegisterStateEvent();
	}
	OSRestoreInterrupts(enabled);

	if (prev == __OSDefaultPowerCallback) {
		return NULL;
	}
	return prev;
}

BOOL OSGetResetButtonState(void)
{
	BOOL enabled;
	BOOL down;

	enabled   = OSDisableInterrupts();
	down      = ResetDown;
	ResetDown = FALSE;
	OSRestoreInterrupts(enabled);

	if (!StmEhRegistered) {
		__OSRegisterStateEvent();
	}

	return down;
}

BOOL __OSInitSTM(void)
{
	PowerCallback = __OSDefaultPowerCallback;
	ResetCallback = __OSDefaultResetCallback;
	ResetDown     = FALSE;

	if (StmReady) {
		return TRUE;
	}

	StmVdInUse = FALSE;

	StmImDesc = IOS_Open("/dev/stm/immediate", 0);
	if (StmImDesc < 0) {
		StmReady = FALSE;
		return FALSE;
	}

	StmEhDesc = IOS_Open("/dev/stm/eventhook", 0);
	if (StmEhDesc < 0) {
		StmReady = FALSE;
		return FALSE;
	}

	__OSRegisterStateEvent();
	StmReady = TRUE;
	return TRUE;
}

static inline void LockUp(void)
{
	OSDisableInterrupts();
	ICFlashInvalidate();
	while (TRUE) { }
}

void __OSShutdownToSBY(void)
{
	__VIRegs[VI_DISP_CONFIG] = 0;

	if (!StmReady) {
		OSPanic(__FILE__, 0x15C, "Error: The firmware doesn't support shutdown feature.\n");
	}

	StmImInBuf[0] = 0;
	IOS_Ioctl(StmImDesc, 0x2003, StmImInBuf, sizeof(StmImInBuf), StmImOutBuf, sizeof(StmImOutBuf));
	LockUp();
}

void __OSHotReset(void)
{
	__VIRegs[VI_DISP_CONFIG] = 0;

	if (!StmReady) {
		OSPanic(__FILE__, 0x19C, "Error: The firmware doesn't support reboot feature.\n");
	}

	IOS_Ioctl(StmImDesc, 0x2001, StmImInBuf, sizeof(StmImInBuf), StmImOutBuf, sizeof(StmImOutBuf));
	LockUp();
}

s32 __OSSetVIForceDimming(BOOL enable, u32 luma, u32 chroma)
{
	BOOL enabled;
	s32 result;

	if (!StmReady) {
		return -10;
	}

	enabled = OSDisableInterrupts();
	if (StmVdInUse) {
		OSRestoreInterrupts(enabled);
		return 0;
	}
	StmVdInUse = TRUE;
	OSRestoreInterrupts(enabled);

	StmVdInBuf[0] = luma << 3 | chroma | enable << 7;
	StmVdInBuf[1] = 0;
	StmVdInBuf[2] = 0;
	StmVdInBuf[3] = 0;
	StmVdInBuf[4] = 0;
	StmVdInBuf[5] = 0xFFFFFFFF;
	StmVdInBuf[6] = 0xFFFF0000;
	StmVdInBuf[7] = 0;

	result
	    = IOS_IoctlAsync(StmImDesc, 0x5001, StmVdInBuf, sizeof(StmVdInBuf), StmVdOutBuf, sizeof(StmVdOutBuf), __OSVIDimReplyHandler, NULL);
	if (result != 0) {
		return result;
	}
	return 1;
}

s32 __OSUnRegisterStateEvent(void)
{
	s32 result;

	if (!StmEhRegistered) {
		return 0;
	}

	if (!StmReady) {
		return -6;
	}

	result = IOS_Ioctl(StmImDesc, 0x3002, StmImInBuf, sizeof(StmImInBuf), StmImOutBuf, sizeof(StmImOutBuf));
	if (result == 0) {
		StmEhRegistered = FALSE;
	}

	return result;
}

static s32 __OSVIDimReplyHandler(s32 result, void* arg)
{
	StmVdInUse = FALSE;
	return 0;
}

static void __OSDefaultResetCallback(void)
{
}

static void __OSDefaultPowerCallback(void)
{
}

static inline BOOL __OSGetResetButtonStateRaw(void)
{
	return !(__PIRegs[0] & 0x10000) ? TRUE : FALSE;
}

static s32 __OSStateEventHandler(s32 result, void* arg)
{
	BOOL enabled;
	OSStateCallback callback;

	if (result != 0) {
		OSPanic(__FILE__, 0x334, "Error on STM state event handler\n");
	}

	StmEhRegistered = FALSE;

	if (StmEhOutBuf[0] == 0x20000) {
		if (__OSGetResetButtonStateRaw()) {
			enabled       = OSDisableInterrupts();
			callback      = ResetCallback;
			ResetDown     = TRUE;
			ResetCallback = __OSDefaultResetCallback;
			callback();
			OSRestoreInterrupts(enabled);
			VIResetDimmingCount();
		}
		__OSRegisterStateEvent();
	}

	if (StmEhOutBuf[0] == 0x800) {
		enabled       = OSDisableInterrupts();
		callback      = PowerCallback;
		PowerCallback = __OSDefaultPowerCallback;
		callback();
		OSRestoreInterrupts(enabled);
	}

	return 0;
}
