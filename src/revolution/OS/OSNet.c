#include "revolution/hw_regs.h"
#include "revolution/ipc.h"
#include "revolution/os/OSReset.h"

#include "Dolphin/os.h"

typedef struct OSIOSRev {
	u8 reserved;
	u8 major;
	u8 minor;
	u8 micro;
	u8 month;
	u8 date;
	u16 year;
} OSIOSRev;

// other files of the library
void __OSGetIOSRev(OSIOSRev* rev);
u32 SCCheckStatus(void);
u32 SCGetCounterBias(void);

static OSShutdownFunctionInfo ShutdownFuncInfo;
static s32 nwc24ShtSuspendResult[8] ATTRIBUTE_ALIGN(32);
static s32 nwc24ShtRequestParam[8] ATTRIBUTE_ALIGN(32);
static s32 nwc24ShtRequestResult[8] ATTRIBUTE_ALIGN(32);
static s32 nwc24TimeParam[8] ATTRIBUTE_ALIGN(32);
static s32 nwc24TimeResult[8] ATTRIBUTE_ALIGN(32);

static s32 nwc24ShtFd = -1;
static s32 nwc24ShtResult;
static BOOL nwc24ShtRequested;
static BOOL nwc24ShtPending;
static s32 nwc24ShtRetryRest;

s32 NWC24iPrepareShutdown(void);
s32 NWC24iSynchronizeRtcCounter(BOOL save);
s32 NWC24SuspendScheduler(void);
static BOOL NWC24Shutdown_(BOOL final, u32 event);
static s32 NWC24iSetRtcCounter_(u32 rtc, u32 flags);
static s32 CallbackAsyncIpc(s32 result, void* arg);

// Every helper takes the name of its caller. Nothing reads it in the retail build, but the names are in
// .data, which is what a (void) use of the parameter leaves behind.
static inline s32 NWC24iIsAsyncRequestPending_(const char* funcName)
{
	(void)funcName;

	if (!OSGetCurrentThread()) {
		return -1;
	}

	return 0;
}

static inline s32 NWC24iOpenResourceManager_(const char* funcName, const char* path, s32* fd, u32 mode)
{
	(void)funcName;

	if (!fd) {
		return -3;
	}

	*fd = IOS_Open(path, mode);
	if (*fd < 0) {
		if (*fd == IPC_RESULT_NOENT) {
			return -29;
		}
		return -42;
	}

	return 0;
}

static inline s32 NWC24iCloseResourceManager_(const char* funcName, s32 fd)
{
	(void)funcName;

	if (IOS_Close(fd) < 0) {
		return -42;
	}

	return 0;
}

static inline s32 NWC24iIoctlResourceManager_(const char* funcName, s32 fd, s32 cmd, void* in, u32 inLen, void* out, u32 outLen)
{
	(void)funcName;

	if (IOS_Ioctl(fd, cmd, in, inLen, out, outLen) < 0) {
		return -42;
	}

	return 0;
}

static inline s32 NWC24iIoctlResourceManagerAsync_(const char* funcName, s32 fd, s32 cmd, void* in, u32 inLen, void* out, u32 outLen,
                                                   void* arg)
{
	(void)funcName;

	if (IOS_IoctlAsync(fd, cmd, in, inLen, out, outLen, CallbackAsyncIpc, arg) < 0) {
		return -42;
	}

	nwc24ShtPending = TRUE;
	return 0;
}

void __OSInitNet(void)
{
	OSIOSRev rev;
	s32 err;

	__OSGetIOSRev(&rev);
	if (rev.major <= 4 || rev.major == 9) {
		return;
	}

	err = NWC24iPrepareShutdown();
	if (err != 0) {
		if (err < 0) {
			OSReport("Failed to register network shutdown function. %d\n", err);
		}

		err = NWC24SuspendScheduler();
		if (err < 0) {
			OSReport("Failed to suspend the WiiConnect24 scheduler. %d\n", err);
		}
	}

	if (!__OSInIPL) {
		err = NWC24iSynchronizeRtcCounter(FALSE);
		if (err != 0) {
			OSReport("Failed to synchronize time with network resource managers. %d\n", err);
		}
	}
}

s32 NWC24iPrepareShutdown(void)
{
	s32 err = 0;

	ShutdownFuncInfo.func     = NWC24Shutdown_;
	ShutdownFuncInfo.priority = 110;
	OSRegisterShutdownFunction(&ShutdownFuncInfo);

	if (nwc24ShtFd < 0) {
		err = NWC24iOpenResourceManager_(__FUNCTION__, "/dev/net/kd/request", &nwc24ShtFd, 1);
	}

	nwc24ShtRetryRest = 5;

	if (err == 0) {
		err = 1;
	}

	return err;
}

static inline s32 GetRTC(u32* rtc)
{
	u32 status;
	u32 bias;

	do {
		status = SCCheckStatus();
		if (status == 2) {
			return -1;
		}
	} while (status != 0);

	bias = SCGetCounterBias();
	*rtc = OSTicksToSeconds(OSGetTime()) - bias;
	return 0;
}

s32 NWC24iSynchronizeRtcCounter(BOOL save)
{
	u32 rtc;
	s32 err;

	err = GetRTC(&rtc);
	if (err != 0) {
		return err;
	}

	return NWC24iSetRtcCounter_(rtc, save ? 1 : 0);
}

s32 NWC24SuspendScheduler(void)
{
	s32 err;
	s32 fd;
	s32 closeErr;

	err = NWC24iIsAsyncRequestPending_(__FUNCTION__);
	if (err < 0) {
		return err;
	}

	err = NWC24iOpenResourceManager_(__FUNCTION__, "/dev/net/kd/request", &fd, 0);
	if (err >= 0) {
		err = NWC24iIoctlResourceManager_(__FUNCTION__, fd, 1, NULL, 0, nwc24ShtSuspendResult, sizeof(nwc24ShtSuspendResult));
		if (err >= 0) {
			err = nwc24ShtSuspendResult[0];
		}

		closeErr = NWC24iCloseResourceManager_(__FUNCTION__, fd);
		if (closeErr < 0) {
			err = closeErr;
		}
	}

	return err;
}

// Not in the retail dol (nothing calls it, the linker dropped it); only its name is left in .data
// (0x8031D0AC), so the body is the counterpart of NWC24SuspendScheduler by assumption.
s32 NWC24ResumeScheduler(void)
{
	s32 err;
	s32 fd;
	s32 closeErr;

	err = NWC24iIsAsyncRequestPending_(__FUNCTION__);
	if (err < 0) {
		return err;
	}

	err = NWC24iOpenResourceManager_(__FUNCTION__, "/dev/net/kd/request", &fd, 0);
	if (err >= 0) {
		err = NWC24iIoctlResourceManager_(__FUNCTION__, fd, 3, NULL, 0, nwc24ShtSuspendResult, sizeof(nwc24ShtSuspendResult));
		if (err >= 0) {
			err = nwc24ShtSuspendResult[0];
		}

		closeErr = NWC24iCloseResourceManager_(__FUNCTION__, fd);
		if (closeErr < 0) {
			err = closeErr;
		}
	}

	return err;
}

static s32 NWC24iRequestShutdown(u32 event, s32* result)
{
	nwc24ShtRequestParam[0] = event;
	return NWC24iIoctlResourceManagerAsync_(__FUNCTION__, nwc24ShtFd, 0x28, nwc24ShtRequestParam, sizeof(nwc24ShtRequestParam),
	                                        nwc24ShtRequestResult, sizeof(nwc24ShtRequestResult), result);
}

static BOOL NWC24Shutdown_(BOOL final, u32 event)
{
	if (final) {
		return TRUE;
	}

	if (nwc24ShtRequested) {
		if (nwc24ShtPending) {
			return FALSE;
		}

		if (nwc24ShtResult >= 0) {
			return TRUE;
		}

		if (nwc24ShtRetryRest > 0) {
			nwc24ShtRequested = FALSE;
			nwc24ShtRetryRest--;
		} else {
			OSReport("NWC24Shutdown_: Give up!\n");
			return TRUE;
		}
	} else if (NWC24iRequestShutdown(event, &nwc24ShtResult) >= 0) {
		nwc24ShtRequested = TRUE;
	}

	return FALSE;
}

static s32 NWC24iSetRtcCounter_(u32 rtc, u32 flags)
{
	s32 err;
	s32 fd;
	s32 closeErr;

	err = NWC24iIsAsyncRequestPending_(__FUNCTION__);
	if (err < 0) {
		return err;
	}

	err = NWC24iOpenResourceManager_(__FUNCTION__, "/dev/net/kd/time", &fd, 0);
	if (err >= 0) {
		nwc24TimeParam[0] = rtc;
		nwc24TimeParam[1] = flags;
		err               = NWC24iIoctlResourceManager_(__FUNCTION__, fd, 0x17, nwc24TimeParam, sizeof(nwc24TimeParam), nwc24TimeResult,
		                                                sizeof(nwc24TimeResult));
		if (err >= 0) {
			err = nwc24TimeResult[0];
		}

		closeErr = NWC24iCloseResourceManager_(__FUNCTION__, fd);
		if (err >= 0) {
			err = closeErr;
		}
	}

	return err;
}

static s32 CallbackAsyncIpc(s32 result, void* arg)
{
	if (arg != NULL) {
		*(s32*)arg = result;
	}

	nwc24ShtPending = FALSE;
	return 0;
}
