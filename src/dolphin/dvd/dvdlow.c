#include "Dolphin/os.h"
#include "Dolphin/hw_regs.h"

WEAK void __DVDLowSetWAType(u32 type, s32 seekLoc);

static BOOL FirstRead                    = TRUE;
static volatile BOOL StopAtNextInt       = FALSE;
static u32 LastLength                    = 0;
static DVDLowCallback Callback           = NULL;
static DVDLowCallback ResetCoverCallback = NULL;
static volatile OSTime LastResetEnd      = 0;
static vu32 ResetOccurred                = FALSE;
static volatile BOOL WaitingCoverClose   = FALSE;
static BOOL Breaking                     = FALSE;
static vu32 WorkAroundType               = 0;
static s32 WorkAroundSeekLocation        = 0;
static volatile OSTime LastReadFinished  = 0;
static OSTime LastReadIssued             = 0;
static volatile BOOL LastCommandWasRead  = FALSE;
static vu32 NextCommandNumber            = 0;

typedef struct DVDBuffer {
	void* addr;
	u32 length;
	u32 offset;
} DVDBuffer;

typedef struct DVDCommand {
	s32 cmd;
	void* addr;
	u32 length;
	u32 offset;
	DVDLowCallback callback;
} DVDCommand;

static DVDCommand CommandList[3];
static OSAlarm AlarmForWA;
static OSAlarm AlarmForTimeout;
static OSAlarm AlarmForBreak;
static DVDBuffer Prev;
static DVDBuffer Curr;

/**
 * @note Address: 0x8016C8CC
 * @note Size: 0x40
 */
WEAK void __DVDInitWA(void)
{
	NextCommandNumber  = 0;
	CommandList[0].cmd = -1;
	__DVDLowSetWAType(0, 0);
	OSInitAlarm();
}

static void Read(void* addr, u32 length, u32 offset, DVDLowCallback callback);

/**
 * @note Address: N/A
 * @note Size: 0x94
 */
static BOOL ProcessNextCommand(void)
{
	s32 n = NextCommandNumber;

	if (CommandList[n].cmd == 1) {
		++NextCommandNumber;
		Read(CommandList[n].addr, CommandList[n].length, CommandList[n].offset, CommandList[n].callback);
		return TRUE;
	} else if (CommandList[n].cmd == 2) {
		++NextCommandNumber;
		DVDLowSeek(CommandList[n].offset, CommandList[n].callback);
		return TRUE;
	}

	return FALSE;
}

/**
 * @note Address: 0x8016C90C
 * @note Size: 0x2E0
 */
WEAK void __DVDInterruptHandler(__OSInterrupt interrupt, OSContext* context)
{
	DVDLowCallback cb;
	OSContext exceptionContext;
	u32 cause = 0;
	u32 reg;
	u32 intr;
	u32 mask;

	if (LastCommandWasRead) {
		LastReadFinished = __OSGetSystemTime();
		FirstRead        = FALSE;
		Prev.addr        = Curr.addr;
		Prev.length      = Curr.length;
		Prev.offset      = Curr.offset;
		if (StopAtNextInt == TRUE) {
			cause |= 8;
		}
	}

	LastCommandWasRead = FALSE;
	StopAtNextInt      = FALSE;
	reg                = __DIRegs[DI_STATUS];
	mask               = reg & 0x2a;
	intr               = (reg & 0x54) & (mask << 1);

	if (intr & 0x40) {
		cause |= 8;
	}

	if (intr & 0x10) {
		cause |= 1;
	}

	if (intr & 4) {
		cause |= 2;
	}

	if (cause) {
		ResetOccurred = FALSE;
		OSCancelAlarm(&AlarmForTimeout);
	}

	__DIRegs[DI_STATUS] = intr | mask;

	if (ResetOccurred && (__OSGetSystemTime() - LastResetEnd) < OSMillisecondsToTicks(200)) {
		reg  = __DIRegs[DI_COVER_STATUS];
		mask = reg & 0x2;
		intr = (reg & 4) & (mask << 1);
		if (intr & 4) {
			if (ResetCoverCallback) {
				ResetCoverCallback(4);
			}
			ResetCoverCallback = NULL;
		}

		__DIRegs[DI_COVER_STATUS] = __DIRegs[DI_COVER_STATUS];
	} else if (WaitingCoverClose) {
		reg  = __DIRegs[DI_COVER_STATUS];
		mask = reg & 2;
		intr = (reg & 4) & (mask << 1);

		if (intr & 4) {
			cause |= 4;
		}

		__DIRegs[DI_COVER_STATUS] = intr | mask;
		WaitingCoverClose         = FALSE;
	} else {
		__DIRegs[DI_COVER_STATUS] = 0;
	}

	if ((cause & 8) && !Breaking) {
		cause &= ~8;
	}

	if ((cause & 1)) {
		if (ProcessNextCommand()) {
			return;
		}
	} else {
		CommandList[0].cmd = -1;
		NextCommandNumber  = 0;
	}

	OSClearContext(&exceptionContext);
	OSSetCurrentContext(&exceptionContext);

	if (cause) {
		cb       = Callback;
		Callback = NULL;
		if (cb) {
			cb(cause);
		}

		Breaking = FALSE;
	}

	OSClearContext(&exceptionContext);
	OSSetCurrentContext(context);
}

/**
 * @note Address: 0x8016CBEC
 * @note Size: 0x84
 */
static void AlarmHandler(OSAlarm* alarm, OSContext* context)
{
	BOOL error = ProcessNextCommand();
}

/**
 * @note Address: 0x8016CC70
 * @note Size: 0x70
 */
static void AlarmHandlerForTimeout(OSAlarm* alarm, OSContext* context)
{
	OSContext tmpContext;
	DVDLowCallback callback;
	__OSMaskInterrupts(0x400);
	OSClearContext(&tmpContext);
	OSSetCurrentContext(&tmpContext);
	callback = Callback;
	Callback = NULL;
	if (callback != NULL) {
		callback(0x10);
	}
	OSClearContext(&tmpContext);
	OSSetCurrentContext(context);
}

/**
 * @note Address: N/A
 * @note Size: 0x64
 */
static void SetTimeoutAlarm(OSTime timeout)
{
	OSCreateAlarm(&AlarmForTimeout);
	OSSetAlarm(&AlarmForTimeout, timeout, AlarmHandlerForTimeout);
}

/**
 * @note Address: 0x8016CCE0
 * @note Size: 0x110
 */
static void Read(void* addr, u32 length, u32 offset, DVDLowCallback callback)
{
	StopAtNextInt      = FALSE;
	LastCommandWasRead = TRUE;
	Callback           = callback;
	LastReadIssued     = __OSGetSystemTime();

	__DIRegs[DI_CMD_BUF_0]    = 0xa8000000;
	__DIRegs[DI_CMD_BUF_1]    = offset / 4;
	__DIRegs[DI_CMD_BUF_2]    = length;
	__DIRegs[DI_DMA_MEM_ADDR] = (u32)addr;
	__DIRegs[DI_DMA_LENGTH]   = length;
	LastLength                = length;
	__DIRegs[DI_CONTROL]      = 3;

	if (length > 0xa00000) {
		SetTimeoutAlarm(OSSecondsToTicks(20));
	} else {
		SetTimeoutAlarm(OSSecondsToTicks(10));
	}
}

/**
 * @note Address: N/A
 * @note Size: 0xA0
 */
BOOL HitCache(DVDBuffer* cur, DVDBuffer* prev)
{
	u32 uVar1 = (prev->offset + prev->length - 1) >> 15;
	u32 uVar2 = (cur->offset >> 15);
	u32 iVar3 = (DVDGetCurrentDiskID()->streaming ? TRUE : FALSE) ? 5 : 15;

	if ((uVar2 > uVar1 - 2) || (uVar2 < uVar1 + iVar3 + 3)) {
		return TRUE;
	}
	return FALSE;
}

/**
 * @note Address: N/A
 * @note Size: 0x34
 */
static void DoJustRead(void* addr, u32 length, u32 offset, DVDLowCallback callback)
{
	CommandList[0].cmd = -1;
	NextCommandNumber  = 0;
	Read(addr, length, offset, callback);
}

/**
 * @note Address: 0x8016CDF0
 * @note Size: 0x80
 */
static void SeekTwiceBeforeRead(void* addr, u32 length, u32 offset, DVDLowCallback callback)
{
	u32 newOffset = offset & ~0x7FFF;
	if (!newOffset) {
		newOffset = 0;
	} else {
		newOffset += WorkAroundSeekLocation;
	}
	CommandList[0].cmd      = 2;
	CommandList[0].offset   = newOffset;
	CommandList[0].callback = callback;
	CommandList[1].cmd      = 1;
	CommandList[1].addr     = addr;
	CommandList[1].length   = length;
	CommandList[1].offset   = offset;
	CommandList[1].callback = callback;
	CommandList[2].cmd      = -1;
	NextCommandNumber       = 0;
	DVDLowSeek(newOffset, callback);
}

/**
 * @note Address: N/A
 * @note Size: 0x8C
 */
static void WaitBeforeRead(void* addr, u32 length, u32 offset, DVDLowCallback callback, OSTime timeout)
{
	CommandList[0].cmd      = 1;
	CommandList[0].addr     = addr;
	CommandList[0].length   = length;
	CommandList[0].offset   = offset;
	CommandList[0].callback = callback;
	CommandList[1].cmd      = -1;
	NextCommandNumber       = 0;
	OSCreateAlarm(&AlarmForWA);
	OSSetAlarm(&AlarmForWA, timeout, AlarmHandler);
}

/**
 * @note Address: 0x8016CE70
 * @note Size: 0x298
 */
WEAK BOOL DVDLowRead(void* addr, u32 length, u32 offset, DVDLowCallback callback)
{
	OSTime diff;
	u32 prev;

	__DIRegs[DI_DMA_LENGTH] = length;
	Curr.addr               = addr;
	Curr.length             = length;
	Curr.offset             = offset;

	if (WorkAroundType == 0) {
		DoJustRead(addr, length, offset, callback);
	} else if (WorkAroundType == 1) {
		if (FirstRead) {
			SeekTwiceBeforeRead(addr, length, offset, callback);
		} else {
			if (!HitCache(&Curr, &Prev)) {
				DoJustRead(addr, length, offset, callback);
			} else {
				prev = (Prev.offset + Prev.length - 1) >> 15;
				if (prev == Curr.offset >> 15 || prev + 1 == Curr.offset >> 15) {
					diff = __OSGetSystemTime() - LastReadFinished;
					if (OSMillisecondsToTicks(5) < diff) {
						DoJustRead(addr, length, offset, callback);
					} else {
						WaitBeforeRead(addr, length, offset, callback, OSMillisecondsToTicks(5) - diff + OSMicrosecondsToTicks(500));
					}
				} else {
					SeekTwiceBeforeRead(addr, length, offset, callback);
				}
			}
		}
	}
	return TRUE;
}

/**
 * @note Address: 0x8016D108
 * @note Size: 0x94
 */
WEAK BOOL DVDLowSeek(u32 offset, DVDLowCallback callback)
{
	StopAtNextInt          = FALSE;
	Callback               = callback;
	__DIRegs[DI_CMD_BUF_0] = 0xab000000;
	__DIRegs[DI_CMD_BUF_1] = offset / 4;
	__DIRegs[DI_CONTROL]   = 1;
	SetTimeoutAlarm(OSSecondsToTicks(10));
	return TRUE;
}

/**
 * @note Address: 0x8016D19C
 * @note Size: 0x2C
 */
WEAK BOOL DVDLowWaitCoverClose(DVDLowCallback callback)
{
	Callback                  = callback;
	WaitingCoverClose         = TRUE;
	StopAtNextInt             = FALSE;
	__DIRegs[DI_COVER_STATUS] = 2;
	return TRUE;
}

/**
 * @note Address: 0x8016D1C8
 * @note Size: 0xA4
 */
WEAK BOOL DVDLowReadDiskID(DVDDiskID* diskID, DVDLowCallback callback)
{
	StopAtNextInt             = FALSE;
	Callback                  = callback;
	__DIRegs[DI_CMD_BUF_0]    = 0xa8000040;
	__DIRegs[DI_CMD_BUF_1]    = 0;
	__DIRegs[DI_CMD_BUF_2]    = sizeof(DVDDiskID);
	__DIRegs[DI_DMA_MEM_ADDR] = (u32)diskID;
	__DIRegs[DI_DMA_LENGTH]   = sizeof(DVDDiskID);
	__DIRegs[DI_CONTROL]      = 3;
	SetTimeoutAlarm(OSSecondsToTicks(10));
	return TRUE;
}

/**
 * @note Address: 0x8016D26C
 * @note Size: 0x8C
 */
WEAK BOOL DVDLowStopMotor(DVDLowCallback callback)
{
	StopAtNextInt          = FALSE;
	Callback               = callback;
	__DIRegs[DI_CMD_BUF_0] = 0xe3000000;
	__DIRegs[DI_CONTROL]   = 1;
	SetTimeoutAlarm(OSSecondsToTicks(10));
	return TRUE;
}

/**
 * @note Address: 0x8016D2F8
 * @note Size: 0x8C
 */
WEAK BOOL DVDLowRequestError(DVDLowCallback callback)
{
	StopAtNextInt          = FALSE;
	Callback               = callback;
	__DIRegs[DI_CMD_BUF_0] = 0xe0000000;
	__DIRegs[DI_CONTROL]   = 1;
	SetTimeoutAlarm(OSSecondsToTicks(10));
	return TRUE;
}

/**
 * @note Address: 0x8016D384
 * @note Size: 0x9C
 */
WEAK BOOL DVDLowInquiry(DVDDriveInfo* info, DVDLowCallback callback)
{
	StopAtNextInt             = FALSE;
	Callback                  = callback;
	__DIRegs[DI_CMD_BUF_0]    = 0x12000000;
	__DIRegs[DI_CMD_BUF_2]    = sizeof(DVDDriveInfo);
	__DIRegs[DI_DMA_MEM_ADDR] = (u32)info;
	__DIRegs[DI_DMA_LENGTH]   = sizeof(DVDDriveInfo);
	__DIRegs[DI_CONTROL]      = 3;
	SetTimeoutAlarm(OSSecondsToTicks(10));
	return TRUE;
}

/**
 * @note Address: 0x8016D420
 * @note Size: 0x98
 */
WEAK BOOL DVDLowAudioStream(u32 subcmd, u32 length, u32 offset, DVDLowCallback callback)
{
	StopAtNextInt          = FALSE;
	Callback               = callback;
	__DIRegs[DI_CMD_BUF_0] = subcmd | 0xe1000000;
	__DIRegs[DI_CMD_BUF_1] = offset >> 2;
	__DIRegs[DI_CMD_BUF_2] = length;
	__DIRegs[DI_CONTROL]   = 1;
	SetTimeoutAlarm(OSSecondsToTicks(10));
	return TRUE;
}

/**
 * @note Address: 0x8016D4B8
 * @note Size: 0x8C
 */
WEAK BOOL DVDLowRequestAudioStatus(u32 subcmd, DVDLowCallback callback)
{
	StopAtNextInt          = FALSE;
	Callback               = callback;
	__DIRegs[DI_CMD_BUF_0] = subcmd | 0xe2000000;
	__DIRegs[DI_CONTROL]   = 1;
	SetTimeoutAlarm(OSSecondsToTicks(10));
	return TRUE;
}

/**
 * @note Address: 0x8016D544
 * @note Size: 0x9C
 */
WEAK BOOL DVDLowAudioBufferConfig(BOOL enable, u32 size, DVDLowCallback callback)
{
	StopAtNextInt          = FALSE;
	Callback               = callback;
	__DIRegs[DI_CMD_BUF_0] = 0xe4000000 | (enable != 0 ? 0x10000 : 0) | size;
	__DIRegs[DI_CONTROL]   = 1;
	SetTimeoutAlarm(OSSecondsToTicks(10));
	return TRUE;
}

/**
 * @note Address: 0x8016D5E0
 * @note Size: 0xBC
 */
WEAK void DVDLowReset()
{
	u32 reg;
	OSTime resetStart;

	__DIRegs[DI_COVER_STATUS] = 2;
	reg                       = __PIRegs[PI_RESETCODE];
	__PIRegs[PI_RESETCODE]    = (reg & ~4) | 1;

	resetStart = __OSGetSystemTime();
	while ((__OSGetSystemTime() - resetStart) < OSMicrosecondsToTicks(12))
		;

	__PIRegs[PI_RESETCODE] = reg | 5;
	ResetOccurred          = TRUE;
	LastResetEnd           = __OSGetSystemTime();
}

/**
 * @note Address: 0x8016D69C
 * @note Size: 0x14
 */
WEAK BOOL DVDLowBreak()
{
	StopAtNextInt = TRUE;
	Breaking      = TRUE;
	return TRUE;
}

/**
 * @note Address: 0x8016D6B0
 * @note Size: 0x1C
 */
WEAK DVDLowCallback DVDLowClearCallback()
{
	DVDLowCallback old;
	__DIRegs[DI_COVER_STATUS] = 0;
	old                       = Callback;
	Callback                  = NULL;
	WaitingCoverClose         = FALSE;
	return old;
}

/**
 * @note Address: 0x8016D6CC
 * @note Size: 0x44
 */

WEAK void __DVDLowSetWAType(u32 type, s32 seekLoc)
{
	BOOL enabled;
	enabled                = OSDisableInterrupts();
	WorkAroundType         = type;
	WorkAroundSeekLocation = seekLoc;
	OSRestoreInterrupts(enabled);
}

/**
 * @note Address: 0x8016D710
 * @note Size: 0x38
 */
BOOL __DVDLowTestAlarm(const OSAlarm* alarm)
{
	if (alarm == &AlarmForBreak) {
		return TRUE;
	} else if (alarm == &AlarmForTimeout) {
		return TRUE;
	} else {
		return FALSE;
	}
}
