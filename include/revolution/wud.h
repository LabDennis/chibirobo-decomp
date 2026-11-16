/* Wii remote device manager (WUD), after wii-ipl's reconstruction (CC0), adapted to the September 2008 SDK. */
#ifndef REVOLUTION_WUD_H
#define REVOLUTION_WUD_H

#include "types.h"

#include "revolution/BTE/bt_types.h"
#include "revolution/BTE/bta_hh_api.h"

#include "revolution/sc.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WUD_MAX_DEV_ENTRY_FOR_STD SC_MAX_DEV_ENTRY_FOR_STD
#define WUD_MAX_DEV_ENTRY_FOR_SMP SC_MAX_DEV_ENTRY_FOR_SMP
#define WUD_MAX_DEV_ENTRY SC_MAX_DEV_ENTRY
#define WUD_DEV_HANDLE_INVALID (-1)
#define WUD_ADDR_LEN BD_ADDR_LEN

typedef struct WUDDevInfo WUDDevInfo;

typedef enum {
    WUD_LIB_STATUS_0,
    WUD_LIB_STATUS_1,
    WUD_LIB_STATUS_2,
    WUD_LIB_STATUS_3,
    WUD_LIB_STATUS_4,
    WUD_LIB_STATUS_5,
} WUDLibStatus;

typedef enum {
    WUD_CHAN0,
    WUD_CHAN1,
    WUD_CHAN2,
    WUD_CHAN3,

    WUD_MAX_CHANNELS,
    WUD_CHAN_INVALID = -1
} WUDChannel;

typedef enum {
    WUD_SYNC_TYPE_STANDARD,
    WUD_SYNC_TYPE_SIMPLE,
} WUDSyncType;

typedef enum {
    WUD_RESULT_SYNC_BUSY = -1,
    WUD_RESULT_SYNC_WAITING,
    WUD_RESULT_SYNC_DONE,
} WUDSyncResult;

typedef void* (*WUDAllocFunc)(u32 size);
typedef BOOL (*WUDFreeFunc)(void* pBlock);

typedef void (*WUDSyncDeviceCallback)(s32 result, s32 num);
typedef void (*WUDClearDeviceCallback)(s32 result);

typedef void (*WUDHidConnCallback)(WUDDevInfo* devHandle, u8 open);
typedef void (*WUDHidRecvCallback)(UINT8 devHandle, UINT8* pReport, UINT16 len);

struct WUDDevInfo {
    SC_BT_DEV_INFO conf;  // 0x00

    BD_ADDR devAddr;   // 0x40
    LINK_KEY linkKey;  // 0x46
    UINT8 devHandle;   // 0x56
    UINT8 subclass;    // 0x57
    UINT8 appID;       // 0x58
    u8 status;         // 0x59

    s8 UNK_0x5A;                   // 0x5A
    u8 UNK_0x5B;                   // 0x5B
    u8 UNK_0x5C;                   // 0x5C
    tBTA_HH_ATTR_MASK hhAttrMask;  // 0x5E
};


BOOL WUDIsLinkedWBC(void);

BOOL WUDInit(void);
BOOL WUDIsBusy(void);
void WUDRegisterAllocator(WUDAllocFunc pAllocFunc, WUDFreeFunc pFreeFunc);
void WUDShutdown(BOOL saveSmp);

WUDLibStatus WUDGetStatus(void);
u8 WUDGetBufferStatus(void);

void WUDSetSniffMode(BD_ADDR addr, s32 interval);

WUDSyncDeviceCallback WUDSetSyncSimpleCallback(WUDSyncDeviceCallback pCallback);
WUDClearDeviceCallback WUDSetClearDeviceCallback(WUDClearDeviceCallback pCallback);

BOOL WUDStartSyncDevice(void);
BOOL WUDStartFastSyncSimple(void);
BOOL WUDStartSyncSpDevice(u8 target);
BOOL WUDCancelSyncDevice(void);
BOOL WUDStopSyncSimple(void);

BOOL WUDStartClearDevice(void);
BOOL WUDSetDisableChannel(s8 afhChannel);

WUDHidRecvCallback WUDSetHidRecvCallback(WUDHidRecvCallback pCallback);
WUDHidConnCallback WUDSetHidConnCallback(WUDHidConnCallback pCallback);

void WUDSetVisibility(u8 disc, u8 conn);
u8 WUDGetConnectable(void);

void WUDSetDeviceHistory(WUDChannel chan, BD_ADDR addr);
BOOL WUDIsLatestDevice(WUDChannel chan, BD_ADDR addr);
void WUDUpdateSCSetting(void);

void WUDiRegisterDevice(BD_ADDR addr);
void WUDiRemoveDevice(BD_ADDR addr);

WUDDevInfo* WUDiGetDevInfo(BD_ADDR addr);
WUDDevInfo* WUDiGetDiscoverDevice(void);

void WUDiMoveTopSmpDevInfoPtr(WUDDevInfo* pInfo);
void WUDiMoveBottomSmpDevInfoPtr(WUDDevInfo* pInfo);
void WUDiMoveTopOfDisconnectedSmpDevice(WUDDevInfo* pInfo);

void WUDiMoveTopStdDevInfoPtr(WUDDevInfo* pInfo);
void WUDiMoveBottomStdDevInfoPtr(WUDDevInfo* pInfo);
void WUDiMoveTopOfDisconnectedStdDevice(WUDDevInfo* pInfo);
void WUDiMoveTopOfUnusedStdDevice(WUDDevInfo* pInfo);

void WUDiSetDevAddrForHandle(UINT8 handle, BD_ADDR addr);
BD_ADDR_PTR WUDiGetDevAddrForHandle(UINT8 handle);
void WUDiSetQueueSizeForHandle(UINT8 handle, u16 size);
u16 WUDiGetQueueSizeForHandle(UINT8 handle);
void WUDiSetNotAckNumForHandle(UINT8 handle, u16 num);
u16 WUDiGetNotAckNumForHandle(UINT8 handle);

BD_ADDR_PTR _WUDGetDevAddr(UINT8 handle);
u16 _WUDGetQueuedSize(s8 handle);
u16 _WUDGetNotAckedSize(s8 handle);
u8 _WUDGetLinkNumber(void);

#ifdef __cplusplus
}
#endif

#endif // REVOLUTION_WUD_H
