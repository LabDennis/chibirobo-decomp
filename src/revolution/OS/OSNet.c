#include "revolution/os.h"
#include "revolution/ipc.h"
#include "revolution/os/OSReset.h"
#include "revolution/sc.h"

typedef struct OSIOSRev {
	u8 reserved;
	u8 major;
	u8 minor;
	u8 micro;
	u8 month;
	u8 date;
	u16 year;
} OSIOSRev;

void __OSGetIOSRev(OSIOSRev* rev);

// The helper layout follows the Revolution SDK reconstructions in xenoblade/rb3
// and tp: the IPC buffers are static to their users, the shutdown state is
// static to NWC24Shutdown_, and the caller check is separate from the async
// request flag. The decomp-only macros from those reconstructions are not part
// of the SDK source.
// xenoblade: https://github.com/atsorensen/xenoblade/blob/master/libs/RVL_SDK/src/revolution/os/OSNet.c
// tp: https://github.com/zeldaret/tp/blob/main/libs/revolution/src/os/OSNet.c

static OSShutdownFunctionInfo ShutdownFuncInfo;
static IOSFd nwc24ShtFd = -1;
static s32 nwc24ShtRetryRest;
static BOOL NWC24iIsRequestPending;

s32 NWC24iPrepareShutdown(void);
s32 NWC24iSynchronizeRtcCounter(BOOL save);
s32 NWC24SuspendScheduler(void);
static BOOL NWC24Shutdown_(BOOL final, u32 event);
static s32 NWC24iSetRtcCounter_(u32 rtc, u32 flags);
static inline s32 NWC24iOpenResourceManager_(const char* funcName, const char* path, IOSFd* fd, u32 mode);
static inline s32 NWC24iCloseResourceManager_(const char* funcName, IOSFd fd);
static inline s32 NWC24iIoctlResourceManager_(const char* funcName, IOSFd fd, s32 cmd, void* input, u32 inputLen, void* output,
                                       u32 outputLen);
static inline s32 NWC24iIoctlResourceManagerAsync_(const char* funcName, IOSFd fd, s32 cmd, void* input, u32 inputLen,
                                            void* output, u32 outputLen, void* arg);
static inline BOOL NWC24iIsAsyncRequestPending_(void);
static inline s32 CheckCallingStatus(const char* funcName);
static IOSError CallbackAsyncIpc(IOSError result, void* arg);

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

static inline s32 GetRTC(u32* rtc);

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

// NONMATCHING: retail has no fd==0 branch for a local descriptor; even xenoblade's
// nested SDK form keeps that check here because the helper emits it for the global
// shutdown descriptor. The strict diff also keeps the extracted string-object boundary
// for "/dev/net/kd/request" and the following function-name strings.
s32 NWC24SuspendScheduler(void)
{
	static s32 susResult[8] ATTRIBUTE_ALIGN(32);
	s32 err;
	s32 closeErr;
	IOSFd fd;

	err = CheckCallingStatus(__FUNCTION__);
	if (err >= 0) {
		err = NWC24iOpenResourceManager_(__FUNCTION__, "/dev/net/kd/request", &fd, 0);
		if (err >= 0) {
			err = NWC24iIoctlResourceManager_(__FUNCTION__, fd, 1, NULL, 0, susResult, sizeof(susResult));
			if (err >= 0) {
				err = susResult[0];
			}

			closeErr = NWC24iCloseResourceManager_(__FUNCTION__, fd);
			if (closeErr < 0) {
				err = closeErr;
			}
		}
	}

	return err;
}

s32 NWC24ResumeScheduler(void)
{
	static s32 susResult[8] ATTRIBUTE_ALIGN(32);
	s32 err;
	s32 closeErr;
	IOSFd fd;

	err = CheckCallingStatus(__FUNCTION__);
	if (err < 0) {
		return err;
	}

	err = NWC24iOpenResourceManager_(__FUNCTION__, "/dev/net/kd/request", &fd, 0);
	if (err >= 0) {
		err = NWC24iIoctlResourceManager_(__FUNCTION__, fd, 3, NULL, 0, susResult, sizeof(susResult));
		if (err >= 0) {
			err = susResult[0];
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
	static u32 shtBuffer[8] ATTRIBUTE_ALIGN(32);
	static u32 shtResult[8] ATTRIBUTE_ALIGN(32);

	shtBuffer[0] = event;
	return NWC24iIoctlResourceManagerAsync_(__FUNCTION__, nwc24ShtFd, 0x28, shtBuffer, sizeof(shtBuffer), shtResult,
	                                        sizeof(shtResult), result);
}

static BOOL NWC24Shutdown_(BOOL final, u32 event)
{
	static BOOL shuttingdown;
	static s32 result;

	if (final) {
		return TRUE;
	}

	if (shuttingdown) {
		if (NWC24iIsAsyncRequestPending_()) {
			return FALSE;
		}

		if (result >= 0) {
			return TRUE;
		}

		if (nwc24ShtRetryRest > 0) {
			shuttingdown = FALSE;
			nwc24ShtRetryRest--;
		} else {
			OSReport("NWC24Shutdown_: Give up!\n");
			return TRUE;
		}
	} else if (NWC24iRequestShutdown(event, &result) >= 0) {
		shuttingdown = TRUE;
	}

	return FALSE;
}

static u32 nwc24TimeCommonBuffer[8] ATTRIBUTE_ALIGN(32);
static u32 nwc24TimeCommonResult[8] ATTRIBUTE_ALIGN(32);

// NONMATCHING: the same honest resource-helper form leaves the local descriptor in a
// stack slot and emits the helper's fd==0 check here; retail keeps the descriptor in r30.
// Removing that check makes this function instruction-identical, but removes the check
// that retail does emit in NWC24iPrepareShutdown for the global shutdown descriptor.
// Temporary open results, a NULL comparison, moving the helper before its callers and
// initializing fd do not make this compiler scalarize the local descriptor. The rest is
// register allocation for rtc/flags plus the object-local string boundary.
static s32 NWC24iSetRtcCounter_(u32 rtc, u32 flags)
{
	s32 err;
	s32 closeErr;
	IOSFd fd;

	err = CheckCallingStatus(__FUNCTION__);
	if (err < 0) {
		return err;
	}

	err = NWC24iOpenResourceManager_(__FUNCTION__, "/dev/net/kd/time", &fd, 0);
	if (err >= 0) {
		nwc24TimeCommonBuffer[0] = rtc;
		nwc24TimeCommonBuffer[1] = flags;
		err = NWC24iIoctlResourceManager_(__FUNCTION__, fd, 0x17, nwc24TimeCommonBuffer, sizeof(nwc24TimeCommonBuffer),
		                                  nwc24TimeCommonResult, sizeof(nwc24TimeCommonResult));
		if (err >= 0) {
			err = nwc24TimeCommonResult[0];
		}

		closeErr = NWC24iCloseResourceManager_(__FUNCTION__, fd);
		if (err >= 0) {
			err = closeErr;
		}
	}

	return err;
}

static inline s32 NWC24iOpenResourceManager_(const char* funcName, const char* path, IOSFd* fd, u32 mode)
{
	(void)funcName;

	if (fd == 0) {
		return -3;
	}

	*fd = IOS_Open(path, mode);
	if (*fd < 0) {
		if (*fd == IOS_ERROR_NOEXISTS) {
			return -29;
		}
		return -42;
	}

	return 0;
}

static inline s32 NWC24iCloseResourceManager_(const char* funcName, IOSFd fd)
{
	(void)funcName;

	if (IOS_Close(fd) < 0) {
		return -42;
	}

	return 0;
}

static inline s32 NWC24iCloseResourceManagerAsync_(const char* funcName, IOSFd fd, void* arg)
{
	(void)funcName;

	if (IOS_CloseAsync(fd, CallbackAsyncIpc, arg) < 0) {
		return -42;
	}

	NWC24iIsRequestPending = TRUE;
	return 0;
}

static inline s32 NWC24iIoctlResourceManager_(const char* funcName, IOSFd fd, s32 cmd, void* input, u32 inputLen, void* output,
                                       u32 outputLen)
{
	(void)funcName;

	if (IOS_Ioctl(fd, cmd, input, inputLen, output, outputLen) < 0) {
		return -42;
	}

	return 0;
}

static inline s32 NWC24iIoctlResourceManagerAsync_(const char* funcName, IOSFd fd, s32 cmd, void* input, u32 inputLen,
                                            void* output, u32 outputLen, void* arg)
{
	(void)funcName;

	if (IOS_IoctlAsync(fd, cmd, input, inputLen, output, outputLen, CallbackAsyncIpc, arg) < 0) {
		return -42;
	}

	NWC24iIsRequestPending = TRUE;
	return 0;
}

static inline BOOL NWC24iIsAsyncRequestPending_(void)
{
	return NWC24iIsRequestPending;
}

static IOSError CallbackAsyncIpc(IOSError result, void* arg)
{
	if (arg != NULL) {
		*(IOSError*)arg = result;
	}

	NWC24iIsRequestPending = FALSE;
	return 0;
}

static inline s32 CheckCallingStatus(const char* funcName)
{
	(void)funcName;

	if (OSGetCurrentThread() == 0) {
		return -1;
	}

	return 0;
}

static inline s32 GetRTC(u32* rtc)
{
	OSTime time;
	u32 bias;
	u32 status;

	do {
		status = SCCheckStatus();
		if (status == SC_STATUS_FATAL) {
			return -1;
		}
	} while (status != SC_STATUS_OK);

	bias = SCGetCounterBias();
	time = OSGetTime();
	*rtc = OSTicksToSeconds(time) - bias;
	return 0;
}
