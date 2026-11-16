/* What WPAD uses from WUD and SC, kept minimal until those libraries bring their own headers. */
#ifndef REVOLUTION_WPAD_EXTERN_H
#define REVOLUTION_WPAD_EXTERN_H

#include "types.h"
#include "revolution/BTE/bta_api.h"
#include "revolution/BTE/bta_hh_api.h"

#define WUD_MAX_DEV_ENTRY 16

typedef void* (*WUDAllocFunc)(u32 size);
typedef BOOL (*WUDFreeFunc)(void* ptr);
typedef void (*WUDSyncDeviceCallback)(s32 result, s32 num);
typedef void (*WUDClearDeviceCallback)(s32 result);

typedef struct WUDDevInfo {
	u8 conf[0x40];
	BD_ADDR devAddr;
	LINK_KEY linkKey;
	u8 devHandle;
	u8 subclass;
	u8 appID;
	u8 status;
	s8 UNK_0x5A;
	u8 sync_type;
	u8 UNK_0x5C;
	u8 UNK_0x5D;
	tBTA_HH_ATTR_MASK hhAttrMask;
} WUDDevInfo;

typedef void (*WUDHidConnCallback)(WUDDevInfo* info, u8 open);
typedef void (*WUDHidRecvCallback)(u8 dev_handle, u8* report, u16 len);

BOOL WUDInit(void);
BOOL WUDIsBusy(void);
void WUDRegisterAllocator(WUDAllocFunc alloc, WUDFreeFunc free);
void WUDShutdown(BOOL exec);
s32 WUDGetStatus(void);
u8 WUDGetBufferStatus(void);
WUDSyncDeviceCallback WUDSetSyncSimpleCallback(WUDSyncDeviceCallback callback);
WUDSyncDeviceCallback WUDSetSyncDeviceCallback(WUDSyncDeviceCallback callback);
WUDClearDeviceCallback WUDSetClearDeviceCallback(WUDClearDeviceCallback callback);
u32 WUDGetAllocatedMemSize(void);
BOOL WUDStartFastSyncSimple(void);
BOOL WUDCancelSyncDevice(void);
BOOL WUDStopSyncSimple(void);
BOOL WUDSetDisableChannel(s8 afhChannel);
WUDHidRecvCallback WUDSetHidRecvCallback(WUDHidRecvCallback callback);
WUDHidConnCallback WUDSetHidConnCallback(WUDHidConnCallback callback);
void WUDSetVisibility(u8 discoverable, u8 connectable);
void WUDSetDeviceHistory(s32 chan, u8* addr);
BOOL WUDIsLatestDevice(s32 chan, u8* addr);
void WUDUpdateSCSetting(void);
BOOL WUDIsLinkedWBC(void);
u8* _WUDGetDevAddr(u8 handle);
u16 _WUDGetQueuedSize(s8 handle);
u16 _WUDGetNotAckedSize(s8 handle);
u8 _WUDGetLinkNumber(void);

u32 SCCheckStatus(void);
u32 SCGetBtDpdSensibility(void);
u8 SCGetWpadSensorBarPosition(void);
u8 SCGetWpadMotorMode(void);
u8 SCGetWpadSpeakerVolume(void);
BOOL SCSetWpadSpeakerVolume(u8 volume);
BOOL SCSetWpadMotorMode(u8 mode);
void SCFlushAsync(void (*callback)(u32 status));

const char* OSGetAppGamename(void);
u8 OSGetAppType(void);
BOOL __VIResetRFIdle(void);
void btm_remove_acl(BD_ADDR bd_addr);
void* GKI_getbuf(UINT16 size);

#endif
