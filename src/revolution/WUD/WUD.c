/* After wii-ipl's reconstruction of WUD.c (CC0), brought to the September 2008 SDK from the disassembly:
 * the balance board (WBC) and Wii Fit save file handling, the debug messages and the firmware patch
 * sequence are this version's. Function and variable names are the SDK's (debug messages, symbol maps). */
#include "revolution/WUD/WUDInternal.h"

#include "revolution/BTE/bt_trace.h"
#include "revolution/BTE/bta_api.h"
#include "revolution/BTE/btm_api.h"
#include "revolution/BTE/btm_int.h"
#include "revolution/BTE/l2c_api.h"
#include "revolution/BTE/misc.h"

#include "revolution/nand.h"

#define DEBUGPrint WUD_DEBUGPrint

// revolution/os.h does not declare these yet
const char* OSGetAppGamename(void);

#define RP_HEALTH_FILE_PATH "/title/00010004/52464e4a/data/RPHealth.dat"
// where Wii Fit keeps the balance board's pairing information in its save file
#define RP_HEALTH_FILE_OFFSET 0x4AF18
#define RP_HEALTH_WBC_INFO_SIZE 128
// the part of the save file that is read to check it, up to the end of the pairing information's block
#define WUD_NAND_BUFFER_SIZE 0x4B000

#define BT_VSC_NINTENDO_SUPER_PEEK_POKE 0xFC0A
#define BT_VSC_NINTENDO_WRITE_PATCH 0xFC4C
#define BT_VSC_NINTENDO_INSTALL_PATCH 0xFC4F

typedef enum {
    WUD_STATE_NAND_OPEN = 1,
    WUD_STATE_NAND_SEEK = 2,
    WUD_STATE_NAND_READ = 3,
    WUD_STATE_NAND_GET_DEV_INFO = 4,
    WUD_STATE_NAND_CLOSE = 5,
    WUD_STATE_NAND_DONE = 6,
} WUDNandState;

// the balance board's entry as Wii Fit stores it, followed by a checksum
typedef struct WUDNandWbcInfo {
    BD_ADDR devAddr;   // 0x00
    LINK_KEY linkKey;  // 0x06
    u8 devName[64];    // 0x16
    u8 UNK_0x56[0x2A];
    u8 crc[4];  // 0x80
    u8 UNK_0x84[0x1C];
} WUDNandWbcInfo;

static void __wudSecurityEventStackCallback(tBTA_DM_SEC_EVT event, tBTA_DM_SEC* pData);
static void __wudSearchEventStackCallback(tBTA_DM_SEARCH_EVT event, tBTA_DM_SEARCH* pData);
static void __wudVendorSpecificEventStackCallback(UINT8 len, UINT8* pData);
static void __wudDeviceStatusEventStackCallback(tBTM_DEV_STATUS status);
static void __wudLinkKeyEventStackCallback(void* p1);
static void __wudPowerMangeEventStackCallback(BD_ADDR addr, tBTM_PM_STATUS status, UINT16 value, UINT8 hciStatus);
static void __wudResetAuthFailCount(void);
static WUDDevInfo* __wudGetWbcDevice(void);

static u32 __wudCalcWiiFitCrc2(void* pData, u32 length, u32 crc);
static void __wudClearControlBlock(void);
static void __wudCheckFirmwareVersion(void);
static void __wudInitSub(void);

static void __wudRemovePatch(void);
static void __wudWritePatch(void);
static void __wudInstallPatch(void);

static WUDDevInfo* __wudGetDevInfoByIndex(int idx);
static WUDDevInfo* __wudGetNewDevInfo(void);
static void __wudRemoveDevInfo(BD_ADDR addr);
static void __wudClearDevInfos(void);
static u8 __wudGetStdDevNumber(void);
static u8 __wudGetSmpDevNumber(void);
static u8 __wudGetConnectionNumber(void);
static u8 __wudGetLinkNumber(void);
static WUDDevInfo* __wudGetOldestSmpDevice(void);
static WUDDevInfo* __wudGetOldestStdDevice(void);

static u8 _wudWiiRemoteDescriptor[] = {
    0x05, 0x01,  // Usage Page (Generic Desktop Ctrls)
    0x09, 0x05,  // Usage (Game Pad)

    0xA1, 0x01,  // Collection (Application)

    0x85, 0x10,        // Report ID (16)
    0x15, 0x00,        // Logical Minimum (0)
    0x26, 0xFF, 0x00,  // Logical Maximum (255)
    0x75, 0x08,        // Report Size (8)
    0x95, 0x01,        // Report Count (1)
    0x06, 0x00, 0xFF,  // Usage Page (Vendor Defined 0xFF00)
    0x09, 0x01,        // Usage (0x01)
    0x91, 0x00,        // Output (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)

    0x85, 0x11,  // Report ID (17)
    0x95, 0x01,  // Report Count (1)
    0x09, 0x01,  // Usage (0x01)
    0x91, 0x00,  // Output (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)

    0x85, 0x12,  // Report ID (18)
    0x95, 0x02,  // Report Count (2)
    0x09, 0x01,  // Usage (0x01)
    0x91, 0x00,  // Output (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)

    0x85, 0x13,  // Report ID (19)
    0x95, 0x01,  // Report Count (1)
    0x09, 0x01,  // Usage (0x01)
    0x91, 0x00,  // Output (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)

    0x85, 0x14,  // Report ID (20)
    0x95, 0x01,  // Report Count (1)
    0x09, 0x01,  // Usage (0x01)
    0x91, 0x00,  // Output (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)

    0x85, 0x15,  // Report ID (21)
    0x95, 0x01,  // Report Count (1)
    0x09, 0x01,  // Usage (0x01)
    0x91, 0x00,  // Output (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)

    0x85, 0x16,  // Report ID (22)
    0x95, 0x15,  // Report Count (21)
    0x09, 0x01,  // Usage (0x01)
    0x91, 0x00,  // Output (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)

    0x85, 0x17,  // Report ID (23)
    0x95, 0x06,  // Report Count (6)
    0x09, 0x01,  // Usage (0x01)
    0x91, 0x00,  // Output (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)

    0x85, 0x18,  // Report ID (24)
    0x95, 0x15,  // Report Count (21)
    0x09, 0x01,  // Usage (0x01)
    0x91, 0x00,  // Output (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)

    0x85, 0x19,  // Report ID (25)
    0x95, 0x01,  // Report Count (1)
    0x09, 0x01,  // Usage (0x01)
    0x91, 0x00,  // Output (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)

    0x85, 0x1A,  // Report ID (26)
    0x95, 0x01,  // Report Count (1)
    0x09, 0x01,  // Usage (0x01)
    0x91, 0x00,  // Output (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)

    0x85, 0x20,  // Report ID (32)
    0x95, 0x06,  // Report Count (6)
    0x09, 0x01,  // Usage (0x01)
    0x81, 0x00,  // Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)

    0x85, 0x21,  // Report ID (33)
    0x95, 0x15,  // Report Count (21)
    0x09, 0x01,  // Usage (0x01)
    0x81, 0x00,  // Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)

    0x85, 0x22,  // Report ID (34)
    0x95, 0x04,  // Report Count (4)
    0x09, 0x01,  // Usage (0x01)
    0x81, 0x00,  // Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)

    0x85, 0x30,  // Report ID (48)
    0x95, 0x02,  // Report Count (2)
    0x09, 0x01,  // Usage (0x01)
    0x81, 0x00,  // Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)

    0x85, 0x31,  // Report ID (49)
    0x95, 0x05,  // Report Count (5)
    0x09, 0x01,  // Usage (0x01)
    0x81, 0x00,  // Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)

    0x85, 0x32,  // Report ID (50)
    0x95, 0x0A,  // Report Count (10)
    0x09, 0x01,  // Usage (0x01)
    0x81, 0x00,  // Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)

    0x85, 0x33,  // Report ID (51)
    0x95, 0x11,  // Report Count (17)
    0x09, 0x01,  // Usage (0x01)
    0x81, 0x00,  // Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)

    0x85, 0x34,  // Report ID (52)
    0x95, 0x15,  // Report Count (21)
    0x09, 0x01,  // Usage (0x01)
    0x81, 0x00,  // Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)

    0x85, 0x35,  // Report ID (53)
    0x95, 0x15,  // Report Count (21)
    0x09, 0x01,  // Usage (0x01)
    0x81, 0x00,  // Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)

    0x85, 0x36,  // Report ID (54)
    0x95, 0x15,  // Report Count (21)
    0x09, 0x01,  // Usage (0x01)
    0x81, 0x00,  // Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)

    0x85, 0x37,  // Report ID (55)
    0x95, 0x15,  // Report Count (21)
    0x09, 0x01,  // Usage (0x01)
    0x81, 0x00,  // Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)

    0x85, 0x3D,  // Report ID (61)
    0x95, 0x15,  // Report Count (21)
    0x09, 0x01,  // Usage (0x01)
    0x81, 0x00,  // Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)

    0x85, 0x3E,  // Report ID (62)
    0x95, 0x15,  // Report Count (21)
    0x09, 0x01,  // Usage (0x01)
    0x81, 0x00,  // Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)

    0x85, 0x3F,  // Report ID (63)
    0x95, 0x15,  // Report Count (21)
    0x09, 0x01,  // Usage (0x01)
    0x81, 0x00,  // Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)

    0xC0,  // End Collection
};

static u8 _wudSuperPeekPokeCmd[] = {0x05, 0x00, 0x9A, 0x0E, 0x00, 0x01, 0x00, 0x00, 0x00};

static u8 _wudPatchData[] = {0x70, 0x99, 0x08, 0x00,  // address (little-endian)
                            0xB4, 0x00, 0x00, 0x00,  // size (little-endian)

                            // patch data
                            0x88, 0x43, 0xD1, 0x07, 0x09, 0x0C, 0x08, 0x43, 0xA0, 0x62, 0x19, 0x23, 0xDB, 0x01, 0x33, 0x80, 0x7C, 0xF7, 0x88, 0xF8,
                            0x28, 0x76, 0x80, 0xF7, 0x17, 0xFF, 0x43, 0x78, 0xEB, 0x70, 0x19, 0x23, 0xDB, 0x01, 0x33, 0x87, 0x7C, 0xF7, 0xBC, 0xFB,
                            0x0B, 0x60, 0xA3, 0x7B, 0x01, 0x49, 0x0B, 0x60, 0x90, 0xF7, 0x96, 0xFB, 0xD8, 0x1D, 0x08, 0x00, 0x00, 0xF0, 0x04, 0xF8,
                            0x00, 0x23, 0x79, 0xF7, 0xE3, 0xFA, 0x00, 0x00, 0x00, 0xB5, 0x00, 0x23, 0x11, 0x49, 0x0B, 0x60, 0x1D, 0x21, 0xC9, 0x03,
                            0x0B, 0x60, 0x7D, 0x20, 0x80, 0x01, 0x01, 0x38, 0xFD, 0xD1, 0x0E, 0x4B, 0x0E, 0x4A, 0x13, 0x60, 0x47, 0x20, 0x00, 0x21,
                            0x96, 0xF7, 0x96, 0xFF, 0x46, 0x20, 0x00, 0x21, 0x96, 0xF7, 0x92, 0xFF, 0x0A, 0x4A, 0x13, 0x68, 0x0A, 0x48, 0x03, 0x40,
                            0x13, 0x60, 0x0A, 0x4A, 0x13, 0x68, 0x0A, 0x48, 0x03, 0x40, 0x13, 0x60, 0x09, 0x4A, 0x13, 0x68, 0x09, 0x48, 0x03, 0x40,
                            0x13, 0x60, 0x00, 0xBD, 0x24, 0x80, 0x0E, 0x00, 0x81, 0x03, 0x0F, 0xFE, 0x5C, 0x00, 0x0F, 0x00, 0x60, 0xFC, 0x0E, 0x00,
                            0xFE, 0xFF, 0x00, 0x00, 0xFC, 0xFC, 0x0E, 0x00, 0xFF, 0x9F, 0x00, 0x00, 0x30, 0xFC, 0x0E, 0x00, 0x7F, 0xFF, 0x00, 0x00};

static WUDPatchList _wudPatchInstallCmd = {7,
                                     {{0x20, 0xBC, 0x65, 0x01, 0x00, 0x84, 0x42, 0x09, 0xD2, 0x84, 0x42, 0x09, 0xD1},
                                      {0x21, 0x84, 0x5A, 0x00, 0x00, 0x83, 0xF0, 0x74, 0xFF, 0x09, 0x0C, 0x08, 0x43},
                                      {0x22, 0x00, 0x61, 0x00, 0x00, 0x83, 0xF0, 0x40, 0xFC, 0x00, 0x00, 0x00, 0x00},
                                      {0x23, 0xCC, 0x9F, 0x01, 0x00, 0x6F, 0xF0, 0xE4, 0xFC, 0x03, 0x28, 0x7D, 0xD1},
                                      {0x24, 0x3C, 0x62, 0x01, 0x00, 0x28, 0x20, 0x00, 0xE0, 0x60, 0x8D, 0x23, 0x68},
                                      {0x25, 0x04, 0x12, 0x01, 0x00, 0x20, 0x1C, 0x20, 0x1C, 0x24, 0xE0, 0xB0, 0x21},
                                      {0x26, 0x74, 0x2F, 0x00, 0x00, 0x86, 0xF0, 0x18, 0xFD, 0x21, 0x4F, 0x3B, 0x60}}};

static u8 _wudResetAuthCountCmd[] = {0x30, 0x36, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

WUDCB __rvl_wudcb;
static WUDDevInfo _wudDiscWork;
static WUDDiscResp _wudDiscResp;
// written to the NAND, which wants its buffers aligned to 32 bytes
static WUDNandWbcInfo _wudNandWbcInfo ATTRIBUTE_ALIGN(32);
static SCBtDeviceInfoArray _scArray;
static NANDFileInfo _wudNandFileInfo;
static NANDCommandBlock _wudNandBlock;
static u8 _wudHandlerStack[0x1000] ATTRIBUTE_ALIGN(32);
static SCBtCmpDevInfoArray _spArray;
static BD_ADDR_PTR _dev_handle_to_bda[WUD_MAX_DEV_ENTRY];
static u16 _dev_handle_queue_size[WUD_MAX_DEV_ENTRY];
static u16 _dev_handle_notack_num[WUD_MAX_DEV_ENTRY];

static u8 _wudDiscNumResps;
static s8 _wudDiscRssi;
static u8 _wudTarget;
static u8 _scFlush;
static u32 _wudPatchSize;
static s32 _wudPatchOffset;
static u32 _wudPatchAddress;
static u8 _wudPatchNum;
static u8 _wudInstallNum;
static u8 _wudNandLocked;
static s32 _wudNandPhase;
static u32 _wudNandWbcCrc;
// For the balance board library. Nothing in this game sets them.
BOOL _linkedWBC;
void (*_initWBC)(void);
static u8* _wudNandBufPtr;
static u8 __bte_trace_level;
static u8 _wudPatchRemoveCmd;
static BOOL _wudAbortSync;
static BOOL _wudReadNand;
static BOOL _wudInitialized;

BOOL WUDIsLinkedWBC(void) {
    return _linkedWBC;
}

void* App_MEMalloc(u32 size) {
    DEBUGPrint("App_MEMalloc\n");
    return __rvl_wudcb.allocFunc(size);
}

BOOL App_MEMfree(void* pBlock) {
    DEBUGPrint("App_MEMfree\n");
    return __rvl_wudcb.freeFunc(pBlock);
}

static void __wudReverseAddr(BD_ADDR_PTR pDst, BD_ADDR src) {
    BOOL enabled;
    int i;

    enabled = OSDisableInterrupts();

    for (i = 0; i < BD_ADDR_LEN; i++) {
        pDst[i] = src[BD_ADDR_LEN - 1 - i];
    }

    OSRestoreInterrupts(enabled);
}

static int __wudSetPinCode(BD_ADDR addr) {
    BD_ADDR pin;
    BD_ADDR_PTR pAddr;

    pAddr = __rvl_wudcb.syncType == WUD_SYNC_TYPE_STANDARD ? __rvl_wudcb.hostAddr : addr;

    if (_wudDiscWork.UNK_0x5B == 4) {
        WUDiRemoveDevice(_wudDiscWork.devAddr);
        btm_remove_acl(_wudDiscWork.devAddr);
    }

    _wudDiscWork.UNK_0x5A = 1;

    __wudReverseAddr(pin, pAddr);
    BTA_DmPinReply(addr, TRUE, BD_ADDR_LEN, pin);

    return 0;
}

static void __wudClearDiscoverResult(void) {
    _wudDiscNumResps = 0;
    memset(&_wudDiscResp, 0, sizeof(WUDDiscResp));
}

static BOOL __wudIsBusyLinkKeyCmd(void) {
    return __rvl_wudcb.linkKeyState != WUD_STATE_LINK_KEY_START ? TRUE : FALSE;
}

static BOOL __wudIsBusyScCmd(void) {
    return SCCheckStatus() == SC_STATUS_BUSY ? TRUE : FALSE;
}

static void __wudPrepareWBCDevInfo(BD_ADDR addr, LINK_KEY linkKey, u8* devName) {
    u32 crc;

    if (addr) {
        WUD_BDCPY(_wudNandWbcInfo.devAddr, addr);
    }

    if (linkKey) {
        memcpy(_wudNandWbcInfo.linkKey, linkKey, LINK_KEY_LEN);
    }

    if (devName) {
        memcpy(_wudNandWbcInfo.devName, devName, sizeof(_wudNandWbcInfo.devName));
    }

    crc = __wudCalcWiiFitCrc2(&_wudNandWbcInfo, RP_HEALTH_WBC_INFO_SIZE, _wudNandWbcCrc);
    memcpy(_wudNandWbcInfo.crc, &crc, sizeof(crc));
}

static void __wudSyncFlushCallback(SCStatus status) {
    WUDCB* p = &__rvl_wudcb;

    DEBUGPrint("__wudSyncFlushCallback() : %d, Sync: %d\n", status, __rvl_wudcb.syncState);

    if (p->syncState == WUD_STATE_SYNC_START) {
        return;
    }

    if (status == SC_STATUS_OK) {
        p->syncState = WUD_STATE_SYNC_COMPLETE;
    } else {
        p->syncState = WUD_STATE_SYNC_ERROR;
    }
}

static u8 __wudSyncPrepareSearch() {
    WUDCB* p = &__rvl_wudcb;
    int i;
    WUDDevInfo* pInfo;

    WUDSetVisibility(FALSE, FALSE);

    if (p->syncLoopNum == 0) {
        return WUD_STATE_SYNC_DONE;
    }

    if (__wudGetLinkNumber() == WUD_MAX_CHANNELS && __wudGetConnectionNumber() == WUD_MAX_CHANNELS) {
        return WUD_STATE_SYNC_DONE;
    }

    if (p->syncSkipChecks) {
        for (i = 0; i < WUD_MAX_DEV_ENTRY; i++) {
            pInfo = __wudGetDevInfoByIndex(i);

            if (pInfo->status == 9) {
                WUDSetSniffMode(pInfo->devAddr, 0);
                return WUD_STATE_SYNC_PREPARE_SEARCH;
            }
        }
    }

    if (p->syncLoopNum > 0) {
        p->syncLoopNum--;
    }

    p->waitStartSearchFrames = 50;

    return WUD_STATE_SYNC_WAIT_FOR_START_SEARCH;
}

static WUDSyncState __wudSyncWaitForStartSearch(void) {
    WUDCB* p = &__rvl_wudcb;

    if (!p->syncLoopNum) {
        return WUD_STATE_SYNC_DONE;
    }

    if (__wudGetLinkNumber() == WUD_MAX_CHANNELS && __wudGetConnectionNumber() == WUD_MAX_CHANNELS) {
        return WUD_STATE_SYNC_DONE;
    }

    if (--p->waitStartSearchFrames < 0) {
        return WUD_STATE_SYNC_START_SEARCH;
    } else {
        return WUD_STATE_SYNC_WAIT_FOR_START_SEARCH;
    }
}

static u8 __wudSyncStartSearch() {
    WUDCB* p = &__rvl_wudcb;
    tBTA_DM_INQ dm_inq;

    dm_inq.mode = BTM_LIMITED_INQUIRY;
    dm_inq.max_resps = 1;
    dm_inq.filter_type = 0;

    if (p->syncSkipChecks) {
        dm_inq.duration = 5 - 2;
    } else if (p->syncType == WUD_SYNC_TYPE_STANDARD) {
        dm_inq.duration = __wudGetLinkNumber() == WUD_MAX_CHANNELS - 1 ? 10 : 5;
        dm_inq.duration -= p->syncLoopNum;
    } else {
        dm_inq.duration = __wudGetLinkNumber() == WUD_MAX_CHANNELS - 1 ? 10 - 2 : 5 - 2;
    }

    __wudClearDiscoverResult();
    BTA_DmSearch(&dm_inq, 0, &__wudSearchEventStackCallback);

    return WUD_STATE_SYNC_WAIT_FOR_SEARCH_RESULT;
}

static u8 __wudSyncWaitForSearchResult() {
    if (__wudGetLinkNumber() == WUD_MAX_CHANNELS && __wudGetConnectionNumber() == WUD_MAX_CHANNELS) {
        DEBUGPrint("Cancel searching because 4 connections exist.\n");
        BTA_DmSearchCancel();

        return WUD_STATE_SYNC_CANCEL_SEARCH;
    }

    return WUD_STATE_SYNC_WAIT_FOR_SEARCH_RESULT;
}

static u8 __wudSyncCheckSearchResult() {
    u32 nextState = WUD_STATE_SYNC_PREPARE_SEARCH;

    if (_wudDiscNumResps > 0) {
        if (WUD_DEV_NAME_IS_CNT(_wudDiscResp.devName)) {
            nextState = WUD_STATE_SYNC_IS_EXISTED_DEVICE;
        }
        if (WUDIsLinkedWBC() && WUD_DEV_NAME_IS_WBC(_wudDiscResp.devName)) {
            nextState = WUD_STATE_SYNC_IS_EXISTED_DEVICE;
        }
        if (_wudDiscRssi < __rvl_wudcb.syncRssi) {
            nextState = WUD_STATE_SYNC_PREPARE_SEARCH;
        }
    }
    if (__rvl_wudcb.syncType == WUD_SYNC_TYPE_STANDARD && nextState == WUD_STATE_SYNC_PREPARE_SEARCH) {
        nextState = WUD_STATE_SYNC_WAIT_FOR_INCOMING;
        __rvl_wudcb.waitIncomingFrames = __wudGetLinkNumber() == 3 ? 200 : 100;
        WUDSetVisibility(0, 1);
    }

    return nextState;
}

static u8 __wudSyncWaitForIncoming(void) {
    u32 nextState = WUD_STATE_SYNC_WAIT_FOR_INCOMING;

    if (__wudGetLinkNumber() == WUD_MAX_CHANNELS && __wudGetConnectionNumber() == WUD_MAX_CHANNELS) {
        nextState = WUD_STATE_SYNC_DONE;
    }

    if (--__rvl_wudcb.waitIncomingFrames < 0) {
        nextState = WUD_STATE_SYNC_PREPARE_SEARCH;
    }

    return nextState;
}

static u8 __wudSyncIsExistedDevice() {
    WUDDevInfo* pInfo;
    WUDSyncState nextState = WUD_STATE_SYNC_PREPARE_FOR_UNKNOWN_DEVICE;

    pInfo = WUDiGetDevInfo(_wudDiscResp.devAddr);

    if (pInfo) {
        memcpy(&_wudDiscWork, pInfo, sizeof(WUDDevInfo));
        nextState = WUD_STATE_SYNC_PREPARE_FOR_EXISTED_DEVICE;
    }

    return nextState;
}

static u8 __wudSyncPrepareForExistedDevice() {
    WUDDevInfo* pWork = &_wudDiscWork;

    pWork->status = 1;

    switch (pWork->UNK_0x5B) {
        case 0:
        case 2:
        case 4:
        case 5: {
            if (__rvl_wudcb.syncType == WUD_SYNC_TYPE_STANDARD) {
                WUDiMoveBottomStdDevInfoPtr(pWork);
                WUDiRemoveDevice(pWork->devAddr);
                return WUD_STATE_SYNC_PREPARE_FOR_UNKNOWN_DEVICE;
            } else {
                pWork->UNK_0x5B = 4;
            }

            break;
        }
        case 1:
        case 3: {
            WUDiMoveBottomSmpDevInfoPtr(pWork);
            WUDiRemoveDevice(__rvl_wudcb.smpListTail->devInfo->devAddr);
            return WUD_STATE_SYNC_PREPARE_FOR_UNKNOWN_DEVICE;
        }
    }

    return WUD_STATE_SYNC_TRY_CONNECT;
}

static u8 __wudSyncPrepareForUnknownDevice() {
    WUDDevInfo* pWork = &_wudDiscWork;

    pWork->status = 1;
    pWork->UNK_0x5B = __rvl_wudcb.syncType == WUD_SYNC_TYPE_STANDARD ? 0 : 1;

    WUD_BDCPY(pWork->devAddr, _wudDiscResp.devAddr);
    memcpy(pWork->conf.devName, _wudDiscResp.devName, sizeof(_wudDiscResp.devName));
    memset(pWork->linkKey, 0, LINK_KEY_LEN);

    return WUD_STATE_SYNC_TRY_CONNECT;
}

static u8 __wudSyncTryConnect() {
    WUDSyncState nextState = WUD_STATE_SYNC_ERROR;
    WUDDevInfo* pInfo;

    if (WUD_DEV_NAME_IS_CNT(_wudDiscResp.devName)) {
        _wudDiscWork.status = 2;
        BTA_HhOpen(_wudDiscWork.devAddr, BTA_HH_PROTO_RPT_MODE, BTA_SEC_AUTHENTICATE);
        nextState = WUD_STATE_SYNC_6;
    }

    if (WUDIsLinkedWBC() && WUD_DEV_NAME_IS_WBC(_wudDiscResp.devName)) {
        pInfo = __wudGetWbcDevice();

        if (pInfo != NULL) {
            DEBUGPrint("Found the registered WBC in database\n");

            if (pInfo->status > 1) {
                return nextState;
            }

            if (WUD_BDCMP(_wudDiscResp.devAddr, pInfo->devAddr) != 0) {
                DEBUGPrint("Removed WBC data\n");
                WUDiMoveBottomStdDevInfoPtr(pInfo);
                WUDiRemoveDevice(pInfo->devAddr);
            }
        }

        _wudDiscWork.status = 2;
        BTA_HhOpen(_wudDiscWork.devAddr, BTA_HH_PROTO_RPT_MODE, BTA_SEC_AUTHENTICATE);
        nextState = WUD_STATE_SYNC_6;
    }

    return nextState;
}

static u8 __wudSyncRegisterDevice() {
    WUDSyncState nextState;

    switch (_wudDiscWork.UNK_0x5B) {
        case 1: {
            nextState = WUD_STATE_SYNC_VIRGIN_SIMPLE;
            break;
        }
        case 0: {
            nextState = WUD_STATE_SYNC_VIRGIN_STANDARD;
            break;
        }
        case 5: {
            nextState = WUD_STATE_SYNC_CHANGE_SIMPLE_TO_STANDARD;
            break;
        }
        case 3: {
            nextState = WUD_STATE_SYNC_COMPLETE;
            break;
        }
        case 4: {
            nextState = WUD_STATE_SYNC_STORED_LINK_KEY_TO_EEPROM;
            break;
        }
        case 2: {
            nextState = WUD_STATE_SYNC_STORED_LINK_KEY_TO_EEPROM;
            break;
        }
        default: {
            break;
        }
    }

    return nextState;
}

static u8 __wudSyncVirginSimple() {
    WUDDevInfo* pInfo;

    if (__wudGetSmpDevNumber() == WUD_MAX_DEV_ENTRY_FOR_SMP) {
        pInfo = __wudGetOldestSmpDevice();
        WUDiRemoveDevice(pInfo->devAddr);
    }

    pInfo = __wudGetNewDevInfo();

    if (!pInfo) {
        return WUD_STATE_SYNC_ERROR;
    }

    if (pInfo->status != 0) {
        return WUD_STATE_SYNC_ERROR;
    }

    memcpy(pInfo, &_wudDiscWork, sizeof(WUDDevInfo));

    WUDiRegisterDevice(pInfo->devAddr);
    WUDiMoveTopSmpDevInfoPtr(pInfo);

    WUDiSetDevAddrForHandle(pInfo->devHandle, pInfo->devAddr);

    return WUD_STATE_SYNC_COMPLETE;
}

static u8 __wudSyncVirginStandard() {
    WUDCB* p = &__rvl_wudcb;
    WUDDevInfo* pInfo;

    if (WUDIsLinkedWBC() && WUD_DEV_NAME_IS_CNT(_wudDiscWork.conf.devName)) {
        pInfo = __wudGetWbcDevice();

        // keep the balance board: drop the oldest remote instead
        if (pInfo && __wudGetStdDevNumber() == WUD_MAX_DEV_ENTRY_FOR_STD) {
            pInfo = p->stdListTail->prev->devInfo;
            WUDiMoveBottomStdDevInfoPtr(pInfo);
            WUDiRemoveDevice(pInfo->devAddr);
        }
    }

    if (__wudGetStdDevNumber() == WUD_MAX_DEV_ENTRY_FOR_STD) {
        pInfo = __wudGetOldestStdDevice();
        WUDiRemoveDevice(pInfo->devAddr);
    }

    pInfo = __wudGetNewDevInfo();

    if (!pInfo) {
        return WUD_STATE_SYNC_ERROR;
    }

    if (pInfo->status != 0) {
        return WUD_STATE_SYNC_ERROR;
    }

    memcpy(pInfo, &_wudDiscWork, sizeof(WUDDevInfo));

    WUDiRegisterDevice(pInfo->devAddr);
    WUDiMoveTopStdDevInfoPtr(pInfo);

    if (WUDIsLinkedWBC() && WUD_DEV_NAME_IS_WBC(pInfo->conf.devName)) {
        WUDiMoveTopOfUnusedStdDevice(pInfo);

        memset(&_scArray.wbc, 0, sizeof(SCBtDeviceInfo));
        WUD_BDCPY(_scArray.wbc.addr, pInfo->devAddr);
        memcpy(&_scArray.wbc.info, &pInfo->conf, sizeof(SC_BT_DEV_INFO));
        memcpy(&_scArray.wbc.info.unk_0x14, pInfo->linkKey, LINK_KEY_LEN);

        __wudPrepareWBCDevInfo(pInfo->devAddr, pInfo->linkKey, pInfo->conf.devName);
    }

    WUDiSetDevAddrForHandle(pInfo->devHandle, pInfo->devAddr);

    return WUD_STATE_SYNC_STORED_LINK_KEY_TO_EEPROM;
}

static u8 __wudSyncChangeSmp2Std() {
    WUDiMoveBottomSmpDevInfoPtr(&_wudDiscWork);
    WUDiRemoveDevice(__rvl_wudcb.smpListTail->devInfo->devAddr);
    return __wudSyncVirginStandard();
}

static u8 __wudSyncStoredLinkKeyToE2prom() {
    WUDCB* p = &__rvl_wudcb;
    WUDDevInfo* pInfo;

    if (__wudIsBusyLinkKeyCmd()) {
        return WUD_STATE_SYNC_STORED_LINK_KEY_TO_EEPROM;
    }

    // the balance board's link key is not written to the Bluetooth module
    if (WUDIsLinkedWBC() && WUD_DEV_NAME_IS_WBC(_wudDiscWork.conf.devName)) {
        return WUD_STATE_SYNC_STORED_DEV_INFO_TO_NAND;
    }

    pInfo = WUDiGetDevInfo(_wudDiscWork.devAddr);

    DEBUGPrint("write stored link key\n");

    DEBUGPrint("addr : %02x:%02x:%02x:%02x:%02x:%02x\n", pInfo->devAddr[0], pInfo->devAddr[1], pInfo->devAddr[2], pInfo->devAddr[3],
               pInfo->devAddr[4], pInfo->devAddr[5]);

    DEBUGPrint("key  : %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n", pInfo->linkKey[0], pInfo->linkKey[1],
               pInfo->linkKey[2], pInfo->linkKey[3], pInfo->linkKey[4], pInfo->linkKey[5], pInfo->linkKey[6], pInfo->linkKey[7], pInfo->linkKey[8],
               pInfo->linkKey[9], pInfo->linkKey[10], pInfo->linkKey[11], pInfo->linkKey[12], pInfo->linkKey[13], pInfo->linkKey[14],
               pInfo->linkKey[15]);

    p->linkKeyState = WUD_STATE_LINK_KEY_WRITING;

    BTM_WriteStoredLinkKey(1, &pInfo->devAddr, &pInfo->linkKey, __wudLinkKeyEventStackCallback);

    return WUD_STATE_SYNC_WAIT_FOR_STORING;
}

static u8 __wudSyncWaitForStoring() {
    return !__wudIsBusyLinkKeyCmd() ? WUD_STATE_SYNC_STORED_DEV_INFO_TO_NAND : WUD_STATE_SYNC_WAIT_FOR_STORING;
}

static u8 __wudSyncStoredDevInfoToNand() {
    WUDSyncState nextState = WUD_STATE_SYNC_COMPLETE;
    u8 i;
    WUDDevInfoList* pIt;

    if (__wudIsBusyScCmd()) {
        return WUD_STATE_SYNC_STORED_DEV_INFO_TO_NAND;
    }

    memset(&_scArray.regist, 0, sizeof(SCBtDeviceInfo) * WUD_MAX_DEV_ENTRY_FOR_STD);

    _scArray.numRegist = __wudGetStdDevNumber();

    for (pIt = __rvl_wudcb.stdListHead, i = 0; pIt != NULL; i++, pIt = pIt->next) {
        WUD_BDCPY(_scArray.regist[i].addr, pIt->devInfo->devAddr);
        memcpy(&_scArray.regist[i].info, &pIt->devInfo->conf, sizeof(SC_BT_DEV_INFO));
    }

    DEBUGPrint("%d devices is stored into SC.\n", i);

    if (!SCSetBtDeviceInfoArray(&_scArray)) {
        return WUD_STATE_SYNC_STORED_DEV_INFO_TO_NAND;
    }

    nextState = WUD_STATE_SYNC_SC_FLUSH;

    // in Japan the balance board is also written into Wii Fit's save file
    if (WUDIsLinkedWBC() && WUD_DEV_NAME_IS_WBC(_wudDiscWork.conf.devName) && SCGetProductGameRegion() == SC_PRODUCT_GAME_REGION_JP) {
        nextState = WUD_STATE_WII_FIT_OPEN;
    }

    return nextState;
}

static void __wudOpenWiiFitCallback(s32 result, NANDCommandBlock* pBlock) {
    if (__rvl_wudcb.syncState != WUD_STATE_SYNC_START) {
        __rvl_wudcb.syncState = result == NAND_RESULT_OK ? WUD_STATE_WII_FIT_SEEK : WUD_STATE_SYNC_SC_FLUSH;
    }

    if (__rvl_wudcb.deleteState != WUD_STATE_DELETE_START) {
        __rvl_wudcb.deleteState = result == NAND_RESULT_OK ? WUD_STATE_WII_FIT_SEEK : WUD_STATE_DELETE_CLEANUP_SETTING;
    }

    _wudNandLocked = FALSE;
    DEBUGPrint("NANDOpen. [%d]\n", result);
}

static void __wudOpenWiiFitFile(void) {
    const char* path = RP_HEALTH_FILE_PATH;

    if (_wudNandLocked) {
        return;
    }

    DEBUGPrint("WbcSaveData: %s\n", path);

    _wudNandLocked = TRUE;
    NANDOpenAsync(path, &_wudNandFileInfo, NAND_ACCESS_WRITE, __wudOpenWiiFitCallback, &_wudNandBlock);
}

static void __wudSeekWiiFitCallback(s32 result, NANDCommandBlock* pBlock) {
    if (__rvl_wudcb.syncState != WUD_STATE_SYNC_START) {
        __rvl_wudcb.syncState = result == RP_HEALTH_FILE_OFFSET ? WUD_STATE_WII_FIT_UPDATE : WUD_STATE_SYNC_SC_FLUSH;
    }

    if (__rvl_wudcb.deleteState != WUD_STATE_DELETE_START) {
        __rvl_wudcb.deleteState = result == RP_HEALTH_FILE_OFFSET ? WUD_STATE_WII_FIT_UPDATE : WUD_STATE_DELETE_CLEANUP_SETTING;
    }

    _wudNandLocked = FALSE;
    DEBUGPrint("NANDSeek. [%d]\n", result);
}

static void __wudSeekWiiFitFile(void) {
    if (_wudNandLocked) {
        return;
    }

    _wudNandLocked = TRUE;
    NANDSeekAsync(&_wudNandFileInfo, RP_HEALTH_FILE_OFFSET, 0, __wudSeekWiiFitCallback, &_wudNandBlock);
}

static void __wudUpdateWiiFitCallback(s32 result, NANDCommandBlock* pBlock) {
    if (__rvl_wudcb.syncState != WUD_STATE_SYNC_START) {
        __rvl_wudcb.syncState = result == RP_HEALTH_WBC_INFO_SIZE + (s32)sizeof(u32) ? WUD_STATE_WII_FIT_CLOSE : WUD_STATE_SYNC_SC_FLUSH;
    }

    if (__rvl_wudcb.deleteState != WUD_STATE_DELETE_START) {
        __rvl_wudcb.deleteState =
            result == RP_HEALTH_WBC_INFO_SIZE + (s32)sizeof(u32) ? WUD_STATE_WII_FIT_CLOSE : WUD_STATE_DELETE_CLEANUP_SETTING;
    }

    _wudNandLocked = FALSE;
    DEBUGPrint("NANDWrite. [%d]\n", result);
}

static void __wudUpdateWiiFitFile(void) {
    if (_wudNandLocked) {
        return;
    }

    DEBUGPrint("WBC to WiiFit\n");
    DEBUGPrint(" %02x %02x %02x %02x %02x %02x\n", _wudNandWbcInfo.devAddr[0], _wudNandWbcInfo.devAddr[1], _wudNandWbcInfo.devAddr[2],
               _wudNandWbcInfo.devAddr[3], _wudNandWbcInfo.devAddr[4], _wudNandWbcInfo.devAddr[5]);
    DEBUGPrint(" %02x %02x %02x %02x %02x %02x %02x %02x\n", _wudNandWbcInfo.linkKey[0], _wudNandWbcInfo.linkKey[1],
               _wudNandWbcInfo.linkKey[2], _wudNandWbcInfo.linkKey[3], _wudNandWbcInfo.linkKey[4], _wudNandWbcInfo.linkKey[5],
               _wudNandWbcInfo.linkKey[6], _wudNandWbcInfo.linkKey[7]);
    DEBUGPrint(" %02x %02x %02x %02x %02x %02x %02x %02x\n", _wudNandWbcInfo.linkKey[8], _wudNandWbcInfo.linkKey[9],
               _wudNandWbcInfo.linkKey[10], _wudNandWbcInfo.linkKey[11], _wudNandWbcInfo.linkKey[12], _wudNandWbcInfo.linkKey[13],
               _wudNandWbcInfo.linkKey[14], _wudNandWbcInfo.linkKey[15]);
    DEBUGPrint(" %s\n", _wudNandWbcInfo.devName);
    DEBUGPrint(" %02x%02x%02x%02x\n", _wudNandWbcInfo.crc[0], _wudNandWbcInfo.crc[1], _wudNandWbcInfo.crc[2], _wudNandWbcInfo.crc[3]);

    _wudNandLocked = TRUE;
    NANDWriteAsync(&_wudNandFileInfo, &_wudNandWbcInfo, RP_HEALTH_WBC_INFO_SIZE + sizeof(u32), __wudUpdateWiiFitCallback, &_wudNandBlock);
}

static void __wudCloseWiiFitCallback(s32 result, NANDCommandBlock* pBlock) {
    if (__rvl_wudcb.syncState != WUD_STATE_SYNC_START) {
        __rvl_wudcb.syncState = WUD_STATE_SYNC_SC_FLUSH;
    }

    if (__rvl_wudcb.deleteState != WUD_STATE_DELETE_START) {
        __rvl_wudcb.deleteState = WUD_STATE_DELETE_CLEANUP_SETTING;
    }

    _wudNandLocked = FALSE;
    DEBUGPrint("NANDClose. [%d]\n", result);
}

static void __wudCloseWiiFit(void) {
    if (_wudNandLocked) {
        return;
    }

    _wudNandLocked = TRUE;
    NANDCloseAsync(&_wudNandFileInfo, __wudCloseWiiFitCallback, &_wudNandBlock);
}

static void __wudSyncScFlush() {
    WUDCB* p = &__rvl_wudcb;
    BOOL enabled;

    enabled = OSDisableInterrupts();
    p->syncState = WUD_STATE_SYNC_13;
    OSRestoreInterrupts(enabled);

    SCFlushAsync(__wudSyncFlushCallback);
}

static u8 __wudSyncWaitForReading() {
    WUDSyncState nextState = WUD_STATE_SYNC_WAIT_FOR_READING;

    if (!__wudIsBusyLinkKeyCmd()) {
        nextState = WUD_STATE_SYNC_VIRGIN_STANDARD;
        WUDiRemoveDevice(__rvl_wudcb.pairAddr);
    }

    return nextState;
}

static u8 __wudSyncComplete() {
    __rvl_wudcb.syncedNum++;

    memset(&_wudDiscWork, 0, sizeof(WUDDevInfo));

    return __rvl_wudcb.syncType == WUD_SYNC_TYPE_STANDARD ? WUD_STATE_SYNC_DONE : WUD_STATE_SYNC_PREPARE_SEARCH;
}

static u8 __wudSyncError() {
    memset(&_wudDiscWork, 0, sizeof(WUDDevInfo));

    return __rvl_wudcb.syncType == WUD_SYNC_TYPE_STANDARD ? WUD_STATE_SYNC_DONE : WUD_STATE_SYNC_PREPARE_SEARCH;
}

static u8 __wudSyncDone() {
    WUDCB* p = &__rvl_wudcb;
    WUDDevInfo* pInfo;
    int i;
    WUDSyncDeviceCallback pSyncCallback;

    if (p->syncSkipChecks) {
        for (i = 0; i < WUD_MAX_DEV_ENTRY; i++) {
            pInfo = __wudGetDevInfoByIndex(i);

            if (pInfo->status == 8) {
                WUDSetSniffMode(pInfo->devAddr, 8);
                return WUD_STATE_SYNC_DONE;
            }
        }
    }

    OSCancelAlarm(&p->alarm);

    if (!_wudAbortSync) {
        WUDSetVisibility(FALSE, TRUE);
    }

    pSyncCallback = p->syncType == WUD_SYNC_TYPE_STANDARD ? p->syncStdCB : p->syncSmpCB;

    if (pSyncCallback != NULL) {
        pSyncCallback(WUD_RESULT_SYNC_DONE, p->syncedNum);
    }

    DEBUGPrint("Pairing Done\n");
    return WUD_STATE_SYNC_START;
}

static void __wudSyncHandler() {
    WUDCB* p = &__rvl_wudcb;

    switch (p->syncState) {
        case WUD_STATE_SYNC_PREPARE_SEARCH: {
            p->syncState = __wudSyncPrepareSearch();
            break;
        }
        case WUD_STATE_SYNC_WAIT_FOR_START_SEARCH: {
            p->syncState = __wudSyncWaitForStartSearch();
            break;
        }
        case WUD_STATE_SYNC_START_SEARCH: {
            p->syncState = __wudSyncStartSearch();
            break;
        }
        case WUD_STATE_SYNC_CHECK_SEARCH_RESULT: {
            p->syncState = __wudSyncCheckSearchResult();
            break;
        }
        case WUD_STATE_SYNC_IS_EXISTED_DEVICE: {
            p->syncState = __wudSyncIsExistedDevice();
            break;
        }
        case WUD_STATE_SYNC_PREPARE_FOR_EXISTED_DEVICE: {
            p->syncState = __wudSyncPrepareForExistedDevice();
            break;
        }
        case WUD_STATE_SYNC_PREPARE_FOR_UNKNOWN_DEVICE: {
            p->syncState = __wudSyncPrepareForUnknownDevice();
            break;
        }
        case WUD_STATE_SYNC_TRY_CONNECT: {
            p->syncState = __wudSyncTryConnect();
            break;
        }
        case WUD_STATE_SYNC_REGISTER_DEVICE: {
            p->syncState = __wudSyncRegisterDevice();
            break;
        }
        case WUD_STATE_SYNC_VIRGIN_SIMPLE: {
            p->syncState = __wudSyncVirginSimple();
            break;
        }
        case WUD_STATE_SYNC_VIRGIN_STANDARD: {
            p->syncState = __wudSyncVirginStandard();
            break;
        }
        case WUD_STATE_SYNC_CHANGE_SIMPLE_TO_STANDARD: {
            p->syncState = __wudSyncChangeSmp2Std();
            break;
        }
        case WUD_STATE_SYNC_STORED_LINK_KEY_TO_EEPROM: {
            p->syncState = __wudSyncStoredLinkKeyToE2prom();
            break;
        }
        case WUD_STATE_SYNC_WAIT_FOR_STORING: {
            p->syncState = __wudSyncWaitForStoring();
            break;
        }
        case WUD_STATE_SYNC_STORED_DEV_INFO_TO_NAND: {
            p->syncState = __wudSyncStoredDevInfoToNand();
            break;
        }
        case WUD_STATE_SYNC_SC_FLUSH: {
            __wudSyncScFlush();
            break;
        }
        case WUD_STATE_SYNC_WAIT_FOR_READING: {
            p->syncState = __wudSyncWaitForReading();
            break;
        }
        case WUD_STATE_SYNC_COMPLETE: {
            p->syncState = __wudSyncComplete();
            break;
        }
        case WUD_STATE_SYNC_ERROR: {
            p->syncState = __wudSyncError();
            break;
        }
        case WUD_STATE_SYNC_DONE: {
            p->syncState = __wudSyncDone();
            break;
        }
        case WUD_STATE_SYNC_WAIT_FOR_SEARCH_RESULT: {
            p->syncState = __wudSyncWaitForSearchResult();
            break;
        }
        case WUD_STATE_SYNC_WAIT_FOR_INCOMING: {
            p->syncState = __wudSyncWaitForIncoming();
            break;
        }
        case WUD_STATE_WII_FIT_OPEN: {
            __wudOpenWiiFitFile();
            break;
        }
        case WUD_STATE_WII_FIT_SEEK: {
            __wudSeekWiiFitFile();
            break;
        }
        case WUD_STATE_WII_FIT_UPDATE: {
            __wudUpdateWiiFitFile();
            break;
        }
        case WUD_STATE_WII_FIT_CLOSE: {
            __wudCloseWiiFit();
            break;
        }
        case WUD_STATE_SYNC_6:
        case WUD_STATE_SYNC_13:
        case WUD_STATE_SYNC_CANCEL_SEARCH: {
            break;
        }
        case WUD_STATE_SYNC_11:
        case WUD_STATE_SYNC_12:
        default: {
            DEBUGPrint("WARNING: Illigal status\n");
            break;
        }
    }
}

static void __wudSyncHandler0(OSAlarm* pAlarm, OSContext* pContext) {
    OSSwitchFiberEx((u32)pAlarm, (u32)pContext, 0, 0, __wudSyncHandler, _wudHandlerStack + sizeof(_wudHandlerStack));
}

static void __wudDeleteFlushCallback(SCStatus status) {
    WUDCB* p = &__rvl_wudcb;

    DEBUGPrint("__wudDeleteFlushCallback() : %d, Delete: %d\n", status, __rvl_wudcb.deleteState);

    if (p->deleteState == WUD_STATE_DELETE_START) {
        return;
    }

    if (status == SC_STATUS_OK) {
        p->deleteState = WUD_STATE_DELETE_DONE;
    } else {
        p->deleteState = WUD_STATE_DELETE_DONE;
    }
}

static WUDDeleteState __wudDeleteDisallowIncoming() {
    BTA_DmSetVisibility(FALSE, FALSE);
    return WUD_STATE_DELETE_DISCONNECT_ALL;
}

static WUDDeleteState __wudDeleteDisconnectAll() {
    int i;
    WUDCB* p = &__rvl_wudcb;
    WUDDevInfo* pInfo;

    for (i = 0; i < WUD_MAX_DEV_ENTRY_FOR_STD; i++) {
        pInfo = &p->stdDevs[i];

        if (pInfo->status > 1) {
            btm_remove_acl(pInfo->devAddr);
        }
    }

    for (i = 0; i < WUD_MAX_DEV_ENTRY_FOR_SMP; i++) {
        pInfo = &p->smpDevs[i];

        if (pInfo->status > 1) {
            btm_remove_acl(pInfo->devAddr);
        }
    }

    return WUD_STATE_DELETE_CLEANUP_DATABASE;
}

static WUDDeleteState __wudDeleteCleanupDatabase() {
    int i;
    WUDCB* p = &__rvl_wudcb;
    WUDDevInfo* pInfo;

    if (__wudGetLinkNumber() > 0) {
        return WUD_STATE_DELETE_CLEANUP_DATABASE;
    }

    for (i = 0; i < WUD_MAX_DEV_ENTRY_FOR_STD; i++) {
        pInfo = &p->stdDevs[i];

        if (pInfo->status == 1) {
            WUDiRemoveDevice(pInfo->devAddr);
        }
    }

    for (i = 0; i < WUD_MAX_DEV_ENTRY_FOR_SMP; i++) {
        pInfo = &p->smpDevs[i];

        if (pInfo->status == 1) {
            WUDiRemoveDevice(pInfo->devAddr);
        }
    }

    if (WUDIsLinkedWBC()) {
        if (SCGetProductGameRegion() == SC_PRODUCT_GAME_REGION_JP) {
            // clear the balance board's link key in Wii Fit's save file
            LINK_KEY linkKey = {0};
            __wudPrepareWBCDevInfo(NULL, linkKey, NULL);

            return WUD_STATE_WII_FIT_OPEN;
        } else {
            return WUD_STATE_DELETE_CLEANUP_SETTING;
        }
    } else {
        return WUD_STATE_DELETE_CLEANUP_SETTING;
    }
}

static void __wudDeleteCleanupSetting() {
    WUDCB* p = &__rvl_wudcb;
    BOOL success = FALSE;

    if (SCCheckStatus() == SC_STATUS_BUSY) {
        return;
    }

    memset(&_scArray, 0, sizeof(SCBtDeviceInfoArray));
    memset(&_spArray, 0, sizeof(SCBtCmpDevInfoArray));
    success |= SCSetBtDeviceInfoArray(&_scArray);
    success |= SCSetBtCmpDevInfoArray(&_spArray);

    if (success) {
        p->deleteState = WUD_STATE_DELETE_6;
        SCFlushAsync(__wudDeleteFlushCallback);
    } else {
        p->deleteState = WUD_STATE_DELETE_DONE;
    }
}

static WUDDeleteState __wudDeleteDone() {
    WUDCB* p = &__rvl_wudcb;

    WUDSetVisibility(FALSE, WUDGetConnectable());

    OSCancelAlarm(&p->alarm);
    p->deleteState = WUD_STATE_DELETE_START;

    if (p->clearDevCB) {
        p->clearDevCB(WUD_RESULT_DELETE_COMPLETE);
    }

    return WUD_STATE_DELETE_START;
}

static void __wudDeleteHandler() {
    WUDCB* p = &__rvl_wudcb;

    p->syncState = WUD_STATE_SYNC_START;

    switch (p->deleteState) {
        case WUD_STATE_DELETE_DISALLOW_INCOMING: {
            p->deleteState = __wudDeleteDisallowIncoming();
            break;
        }
        case WUD_STATE_DELETE_DISCONNECT_ALL: {
            p->deleteState = __wudDeleteDisconnectAll();
            break;
        }
        case WUD_STATE_DELETE_CLEANUP_DATABASE: {
            p->deleteState = __wudDeleteCleanupDatabase();
            break;
        }
        case WUD_STATE_DELETE_CLEANUP_SETTING: {
            __wudDeleteCleanupSetting();
            break;
        }
        case WUD_STATE_DELETE_DONE: {
            p->deleteState = __wudDeleteDone();
            break;
        }
        case WUD_STATE_WII_FIT_OPEN: {
            __wudOpenWiiFitFile();
            break;
        }
        case WUD_STATE_WII_FIT_SEEK: {
            __wudSeekWiiFitFile();
            break;
        }
        case WUD_STATE_WII_FIT_UPDATE: {
            __wudUpdateWiiFitFile();
            break;
        }
        case WUD_STATE_WII_FIT_CLOSE: {
            __wudCloseWiiFit();
            break;
        }
        case WUD_STATE_DELETE_7: {
            break;
        }
    }
}

static void __wudDeleteHandler0(OSAlarm* pAlarm, OSContext* pContext) {
    OSSwitchFiberEx((u32)pAlarm, (u32)pContext, 0, 0, __wudDeleteHandler, _wudHandlerStack + sizeof(_wudHandlerStack));
}

static WUDStackState __wudStackGetStoredLinkKey() {
    WUDCB* p = &__rvl_wudcb;

    // Set by BTA_HH_ENABLE_EVT
    if (p->hhFlags == 1) {
        p->linkKeyState = WUD_STATE_LINK_KEY_READING;
        BTM_ReadStoredLinkKey(NULL, __wudLinkKeyEventStackCallback);
    }

    return WUD_STATE_STACK_CHECK_DEVICE_INFO;
}

static WUDStackState __wudStackCheckDeviceInfo() {
    WUDCB* p = &__rvl_wudcb;
    WUDDevInfo* pInfo;
    int i;

    if (p->linkKeyState == WUD_STATE_LINK_KEY_START) {
        for (i = 0; i < WUD_MAX_DEV_ENTRY; i++) {
            pInfo = __wudGetDevInfoByIndex(i);
            if (pInfo->status == 0) {
                continue;
            }

            if (pInfo->UNK_0x5C == 1) {
                p->linkKeyState = WUD_STATE_LINK_KEY_DELETING;
                BTM_DeleteStoredLinkKey(pInfo->devAddr, __wudLinkKeyEventStackCallback);

                pInfo->UNK_0x5C = 0;
                return WUD_STATE_STACK_CHECK_DEVICE_INFO;
            }

            if (pInfo->UNK_0x5C != 3) {
                __wudRemoveDevInfo(pInfo->devAddr);
            }
        }

        return WUD_STATE_STACK_DONE;
    }

    return WUD_STATE_STACK_CHECK_DEVICE_INFO;
}

static WUDStackState __wudStackDone() {
    WUDCB* p = &__rvl_wudcb;

    if (p->linkKeyState == WUD_STATE_LINK_KEY_START) {
        OSCancelAlarm(&p->alarm);
        __wudCheckFirmwareVersion();
        return WUD_STATE_STACK_INITIALIZED;
    }

    return WUD_STATE_STACK_CHECK_DEVICE_INFO;
}

static void __wudStackHandler() {
    WUDCB* p = &__rvl_wudcb;

    switch (p->stackState) {
        case WUD_STATE_STACK_GET_STORED_LINK_KEY: {
            p->stackState = __wudStackGetStoredLinkKey();
            break;
        }
        case WUD_STATE_STACK_CHECK_DEVICE_INFO: {
            p->stackState = __wudStackCheckDeviceInfo();
            break;
        }
        case WUD_STATE_STACK_DONE: {
            p->stackState = __wudStackDone();
            break;
        }
    }
}

static void __wudStackHandler0(OSAlarm* pAlarm, OSContext* pContext) {
    OSSwitchFiberEx((u32)pAlarm, (u32)pContext, 0, 0, __wudStackHandler, _wudHandlerStack + sizeof(_wudHandlerStack));
}

static void __wudInitFlushCallback(SCStatus status) {
    DEBUGPrint("__wudInitFlushCallback() : %d, Init: %d\n", status, __rvl_wudcb.initState);
    __rvl_wudcb.initState = WUD_STATE_INIT_DONE;
}

static WUDInitState __wudInitWaitForInitialization() {
    s32 diff = 0;
    WUDInitState nextState = WUD_STATE_INIT_WAIT_FOR_INITIALIZATION;
    s64 time = __OSGetSystemTime();

    diff = 500 - OSTicksToMilliseconds(OSDiffTick(time, __OSStartTime));

    if (diff < 0 && SCCheckStatus() != SC_STATUS_BUSY) {
        __wudClearControlBlock();
        nextState = WUD_STATE_INIT_GET_DEV_INFO;

        if (WUDIsLinkedWBC() && SCGetProductGameRegion() == SC_PRODUCT_GAME_REGION_JP) {
            nextState = WUD_STATE_INIT_GET_DEV_INFO_FROM_WII_FIT;
            _wudNandLocked = FALSE;
            _wudNandPhase = 0;
        }
    }

    return nextState;
}

static BOOL __wudIsExistedDevice(u8* addr) {
    int i;

    for (i = 0; i < _scArray.numRegist; i++) {
        if (WUD_BDCMP(_scArray.devices[i].addr, addr) == 0) {
            return TRUE;
        }
    }

    return FALSE;
}



static WUDInitState __wudInitDevInfo() {
    int i, j;
    int arrayNum;
    int smpNum;
    u8 a;
    WUDDevInfo* pInfo;
    BD_ADDR emptyAddr;

    memset(&_scArray, 0, sizeof(_scArray));
    memset(&_spArray, 0, sizeof(_spArray));
    memset(emptyAddr, 0, sizeof(emptyAddr));

    SCGetBtDeviceInfoArray(&_scArray);
    SCGetBtCmpDevInfoArray(&_spArray);

    __rvl_wudcb.syncType = 0;

    arrayNum = _scArray.numRegist;

    for (i = 0, a = 0; i < WUD_MAX_DEV_ENTRY_FOR_STD; i++) {
        if (arrayNum == 0) {
            break;
        }

        if (!WUD_DEV_NAME_IS_CNT(_scArray.devices[i].info.devName)) {
            memset(&_scArray.devices[i], 0, sizeof(SCBtDeviceInfo));
        }

        if (WUD_BDCMP(_scArray.devices[i].addr, emptyAddr) == 0) {
            if (i < WUD_MAX_DEV_ENTRY_FOR_STD - 1) {
                for (j = i + 1; j < WUD_MAX_DEV_ENTRY_FOR_STD; j++) {
                    if (WUD_DEV_NAME_IS_CNT(_scArray.devices[j].info.devName)) {
                        memcpy(&_scArray.devices[i], &_scArray.devices[j], sizeof(SCBtDeviceInfo));
                        memset(&_scArray.devices[j], 0, sizeof(SCBtDeviceInfo));
                        goto there;
                    }
                }
            }
            continue;
        }

    there:
        pInfo = __wudGetNewDevInfo();
        if (pInfo == NULL) {
            continue;
        }

        WUD_BDCPY(pInfo->devAddr, _scArray.devices[i].addr);
        memcpy(&pInfo->conf, &_scArray.devices[i].info, sizeof(SC_BT_DEV_INFO));

        pInfo->status = 1;
        pInfo->UNK_0x5B = 0;
        pInfo->UNK_0x5C = 2;
        pInfo->subclass = 2;
        pInfo->hhAttrMask = 0x8074;
        pInfo->appID = 3;

        DEBUGPrint("addr : %02x:%02x:%02x:%02x:%02x:%02x\n", pInfo->devAddr[0], pInfo->devAddr[1], pInfo->devAddr[2], pInfo->devAddr[3],
                   pInfo->devAddr[4], pInfo->devAddr[5]);
        DEBUGPrint("name : %s\n", pInfo->conf.devName);

        a++;
        arrayNum--;
    }

    if (WUDIsLinkedWBC() && WUD_DEV_NAME_IS_WBC(_scArray.wbc.info.devName)) {
        pInfo = __wudGetNewDevInfo();
        if (pInfo == NULL) {
            // no room left: the balance board takes the last entry
            pInfo = __wudGetDevInfoByIndex(WUD_MAX_DEV_ENTRY_FOR_STD - 1);
            a--;
        }

        WUD_BDCPY(pInfo->devAddr, _scArray.wbc.addr);
        memcpy(pInfo->conf.devName, _scArray.wbc.info.devName, sizeof("Nintendo RVL-WBC-01") - 1);
        memcpy(pInfo->linkKey, &_scArray.wbc.info.unk_0x14, LINK_KEY_LEN);

        pInfo->status = 1;
        pInfo->UNK_0x5B = 0;
        pInfo->subclass = 2;
        pInfo->hhAttrMask = 0x8074;
        pInfo->appID = 3;
        pInfo->UNK_0x5C = 3;

        DEBUGPrint("addr : %02x:%02x:%02x:%02x:%02x:%02x\n", pInfo->devAddr[0], pInfo->devAddr[1], pInfo->devAddr[2], pInfo->devAddr[3],
                   pInfo->devAddr[4], pInfo->devAddr[5]);
        DEBUGPrint("name : %s\n", pInfo->conf.devName);

        memset(&_scArray.devices[a], 0, sizeof(SCBtDeviceInfo));
        WUD_BDCPY(_scArray.devices[a].addr, _scArray.wbc.addr);
        memcpy(_scArray.devices[a].info.devName, _scArray.wbc.info.devName, sizeof("Nintendo RVL-WBC-01") - 1);

        a++;
    }

    _scArray.numRegist = a;

    __rvl_wudcb.syncType = WUD_SYNC_TYPE_SIMPLE;

    for (i = WUD_MAX_DEV_ENTRY_FOR_SMP - 1, smpNum = _spArray.numRegist; i >= 0; i--) {
        if (smpNum == 0) {
            break;
        }

        if (WUD_BDCMP(_spArray.devices[i].addr, emptyAddr) == 0) {
            continue;
        }

        if (__wudIsExistedDevice(_spArray.devices[i].addr)) {
            continue;
        }

        pInfo = __wudGetNewDevInfo();
        if (pInfo == NULL) {
            continue;
        }

        WUD_BDCPY(pInfo->devAddr, _spArray.devices[i].addr);
        memcpy(&pInfo->conf, &_spArray.devices[i].name, sizeof(SC_BT_DEV_INFO));
        memcpy(&pInfo->linkKey, &_spArray.devices[i].linkKey, sizeof(LINK_KEY));

        pInfo->status = 1;
        pInfo->UNK_0x5B = 1;
        pInfo->UNK_0x5C = 3;

        if (WUD_DEV_NAME_IS_CNT_01(pInfo->conf.devName)) {
            pInfo->subclass = 2;
            pInfo->hhAttrMask = 0x8074;
            pInfo->appID = 3;
        }

        WUD_DEBUGPrint("addr : %02x:%02x:%02x:%02x:%02x:%02x\n", pInfo->devAddr[0], pInfo->devAddr[1], pInfo->devAddr[2], pInfo->devAddr[3],
                       pInfo->devAddr[4], pInfo->devAddr[5]);
        WUD_DEBUGPrint("name : %s\n", pInfo->conf.devName);

        WUDiMoveTopSmpDevInfoPtr(pInfo);

        smpNum--;
    }

    __rvl_wudcb.syncType = WUD_SYNC_TYPE_STANDARD;
    __rvl_wudcb.initState = WUD_STATE_INIT_WAIT_FOR_FLUSH;

    memset(&_spArray, 0, sizeof(_spArray));

    SCSetBtDeviceInfoArray(&_scArray);
    SCSetBtCmpDevInfoArray(&_spArray);
    SCFlushAsync(__wudInitFlushCallback);

    return WUD_STATE_INIT_DONE;
}

static WUDInitState __wudInitDone() {
    WUDCB* p = &__rvl_wudcb;

    OSCancelAlarm(&p->alarm);
    p->libStatus = WUD_LIB_STATUS_1;

    BTA_EnableBluetooth(__wudSecurityEventStackCallback);
    return WUD_STATE_INIT_INITIALIZED;
}

static void __wudNandResultCallback(s32 result, NANDCommandBlock* pBlock) {
    _wudNandLocked = FALSE;

    switch (_wudNandPhase) {
        case WUD_STATE_NAND_OPEN: {
            _wudNandPhase = result == NAND_RESULT_OK ? _wudNandPhase + 1 : 0xFF;
            break;
        }
        case WUD_STATE_NAND_SEEK: {
            _wudNandPhase = result == 0 ? _wudNandPhase + 1 : WUD_STATE_NAND_CLOSE;
            break;
        }
        case WUD_STATE_NAND_READ: {
            _wudNandPhase = result == WUD_NAND_BUFFER_SIZE ? _wudNandPhase + 1 : WUD_STATE_NAND_CLOSE;
            break;
        }
        default: {
            _wudNandPhase = WUD_STATE_NAND_DONE;
            break;
        }
    }
}

static void __wudNandFlushCallback(SCStatus status) {
    _wudNandLocked = FALSE;
    _wudNandPhase++;
}

static u32 __wudCalcWiiFitCrc1(void* pData, u32 length) {
    u16* p = pData;
    u32 hi = 0;
    u16 lo = 0;
    u32 i;
    u32 num = length / sizeof(u16);

    for (i = 0; i < num; i++, p++) {
        hi += *p;
        lo += ~*p;
    }

    return (u16)hi << 16 | lo;
}

static u32 __wudCalcWiiFitCrc2(void* pData, u32 length, u32 crc) {
    u16* p = pData;
    u32 hi = (u16)(crc >> 16);
    u16 lo = (u16)crc;
    u32 i;
    u32 num = length / sizeof(u16);

    for (i = 0; i < num; i++, p++) {
        hi += *p;
        lo += ~*p;
    }

    return (u16)hi << 16 | lo;
}

// Reads the balance board's pairing information out of Wii Fit's save file into SYSCONF.
static WUDInitState __wudGetDevInfoFromWiiFit(void) {
    const char* path = RP_HEALTH_FILE_PATH;
    const char* gameName = "RFNJ";
    u8 zero[16] = {0};
    WUDInitState nextState = WUD_STATE_INIT_GET_DEV_INFO_FROM_WII_FIT;
    s32 result;
    u32 crc;
    u32 calcCrc;

    if (_wudNandLocked) {
        return nextState;
    }

    if (!_wudNandBufPtr || (u32)_wudNandBufPtr % 32 != 0) {
        OSErrorLine(2701, "Error: the workarea for synchronizing WBC pairing information is not allocated.\n");
    }

    switch (_wudNandPhase) {
        case 0: {
            result = NANDInit();
            _wudNandPhase = result == NAND_RESULT_OK ? WUD_STATE_NAND_OPEN : 0xFF;
            DEBUGPrint("NANDInit. [%d]\n", result);
            break;
        }
        case WUD_STATE_NAND_OPEN: {
            if (SCGetProductGameRegion() == SC_PRODUCT_GAME_REGION_JP) {
                DEBUGPrint("WbcSaveData: %s\n", path);

                _wudNandLocked = TRUE;
                _wudNandWbcCrc = 0;
                NANDOpenAsync(path, &_wudNandFileInfo, NAND_ACCESS_READ, __wudNandResultCallback, &_wudNandBlock);
            } else {
                _wudNandPhase = WUD_STATE_NAND_DONE;
            }
            break;
        }
        case WUD_STATE_NAND_SEEK: {
            _wudNandLocked = TRUE;
            NANDSeekAsync(&_wudNandFileInfo, 0, 0, __wudNandResultCallback, &_wudNandBlock);
            break;
        }
        case WUD_STATE_NAND_READ: {
            _wudNandLocked = TRUE;
            NANDReadAsync(&_wudNandFileInfo, _wudNandBufPtr, WUD_NAND_BUFFER_SIZE, __wudNandResultCallback, &_wudNandBlock);
            break;
        }
        case WUD_STATE_NAND_GET_DEV_INFO: {
            memcpy(&_wudNandWbcInfo, _wudNandBufPtr + RP_HEALTH_FILE_OFFSET, RP_HEALTH_WBC_INFO_SIZE);
            memcpy(&crc, _wudNandBufPtr + RP_HEALTH_FILE_OFFSET + RP_HEALTH_WBC_INFO_SIZE, sizeof(crc));
            DEBUGPrint("CRC  = %d:%08x\n", crc, crc);

            _wudNandWbcCrc = __wudCalcWiiFitCrc1(_wudNandBufPtr, RP_HEALTH_FILE_OFFSET);
            calcCrc = __wudCalcWiiFitCrc2(_wudNandBufPtr + RP_HEALTH_FILE_OFFSET, RP_HEALTH_WBC_INFO_SIZE, _wudNandWbcCrc);
            DEBUGPrint("CRC' = %d:%08x\n", calcCrc, calcCrc);

            if (crc != calcCrc) {
                _wudNandPhase = WUD_STATE_NAND_CLOSE;
                DEBUGPrint("CRC Error on WiiFit SaveData.\n");
                break;
            }

            if (WUD_BDCMP(_wudNandWbcInfo.devAddr, zero) == 0) {
                _wudNandPhase = WUD_STATE_NAND_CLOSE;
                DEBUGPrint("WiiFit does not register WBC yet.\n");
                break;
            }

            if (memcmp(OSGetAppGamename(), gameName, 4) != 0 && memcmp(_wudNandWbcInfo.linkKey, zero, LINK_KEY_LEN) == 0) {
                _wudNandPhase = WUD_STATE_NAND_CLOSE;
                DEBUGPrint("LinkKey saved in WiiFit SaveData is invalid.\n");
                break;
            }

            memset(_wudNandBufPtr, 0, WUD_NAND_BUFFER_SIZE);

            memset(&_scArray, 0, sizeof(_scArray));
            SCGetBtDeviceInfoArray(&_scArray);

            WUD_BDCPY(_scArray.wbc.addr, _wudNandWbcInfo.devAddr);
            memcpy(&_scArray.wbc.info, _wudNandWbcInfo.devName, sizeof(SC_BT_DEV_INFO));
            memcpy(&_scArray.wbc.info.unk_0x14, _wudNandWbcInfo.linkKey, LINK_KEY_LEN);

            DEBUGPrint("WBC from WiiFit\n");
            DEBUGPrint(" %02x %02x %02x %02x %02x %02x\n", _wudNandWbcInfo.devAddr[0], _wudNandWbcInfo.devAddr[1],
                       _wudNandWbcInfo.devAddr[2], _wudNandWbcInfo.devAddr[3], _wudNandWbcInfo.devAddr[4], _wudNandWbcInfo.devAddr[5]);
            DEBUGPrint(" %02x %02x %02x %02x %02x %02x %02x %02x\n", _wudNandWbcInfo.linkKey[0], _wudNandWbcInfo.linkKey[1],
                       _wudNandWbcInfo.linkKey[2], _wudNandWbcInfo.linkKey[3], _wudNandWbcInfo.linkKey[4], _wudNandWbcInfo.linkKey[5],
                       _wudNandWbcInfo.linkKey[6], _wudNandWbcInfo.linkKey[7]);
            DEBUGPrint(" %02x %02x %02x %02x %02x %02x %02x %02x\n", _wudNandWbcInfo.linkKey[8], _wudNandWbcInfo.linkKey[9],
                       _wudNandWbcInfo.linkKey[10], _wudNandWbcInfo.linkKey[11], _wudNandWbcInfo.linkKey[12], _wudNandWbcInfo.linkKey[13],
                       _wudNandWbcInfo.linkKey[14], _wudNandWbcInfo.linkKey[15]);
            DEBUGPrint(" %s\n", _wudNandWbcInfo.devName);

            SCSetBtDeviceInfoArray(&_scArray);

            _wudNandLocked = TRUE;
            SCFlushAsync(__wudNandFlushCallback);
            break;
        }
        case WUD_STATE_NAND_CLOSE: {
            _wudNandLocked = TRUE;
            NANDCloseAsync(&_wudNandFileInfo, __wudNandResultCallback, &_wudNandBlock);
            break;
        }
        default: {
            nextState = WUD_STATE_INIT_GET_DEV_INFO;
            break;
        }
    }

    return nextState;
}

static void __wudInitHandler() {
    WUDCB* p = &__rvl_wudcb;

    switch (p->initState) {
        case WUD_STATE_INIT_WAIT_FOR_INITIALIZATION: {
            p->initState = __wudInitWaitForInitialization();
            break;
        }
        case WUD_STATE_INIT_GET_DEV_INFO: {
            __wudInitDevInfo();
            break;
        }
        case WUD_STATE_INIT_DONE: {
            p->initState = __wudInitDone();
            break;
        }
        case WUD_STATE_INIT_GET_DEV_INFO_FROM_WII_FIT: {
            p->initState = __wudGetDevInfoFromWiiFit();
            break;
        }
    }
}

static void __wudInitHandler0(OSAlarm* pAlarm, OSContext* pContext) {
    OSSwitchFiberEx((u32)pAlarm, (u32)pContext, 0, 0, __wudInitHandler, _wudHandlerStack + sizeof(_wudHandlerStack));
}

static void __wudShutdownFlushCallback(SCStatus status) {
    WUDCB* p = &__rvl_wudcb;

    DEBUGPrint("__wudShutdownFlushCallback() : %d, Shutdown: %d\n", status, __rvl_wudcb.shutdownState);

    p->shutdownState = WUD_STATE_SHUTDOWN_DONE;
}

static void __wudShutdownStoreSetting() {
    WUDCB* p = &__rvl_wudcb;
    BOOL result = _wudReadNand;

    if (SCCheckStatus() == SC_STATUS_BUSY) {
        return;
    }

    result &= SCSetBtDeviceInfoArray(&_scArray);
    result &= SCSetBtCmpDevInfoArray(&_spArray);

    if (result) {
        p->shutdownState = WUD_STATE_SHUTDOWN_FLUSH_SETTINGS;
        SCFlushAsync(__wudShutdownFlushCallback);
    } else {
        p->shutdownState = WUD_STATE_SHUTDOWN_DONE;
    }
}

static void __wudShutdownDone() {
    OSCancelAlarm(&__rvl_wudcb.alarm);
    BTA_DisableBluetooth();
}

static void __wudShutdownHandler() {
    WUDCB* p = &__rvl_wudcb;

    switch (p->shutdownState) {
        case WUD_STATE_SHUTDOWN_STORE_SETTINGS: {
            __wudShutdownStoreSetting();
            break;
        }
        case WUD_STATE_SHUTDOWN_DONE: {
            __wudShutdownDone();
            break;
        }
    }
}

static void __wudShutdownHandler0(OSAlarm* pAlarm, OSContext* pContext) {
    OSSwitchFiberEx((u32)pAlarm, (u32)pContext, 0, 0, __wudShutdownHandler, _wudHandlerStack + sizeof(_wudHandlerStack));
}

static void __wudClearControlBlock() {
    WUDCB* p = &__rvl_wudcb;
    int i;

    DEBUGPrint("__wudClearControlBlock\n");

    for (i = 0; i < WUD_MAX_DEV_ENTRY; i++) {
        WUDiSetDevAddrForHandle(i, NULL);
        WUDiSetQueueSizeForHandle(i, 0);
        WUDiSetNotAckNumForHandle(i, 0);
    }

    p->smpListTail = &p->smpList[WUD_MAX_DEV_ENTRY_FOR_SMP - 1];
    p->smpListHead = &p->smpList[0];

    for (i = 0; i < WUD_MAX_DEV_ENTRY_FOR_SMP; i++) {
        p->smpList[i].devInfo = &p->smpDevs[WUD_MAX_DEV_ENTRY_FOR_SMP - 1 - i];
        p->smpList[i].prev = i == 0 ? NULL : &p->smpList[i - 1];
        p->smpList[i].next = i == WUD_MAX_DEV_ENTRY_FOR_SMP - 1 ? NULL : &p->smpList[i + 1];
    }

    p->stdListTail = &p->stdList[WUD_MAX_DEV_ENTRY_FOR_STD - 1];
    p->stdListHead = &p->stdList[0];

    for (i = 0; i < WUD_MAX_DEV_ENTRY_FOR_STD; i++) {
        p->stdList[i].devInfo = &p->stdDevs[i];
        p->stdList[i].prev = i == 0 ? NULL : &p->stdList[i - 1];
        p->stdList[i].next = i == WUD_MAX_DEV_ENTRY_FOR_STD - 1 ? NULL : &p->stdList[i + 1];
    }

    p->syncState = WUD_STATE_SYNC_START;
    p->linkKeyState = WUD_STATE_LINK_KEY_START;
    p->deleteState = WUD_STATE_DELETE_START;
    p->stackState = WUD_STATE_STACK_START;
    p->initState = WUD_STATE_INIT_START;
    p->shutdownState = WUD_STATE_SHUTDOWN_START;

    p->syncSkipChecks = FALSE;
    p->syncType = WUD_SYNC_TYPE_STANDARD;
    p->syncLoopNum = 1;

    p->connectable = FALSE;
    p->discoverable = FALSE;

    p->libStatus = WUD_LIB_STATUS_0;
    p->pmID = 0;
    p->syncRssi = -65;

    memset(p->hostAddr, 0, BD_ADDR_LEN);
    memset(p->pairAddr, 0, BD_ADDR_LEN);

    p->bufferStatus0 = 0;
    p->bufferStatus1 = 10;

    __wudClearDevInfos();
}

/* PUBLIC API */

extern void bta_sys_set_trace_level(UINT8 level);

BOOL WUDInit() {
    WUDCB* p = &__rvl_wudcb;

    if (_wudInitialized) {
        return FALSE;
    }

    DEBUGPrint("BTA_Init() is started\n");

    BTA_Init();
    bta_sys_set_trace_level(__bte_trace_level);
    L2CA_SetTraceLevel(__bte_trace_level);
    SDP_SetTraceLevel(__bte_trace_level);

    DEBUGPrint("BTA_Init() is done\n");

    p->syncStdCB = NULL;
    p->syncSmpCB = NULL;
    p->clearDevCB = NULL;

    p->hidConnCB = NULL;
    p->hidRecvCB = NULL;

    p->initState = WUD_STATE_INIT_WAIT_FOR_INITIALIZATION;

    SCInit();

    if (WUDIsLinkedWBC() && _initWBC != NULL) {
        _initWBC();
    }

    OSCreateAlarm(&p->alarm);
    OSSetPeriodicAlarm(&p->alarm, OSGetTime(), OSMillisecondsToTicks(10), __wudInitHandler0);

    _wudInitialized = TRUE;
    _scFlush = FALSE;

    return TRUE;
}

void WUDRegisterAllocator(WUDAllocFunc pAllocFunc, WUDFreeFunc pFreeFunc) {
    WUDCB* p = &__rvl_wudcb;
    BOOL enabled;

    enabled = OSDisableInterrupts();

    p->allocFunc = pAllocFunc;
    p->freeFunc = pFreeFunc;

    OSRestoreInterrupts(enabled);
}

void WUDShutdown(BOOL saveSmp) {
    WUDCB* p = &__rvl_wudcb;
    BOOL enabled;
    int i;
    WUDDevInfoList* pIt;

    DEBUGPrint("WUDShutdown()\n");

    enabled = OSDisableInterrupts();

    if (WUDIsBusy()) {
        OSCancelAlarm(&p->alarm);
    }

    memset(_scArray.devices, 0, sizeof(SCBtDeviceInfo) * WUD_MAX_DEV_ENTRY_FOR_STD);

    for (i = 0, pIt = __rvl_wudcb.stdListHead; pIt != NULL; pIt = pIt->next, i++) {
        WUD_BDCPY(_scArray.devices[i].addr, pIt->devInfo->devAddr);

        memcpy(&_scArray.devices[i].info, &pIt->devInfo->conf, sizeof(SC_BT_DEV_INFO));
    }

    _scArray.numRegist = __wudGetStdDevNumber();

    memset(_spArray.devices, 0, sizeof(SCBtCmpDevInfo) * WUD_MAX_DEV_ENTRY_FOR_SMP);

    if (saveSmp) {
        for (i = 0, pIt = __rvl_wudcb.smpListHead; pIt != NULL; pIt = pIt->next, i++) {
            WUD_BDCPY(_spArray.devices[i].addr, pIt->devInfo->devAddr);
            memcpy(_spArray.devices[i].name, pIt->devInfo->conf.devName, sizeof(_spArray.devices[i].name));

            memcpy(&_spArray.devices[i].linkKey, &pIt->devInfo->linkKey, LINK_KEY_LEN);
        }

        _spArray.numRegist = __wudGetSmpDevNumber();
    } else {
        _spArray.numRegist = 0;
    }

    p->shutdownState = WUD_STATE_SHUTDOWN_STORE_SETTINGS;

    OSCreateAlarm(&p->alarm);
    OSSetPeriodicAlarm(&p->alarm, OSGetTime(), OSMillisecondsToTicks(10), __wudShutdownHandler0);

    p->libStatus = WUD_LIB_STATUS_4;

    OSRestoreInterrupts(enabled);
}

WUDLibStatus WUDGetStatus() {
    WUDCB* p = &__rvl_wudcb;
    BOOL enabled = OSDisableInterrupts();

    s8 libStatus = p->libStatus;

    OSRestoreInterrupts(enabled);
    return libStatus;
}

u8 WUDGetBufferStatus() {
    WUDCB* p = &__rvl_wudcb;
    u8 status;
    BOOL enabled = OSDisableInterrupts();

    status = p->bufferStatus1 - p->bufferStatus0;

    OSRestoreInterrupts(enabled);
    return status;
}

void WUDSetSniffMode(BD_ADDR addr, s32 interval) {
    WUDCB* p = &__rvl_wudcb;
    tBTM_PM_PWR_MD block;

    if (interval > 0) {
        block.mode = BTM_PM_MD_SNIFF;
    } else {
        block.mode = BTM_PM_MD_ACTIVE;
    }

    block.max = interval;
    block.min = interval;
    block.attempt = 1;
    block.timeout = 0;

    BTM_SetPowerMode(p->pmID, addr, &block);
}

WUDSyncDeviceCallback WUDSetSyncSimpleCallback(WUDSyncDeviceCallback pCallback) {
    WUDCB* p = &__rvl_wudcb;
    BOOL enabled;
    WUDSyncDeviceCallback pOldCallback;

    DEBUGPrint("WUDSetSyncDeviceCallback\n");

    enabled = OSDisableInterrupts();

    pOldCallback = p->syncSmpCB;
    p->syncSmpCB = pCallback;

    OSRestoreInterrupts(enabled);
    return pOldCallback;
}

WUDClearDeviceCallback WUDSetClearDeviceCallback(WUDClearDeviceCallback pCallback) {
    WUDCB* p = &__rvl_wudcb;
    BOOL enabled;
    WUDClearDeviceCallback pOldCallback;

    DEBUGPrint("WUDSetClearDeviceCallback\n");

    enabled = OSDisableInterrupts();

    pOldCallback = p->clearDevCB;
    p->clearDevCB = pCallback;

    OSRestoreInterrupts(enabled);
    return pOldCallback;
}

static BOOL __wudStartSyncDevice(u8 syncType, s8 syncLoopNum, u8 target, BOOL syncSkipChecks) {
    WUDCB* p = &__rvl_wudcb;
    BOOL success = FALSE;
    BOOL enabled;
    u32 libStatus;

    enabled = OSDisableInterrupts();
    libStatus = p->libStatus;
    OSRestoreInterrupts(enabled);

    if (libStatus == WUD_LIB_STATUS_3 && !WUDIsBusy()) {
        enabled = OSDisableInterrupts();
        _wudTarget = target;
        p->syncState = WUD_STATE_SYNC_PREPARE_SEARCH;
        p->syncLoopNum = syncLoopNum;
        p->syncType = syncType;
        p->syncSkipChecks = syncSkipChecks ? TRUE : FALSE;
        p->syncedNum = 0;
        p->waitStartSearchFrames = 50;
        p->waitIncomingFrames = 200;

        OSCreateAlarm(&p->alarm);
        OSSetPeriodicAlarm(&p->alarm, OSGetTime(), OSMillisecondsToTicks(20), __wudSyncHandler0);

        OSRestoreInterrupts(enabled);

        success = TRUE;
    }

    return success;
}

static BOOL __wudStartSyncStandard(BOOL syncSkipChecks) {
    return __wudStartSyncDevice(WUD_SYNC_TYPE_STANDARD, 3, 0, syncSkipChecks);
}

BOOL WUDStartSyncDevice(void) {
    return __wudStartSyncStandard(FALSE);
}

static BOOL __wudStartSyncSimple(BOOL syncSkipChecks) {
    return __wudStartSyncDevice(WUD_SYNC_TYPE_SIMPLE, -1, 0, syncSkipChecks);
}

BOOL WUDStartFastSyncSimple(void) {
    return __wudStartSyncSimple(TRUE);
}

// not in the dol; body as in Petari
BOOL WUDStartSyncSpDevice(u8 target) {
    WUDCB* p = &__rvl_wudcb;
    BOOL success;
    BOOL enabled;
    WUDSyncDeviceCallback pSyncCallback;

    DEBUGPrint("WUDStartSyncSpDevice()\n");
    success = __wudStartSyncDevice(WUD_SYNC_TYPE_SIMPLE, -1, target, FALSE);

    enabled = OSDisableInterrupts();
    pSyncCallback = p->syncSmpCB;
    OSRestoreInterrupts(enabled);

    if (!success && pSyncCallback != NULL) {
        pSyncCallback(WUD_RESULT_SYNC_BUSY, 0);
    }

    return success;
}

static BOOL __wudStopSyncDevice(void) {
    WUDCB* p = &__rvl_wudcb;
    BOOL success = FALSE;
    BOOL enabled = OSDisableInterrupts();

    if (p->libStatus == WUD_LIB_STATUS_3) {
        if (p->syncLoopNum != 0) {
            if (p->syncState == WUD_STATE_SYNC_WAIT_FOR_SEARCH_RESULT) {
                BTA_DmSearchCancel();
            }

            p->syncLoopNum = 0;
        }

        success = TRUE;
    }

    OSRestoreInterrupts(enabled);
    return success;
}

BOOL WUDCancelSyncDevice(void) {
    _wudAbortSync = TRUE;

    return __wudStopSyncDevice();
}

BOOL WUDStopSyncSimple(void) {
    return __wudStopSyncDevice();
}

BOOL WUDStartClearDevice() {
    WUDCB* p = &__rvl_wudcb;
    BOOL success;
    BOOL enabled;
    u32 libStatus;

    success = FALSE;
    DEBUGPrint("WUDStartClearDevice()\n");

    enabled = OSDisableInterrupts();
    libStatus = p->libStatus;

    OSRestoreInterrupts(enabled);

    if (libStatus == WUD_LIB_STATUS_3 && !WUDIsBusy()) {
        enabled = OSDisableInterrupts();

        p->deleteState = WUD_STATE_DELETE_DISALLOW_INCOMING;

        OSCreateAlarm(&p->alarm);
        OSSetPeriodicAlarm(&p->alarm, OSGetTime(), OSMillisecondsToTicks(20), __wudDeleteHandler0);

        OSRestoreInterrupts(enabled);

        success = TRUE;
    }

    return success;
}

BOOL WUDSetDisableChannel(s8 afhChannel) {
    int first = 0;
    int last = 0;
    BOOL success = FALSE;
    WUDCB* p = &__rvl_wudcb;
    BOOL enabled;
    u32 libStatus;
    tBTM_STATUS status;

    DEBUGPrint("WUDSetDisableChannel()\n");

    if (afhChannel < 0 || 13 < afhChannel) {
        return FALSE;
    }

    enabled = OSDisableInterrupts();
    libStatus = p->libStatus;
    OSRestoreInterrupts(enabled);

    if (libStatus == WUD_LIB_STATUS_3) {
        if (!afhChannel) {
            first = 0xFF;
            last = 0xFF;
        } else {
            first = (afhChannel + 1) * 5 - 14;
            last = (afhChannel + 1) * 5 + 14;

            if (first < 0) {
                first = 0;
            }

            if (last > 78) {
                last = 78;
            }
        }

        status = BTM_SetAfhChannels(first, last);
        DEBUGPrint("BTM_SetAfhChannels() : %d\n", status);

        success = TRUE;
    }

    return success;
}

WUDHidRecvCallback WUDSetHidRecvCallback(WUDHidRecvCallback pCallback) {
    WUDCB* p = &__rvl_wudcb;
    BOOL enabled;
    WUDHidRecvCallback pOldCallback;

    DEBUGPrint("WUDSetHidRecvCallback()\n");

    enabled = OSDisableInterrupts();

    pOldCallback = p->hidRecvCB;
    p->hidRecvCB = pCallback;

    OSRestoreInterrupts(enabled);
    return pOldCallback;
}

WUDHidConnCallback WUDSetHidConnCallback(WUDHidConnCallback pCallback) {
    WUDCB* p = &__rvl_wudcb;
    BOOL enabled;
    WUDHidConnCallback pOldCallback;

    DEBUGPrint("WUDSetHidConnCallback()\n");

    enabled = OSDisableInterrupts();

    pOldCallback = p->hidConnCB;
    p->hidConnCB = pCallback;

    OSRestoreInterrupts(enabled);
    return pOldCallback;
}

void WUDSetVisibility(u8 disc, u8 conn) {
    WUDCB* p = &__rvl_wudcb;
    BOOL enabled = OSDisableInterrupts();

    p->discoverable = disc;
    p->connectable = conn;

    OSRestoreInterrupts(enabled);
    BTA_DmSetVisibility(disc, conn);
}

u8 WUDGetConnectable() {
    WUDCB* p = &__rvl_wudcb;
    u8 connectable;
    BOOL enabled = OSDisableInterrupts();

    connectable = p->connectable;

    OSRestoreInterrupts(enabled);
    return connectable;
}

static void __wudModuleRebootCallback(void* p1) {
    DEBUGPrint("Initialize the BCM2045 firmware complete.\n");
    __wudInitSub();
}

static void __wudModuleReboot(void) {
    DEBUGPrint("Reset again!\n");
    BTM_DeviceReset(__wudModuleRebootCallback);
}

static void __wudInstallPatchCallback(void* p1) {
    if ((_wudPatchNum == _wudInstallNum && p1 != NULL) || p1 == NULL) {
        __wudModuleReboot();
    } else {
        __wudInstallPatch();
    }
}

static void __wudInstallPatch(void) {
    u8 buf[WUD_PATCH_BUFFER_SIZE];
    u8 num;

    num = MIN(_wudPatchNum - _wudInstallNum, WUD_MAX_PATCHES);
    buf[0] = num;

    memcpy(&buf[sizeof(num)], _wudPatchInstallCmd.cmds[_wudInstallNum].data, num * sizeof(WUDPatchCmd));

    _wudInstallNum += num;

    DEBUGPrint("Install the patches\n");
    BTM_VendorSpecificCommand(BT_VSC_NINTENDO_INSTALL_PATCH, sizeof(num) + num * sizeof(WUDPatchCmd), buf, __wudInstallPatchCallback);
}

static void __wudWritePatchCallback(void* p1) {
    if (p1 != NULL) {
        if (_wudPatchSize == (u32)_wudPatchOffset) {
            _wudInstallNum = 0;
            _wudPatchNum = _wudPatchInstallCmd.num;
            __wudInstallPatch();
        } else {
            __wudWritePatch();
        }
    } else {
        __wudModuleReboot();
    }
}

static void __wudWritePatch(void) {
    u8 buf[WUD_PATCH_BUFFER_SIZE];
    u8 length;
    int i;

    length = MIN(_wudPatchSize - _wudPatchOffset, WUD_PATCH_BUFFER_SIZE - sizeof(u32));

    for (i = 0; i < (s32)sizeof(u32); i++) {
        buf[i] = (_wudPatchAddress + _wudPatchOffset) >> i * 8;
    }

    for (i = 0; i < (s32)length; i++) {
        buf[(s32)sizeof(u32) + i] = _wudPatchData[sizeof(_wudPatchSize) + sizeof(_wudPatchOffset) + _wudPatchOffset + i];
    }

    _wudPatchOffset += length;

    DEBUGPrint("Download the patch codes [%d]\n", _wudPatchOffset);
    BTM_VendorSpecificCommand(BT_VSC_NINTENDO_WRITE_PATCH, (s32)sizeof(u32) + length, buf, __wudWritePatchCallback);
}

static void __wudRemovePatchCallback(void* p1) {
    DEBUGPrint("RemovePatch Callback\n");

    if (p1 != NULL) {
        _wudPatchOffset = 0;
        __wudWritePatch();
    } else {
        __wudModuleReboot();
    }
}

static void __wudRemovePatch(void) {
    DEBUGPrint("RemovePatch\n");
    BTM_VendorSpecificCommand(BT_VSC_NINTENDO_INSTALL_PATCH, sizeof(_wudPatchRemoveCmd), &_wudPatchRemoveCmd, __wudRemovePatchCallback);
}

static void __wudSuperPeekPokeCallback(void* p1) {
    DEBUGPrint("SuperPeekPoke Callback\n");
    __wudRemovePatch();
}

static void __wudSuperPeekPoke(void) {
    DEBUGPrint("SuperPeekPoke\n");
    BTM_VendorSpecificCommand(BT_VSC_NINTENDO_SUPER_PEEK_POKE, sizeof(_wudSuperPeekPokeCmd), _wudSuperPeekPokeCmd, __wudSuperPeekPokeCallback);
}

static void __wudAppendRuntimePatch(void) {
    int i;

    DEBUGPrint("__wudAppendRuntimePatch()\n");

    _wudPatchSize = 0;
    _wudPatchAddress = 0;

    for (i = 0; i < (int)sizeof(u32); i++) {
        _wudPatchAddress <<= 8;
        _wudPatchAddress += _wudPatchData[3 - i];

        _wudPatchSize <<= 8;
        _wudPatchSize += _wudPatchData[7 - i];
    }

    if (__OSInIPL) {
        __wudSuperPeekPoke();
    } else {
        __wudRemovePatch();
    }
}

static void __wudCheckFirmwareVersion(void) {
    tBTM_VERSION_INFO version;
    BTM_ReadLocalVersion(&version);

    DEBUGPrint("hci_version   : %02x\n", version.hci_version);
    DEBUGPrint("hci_revision  : %04x\n", version.hci_revision);

    DEBUGPrint("lmp_version   : %02x\n", version.lmp_version);
    DEBUGPrint("lmp_subversion: %04x\n", version.lmp_subversion);

    DEBUGPrint("manufacturer  : %04x\n", version.manufacturer);

    DEBUGPrint(" ==> 2045 firmware ver.002.003.014.%d\n", version.hci_revision & 0xFFF);

    if ((version.hci_revision & 0xFFF) == 167) {
        __wudAppendRuntimePatch();
    } else {
        __wudInitSub();
    }
}

static void __wudInitSub(void) {
    WUDCB* p = &__rvl_wudcb;
    BOOL enabled;
    int i;

    char devName[] = "Wii";
    DEV_CLASS devClass = {
        0x00,  // No designated Major Service Classes
        0x04,  // Major Device Class 4 (Audio/Video)
        0x48   // Minor Device Class 18 (Audio/Video -> Gaming/Toy)
    };

    DEBUGPrint("start __wudInitSub()\n");

    BTA_DmSetDeviceName(devName);
    BTM_SetDeviceClass(devClass);

    BTM_RegisterForVSEvents(&__wudVendorSpecificEventStackCallback);
    BTM_RegisterForDeviceStatusNotif(&__wudDeviceStatusEventStackCallback);
    BTM_PmRegister(BTM_PM_REG_SET | BTM_PM_REG_NOTIF, &p->pmID, &__wudPowerMangeEventStackCallback);

    BTM_WritePageTimeout(32768);
    BTM_SetDefaultLinkPolicy(HCI_ENABLE_MASTER_SLAVE_SWITCH | HCI_ENABLE_SNIFF_MODE);
    BTM_SetDefaultLinkSuperTout(3200);

    for (i = 0; i < WUD_MAX_DEV_ENTRY_FOR_STD; i++) {
        if (p->stdDevs[i].status == 1) {
            WUDiRegisterDevice(p->stdDevs[i].devAddr);
        }
    }

    for (i = 0; i < WUD_MAX_DEV_ENTRY_FOR_SMP; i++) {
        if (p->smpDevs[i].status == 1) {
            WUDiRegisterDevice(p->smpDevs[i].devAddr);
        }
    }

    enabled = OSDisableInterrupts();
    p->libStatus = WUD_LIB_STATUS_3;
    _wudReadNand = TRUE;
    OSRestoreInterrupts(enabled);

    WUDSetVisibility(FALSE, TRUE);
}

extern void WUDiHidHostEventCallback(tBTA_HH_EVT event, tBTA_HH* pData);

static void __wudEnableStack(void) {
    WUDCB* p = &__rvl_wudcb;

    BTA_HhEnable(BTA_SEC_AUTHENTICATE, WUDiHidHostEventCallback);

    p->stackState = WUD_STATE_STACK_GET_STORED_LINK_KEY;

    OSCreateAlarm(&p->alarm);
    OSSetPeriodicAlarm(&p->alarm, OSGetTime(), OSMillisecondsToTicks(10), __wudStackHandler0);
}

static void __wudProcSyncEvent(void) {
    WUDCB* p = &__rvl_wudcb;
    WUDSyncDeviceCallback pSyncCallback;
    BOOL enabled;

    DEBUGPrint("__wudProcSyncEvent()\n");

    if (WUDIsBusy()) {
        return;
    }

    enabled = OSDisableInterrupts();
    pSyncCallback = p->syncStdCB;
    OSRestoreInterrupts(enabled);

    if (pSyncCallback != NULL) {
        pSyncCallback(WUD_RESULT_SYNC_WAITING, 0);
    } else {
        WUDStartSyncDevice();
    }
}


static void __wudProcDeleteEvent(void) {
    WUDCB* p = &__rvl_wudcb;
    WUDClearDeviceCallback pClearCallback;
    s32 result;
    BOOL enabled;

    DEBUGPrint("__wudProcDeleteEvent()\n");
    enabled = OSDisableInterrupts();

    pClearCallback = p->clearDevCB;
    result = WUDIsBusy() ? -1 /* WUD_RESULT_DELETE_BUSY */ : WUD_RESULT_DELETE_WAITING;

    OSRestoreInterrupts(enabled);

    if (pClearCallback != NULL) {
        pClearCallback(result);
    } else {
        WUDStartClearDevice();
    }
}

void WUDiRegisterDevice(BD_ADDR addr) {
    WUDCB* p = &__rvl_wudcb;
    WUDDevInfo* pInfo;
    tBTA_STATUS status;
    BOOL enabled = OSDisableInterrupts();
    ;

    pInfo = WUDiGetDevInfo(addr);

    status = BTA_DmAddDevice(pInfo->devAddr, pInfo->linkKey, 0, FALSE);
    DEBUGPrint("BTA_DmAddDevice(): %d\n", status);

    if (WUD_DEV_NAME_IS_CNT(pInfo->conf.devName) || (WUD_DEV_NAME_IS_WBC(pInfo->conf.devName) && WUDIsLinkedWBC())) {
        tHID_DEV_DSCP_INFO desc;
        desc.dl_len = sizeof(_wudWiiRemoteDescriptor);
        desc.dsc_list = _wudWiiRemoteDescriptor;

        DEBUGPrint("BTA_HhAddDev()\n");
        BTA_HhAddDev(pInfo->devAddr, pInfo->hhAttrMask, pInfo->subclass, pInfo->appID, desc);
    }

    if (pInfo->UNK_0x5B == 0 || pInfo->UNK_0x5B == 4 || pInfo->UNK_0x5B == 2 || pInfo->UNK_0x5B == 5) {
        p->devNums++;
    } else {
        p->devSmpNums++;
    }

    OSRestoreInterrupts(enabled);
}

void WUDiRemoveDevice(BD_ADDR addr) {
    WUDCB* p = &__rvl_wudcb;
    WUDDevInfo* pInfo;
    tBTM_STATUS status;
    BOOL enabled = OSDisableInterrupts();

    DEBUGPrint("WUDiRemoveDevice : \n");

    pInfo = WUDiGetDevInfo(addr);

    if (pInfo != NULL) {
        DEBUGPrint(" handle : %d,  addr : %02x:%02x:%02x:%02x:%02x:%02x\n", pInfo->devHandle, pInfo->devAddr[0], pInfo->devAddr[1], pInfo->devAddr[2],
                   pInfo->devAddr[3], pInfo->devAddr[4], pInfo->devAddr[5]);

        DEBUGPrint("remove device info from database.\n");

        if (WUD_DEV_NAME_IS_CNT(pInfo->conf.devName) || (WUD_DEV_NAME_IS_WBC(pInfo->conf.devName) && WUDIsLinkedWBC())) {
            DEBUGPrint("BTA_HhRemoveDev()\n");
            DEBUGPrint(" handle : %d\n", pInfo->devHandle);
            BTA_HhRemoveDev(pInfo->devHandle);
        }

        status = BTA_DmRemoveDevice(pInfo->devAddr);
        DEBUGPrint("BTA_DmRemoveDevice(): %d\n", status);

        if (pInfo->UNK_0x5B == 0 || pInfo->UNK_0x5B == 2 || pInfo->UNK_0x5B == 4 || pInfo->UNK_0x5B == 5) {
            p->devNums--;
        } else {
            p->devSmpNums--;
        }

        memset(pInfo, 0, sizeof *pInfo);
    }

    OSRestoreInterrupts(enabled);
}

WUDDevInfo* WUDiGetDevInfo(BD_ADDR addr) {
    WUDCB* p = &__rvl_wudcb;
    WUDDevInfo* pInfo;
    BOOL enabled;
    int i;

    pInfo = NULL;
    enabled = OSDisableInterrupts();

    for (i = 0; i < WUD_MAX_DEV_ENTRY_FOR_STD; i++) {
        if (WUD_BDCMP(p->stdDevs[i].devAddr, addr) == 0) {
            pInfo = &p->stdDevs[i];
            break;
        }
    }

    if (pInfo == NULL) {
        for (i = 0; i < WUD_MAX_DEV_ENTRY_FOR_SMP; i++) {
            if (WUD_BDCMP(p->smpDevs[i].devAddr, addr) == 0) {
                pInfo = &p->smpDevs[i];
                break;
            }
        }
    }

    OSRestoreInterrupts(enabled);
    return pInfo;
}

WUDDevInfo* __wudGetDevInfoByIndex(int idx) {
    WUDCB* p = &__rvl_wudcb;
    WUDDevInfo* pInfo;
    BOOL enabled;

    enabled = OSDisableInterrupts();

    if (0 <= idx && idx < WUD_MAX_DEV_ENTRY_FOR_STD) {
        pInfo = &p->stdDevs[idx];
    } else {
        pInfo = &p->smpDevs[idx - WUD_MAX_DEV_ENTRY_FOR_STD];
    }

    OSRestoreInterrupts(enabled);
    return pInfo;
}

static WUDDevInfo* __wudGetNewStdDevInfo(void) {
    WUDCB* p = &__rvl_wudcb;
    WUDDevInfo* pInfo = NULL;
    int i;

    for (i = 0; i < WUD_MAX_DEV_ENTRY_FOR_STD; i++) {
        if (p->stdDevs[i].status == 0) {
            DEBUGPrint("new entry index is %d\n", i);
            pInfo = &p->stdDevs[i];
            break;
        }
    }

    return pInfo;
}

static WUDDevInfo* __wudGetNewSmpDevInfo(void) {
    return __rvl_wudcb.smpListTail->devInfo;
}

static WUDDevInfo* __wudGetNewDevInfo(void) {
    WUDCB* p = &__rvl_wudcb;
    WUDDevInfo* pInfo;
    BOOL enabled;

    pInfo = NULL;
    enabled = OSDisableInterrupts();

    if (p->syncType == WUD_SYNC_TYPE_STANDARD) {
        pInfo = __wudGetNewStdDevInfo();
    } else {
        pInfo = __wudGetNewSmpDevInfo();
    }

    OSRestoreInterrupts(enabled);
    return pInfo;
}

void __wudRemoveDevInfo(BD_ADDR addr) {
    WUDDevInfo* pInfo;
    BOOL enabled;

    pInfo = WUDiGetDevInfo(addr);
    if (pInfo == NULL) {
        return;
    }

    enabled = OSDisableInterrupts();
    memset(pInfo, 0, sizeof(WUDDevInfo));
    OSRestoreInterrupts(enabled);
}

void __wudClearDevInfos() {
    WUDCB* p = &__rvl_wudcb;
    BOOL enabled;

    enabled = OSDisableInterrupts();

    memset(p->stdDevs, 0, sizeof(WUDDevInfo) * WUD_MAX_DEV_ENTRY_FOR_STD);
    memset(p->smpDevs, 0, sizeof(WUDDevInfo) * WUD_MAX_DEV_ENTRY_FOR_SMP);

    p->devNums = 0;
    p->devSmpNums = 0;

    p->connectedNum = 0;
    p->linkedNum = 0;
    p->syncedNum = 0;

    OSRestoreInterrupts(enabled);
}

u8 __wudGetStdDevNumber() {
    WUDCB* p = &__rvl_wudcb;
    BOOL enabled = OSDisableInterrupts();

    u8 num = p->devNums;

    OSRestoreInterrupts(enabled);
    return num;
}

u8 __wudGetSmpDevNumber() {
    WUDCB* p = &__rvl_wudcb;
    BOOL enabled = OSDisableInterrupts();

    u8 num = p->devSmpNums;

    OSRestoreInterrupts(enabled);
    return num;
}

u8 __wudGetConnectionNumber() {
    WUDCB* p = &__rvl_wudcb;
    BOOL enabled = OSDisableInterrupts();

    u8 num = p->connectedNum;

    OSRestoreInterrupts(enabled);
    return num;
}

u8 __wudGetLinkNumber() {
    WUDCB* p = &__rvl_wudcb;
    BOOL enabled = OSDisableInterrupts();

    u8 num = p->linkedNum;

    OSRestoreInterrupts(enabled);
    return num;
}

void WUDiMoveTopSmpDevInfoPtr(WUDDevInfo* pInfo) {
    WUDCB* p = &__rvl_wudcb;
    BOOL enabled;
    int i;

    enabled = OSDisableInterrupts();

    for (i = 0; i < WUD_MAX_DEV_ENTRY_FOR_SMP; i++) {
        if (WUD_BDCMP(p->smpList[i].devInfo->devAddr, pInfo->devAddr) != 0) {
            continue;
        }

        if (WUD_BDCMP(p->smpListHead->devInfo->devAddr, p->smpList[i].devInfo->devAddr) == 0) {
            break;
        }

        p->smpList[i].prev->next = p->smpList[i].next;

        if (WUD_BDCMP(p->smpListTail->devInfo->devAddr, p->smpList[i].devInfo->devAddr) == 0) {
            p->smpListTail = p->smpList[i].prev;
        } else {
            p->smpList[i].next->prev = p->smpList[i].prev;
        }

        p->smpList[i].next = p->smpListHead;
        p->smpListHead->prev = &p->smpList[i];
        p->smpListHead = &p->smpList[i];

        p->smpList[i].prev = NULL;

        break;
    }

    OSRestoreInterrupts(enabled);
}

void WUDiMoveBottomSmpDevInfoPtr(WUDDevInfo* pInfo) {
    WUDCB* p = &__rvl_wudcb;
    BOOL enabled;
    int i;

    enabled = OSDisableInterrupts();

    for (i = 0; i < WUD_MAX_DEV_ENTRY_FOR_SMP; i++) {
        if (WUD_BDCMP(p->smpList[i].devInfo->devAddr, pInfo->devAddr) != 0) {
            continue;
        }

        if (WUD_BDCMP(p->smpListTail->devInfo->devAddr, p->smpList[i].devInfo->devAddr) == 0) {
            break;
        }

        p->smpList[i].next->prev = p->smpList[i].prev;

        if (WUD_BDCMP(p->smpListHead->devInfo->devAddr, p->smpList[i].devInfo->devAddr) == 0) {
            p->smpListHead = p->smpList[i].next;
        } else {
            p->smpList[i].prev->next = p->smpList[i].next;
        }

        p->smpList[i].prev = p->smpListTail;
        p->smpListTail->next = &p->smpList[i];
        p->smpListTail = &p->smpList[i];

        p->smpList[i].next = NULL;

        break;
    }

    OSRestoreInterrupts(enabled);
}

WUDDevInfo* __wudGetOldestSmpDevice() {
    return __rvl_wudcb.smpListTail->devInfo;
}

void WUDiMoveTopOfDisconnectedSmpDevice(WUDDevInfo* pInfo) {
    WUDCB* p = &__rvl_wudcb;
    int i;
    BOOL enabled;
    WUDDevInfoList* pIt;

    enabled = OSDisableInterrupts();

    for (i = 0; i < WUD_MAX_DEV_ENTRY_FOR_SMP; i++) {
        if (WUD_BDCMP(p->smpList[i].devInfo->devAddr, pInfo->devAddr) != 0) {
            continue;
        }

        for (pIt = p->smpListHead; pIt; pIt = pIt->next) {
            if (WUD_BDCMP(pIt->devInfo->devAddr, pInfo->devAddr) == 0) {
                continue;
            }

            if (pIt->devInfo->status > 1) {
                continue;
            }

            if (WUD_BDCMP(p->smpListHead->devInfo->devAddr, p->smpList[i].devInfo->devAddr) == 0) {
                if (pIt == p->smpListHead->next) {
                    break;
                }

                p->smpListHead = p->smpList[i].next;
            } else {
                p->smpList[i].prev->next = p->smpList[i].next;
            }

            p->smpList[i].next->prev = p->smpList[i].prev;

            if (pIt != p->smpListHead) {
                p->smpList[i].prev = pIt->prev;
                p->smpList[i].next = pIt;

                pIt->prev->next = &p->smpList[i];
                pIt->prev = &p->smpList[i];
            } else {
                p->smpList[i].prev = pIt;
                p->smpList[i].next = pIt->next;

                pIt->next->prev = &p->smpList[i];
                pIt->next = &p->smpList[i];
            }

            break;
        }
    }

    OSRestoreInterrupts(enabled);
}

void WUDiMoveTopStdDevInfoPtr(WUDDevInfo* pInfo) {
    WUDCB* p = &__rvl_wudcb;
    BOOL enabled;
    int i;

    enabled = OSDisableInterrupts();

    for (i = 0; i < WUD_MAX_DEV_ENTRY_FOR_STD; i++) {
        if (WUD_BDCMP(p->stdList[i].devInfo->devAddr, pInfo->devAddr) != 0) {
            continue;
        }

        if (WUD_BDCMP(p->stdListHead->devInfo->devAddr, p->stdList[i].devInfo->devAddr) == 0) {
            break;
        }

        p->stdList[i].prev->next = p->stdList[i].next;

        if (WUD_BDCMP(p->stdListTail->devInfo->devAddr, p->stdList[i].devInfo->devAddr) == 0) {
            p->stdListTail = p->stdList[i].prev;
        } else {
            p->stdList[i].next->prev = p->stdList[i].prev;
        }

        p->stdList[i].next = p->stdListHead;
        p->stdListHead->prev = &p->stdList[i];
        p->stdListHead = &p->stdList[i];

        p->stdList[i].prev = NULL;

        break;
    }

    OSRestoreInterrupts(enabled);
}

void WUDiMoveBottomStdDevInfoPtr(WUDDevInfo* pInfo) {
    WUDCB* p = &__rvl_wudcb;
    BOOL enabled;
    int i;

    enabled = OSDisableInterrupts();

    for (i = 0; i < WUD_MAX_DEV_ENTRY_FOR_STD; i++) {
        if (WUD_BDCMP(p->stdList[i].devInfo->devAddr, pInfo->devAddr) != 0) {
            continue;
        }

        if (WUD_BDCMP(p->stdListTail->devInfo->devAddr, p->stdList[i].devInfo->devAddr) == 0) {
            break;
        }

        p->stdList[i].next->prev = p->stdList[i].prev;

        if (WUD_BDCMP(p->stdListHead->devInfo->devAddr, p->stdList[i].devInfo->devAddr) == 0) {
            p->stdListHead = p->stdList[i].next;
        } else {
            p->stdList[i].prev->next = p->stdList[i].next;
        }

        p->stdList[i].prev = p->stdListTail;
        p->stdListTail->next = &p->stdList[i];
        p->stdListTail = &p->stdList[i];

        p->stdList[i].next = NULL;

        break;
    }

    OSRestoreInterrupts(enabled);
}

WUDDevInfo* __wudGetOldestStdDevice() {
    return __rvl_wudcb.stdListTail->devInfo;
}

void WUDiMoveTopOfDisconnectedStdDevice(WUDDevInfo* pInfo) {
    WUDCB* p = &__rvl_wudcb;
    int i;
    BOOL enabled;
    WUDDevInfoList* pIt;

    enabled = OSDisableInterrupts();

    for (i = 0; i < WUD_MAX_DEV_ENTRY_FOR_STD; i++) {
        if (WUD_BDCMP(p->stdList[i].devInfo->devAddr, pInfo->devAddr) != 0) {
            continue;
        }

        for (pIt = p->stdListHead; pIt; pIt = pIt->next) {
            if (WUD_BDCMP(pIt->devInfo->devAddr, pInfo->devAddr) == 0) {
                continue;
            }

            // a connected device stays where it is, except the balance board
            if (pIt->devInfo->status != 1 && pIt->devInfo->status != 0 &&
                (WUD_DEV_NAME_IS_CNT(pIt->devInfo->conf.devName) || !WUDIsLinkedWBC())) {
                continue;
            }

            if (WUD_BDCMP(p->stdListHead->devInfo->devAddr, p->stdList[i].devInfo->devAddr) == 0) {
                if (pIt == p->stdListHead->next)
                    break;

                p->stdListHead = p->stdList[i].next;
            } else {
                p->stdList[i].prev->next = p->stdList[i].next;
            }

            p->stdList[i].next->prev = p->stdList[i].prev;

            if (pIt != p->stdListHead) {
                p->stdList[i].prev = pIt->prev;
                p->stdList[i].next = pIt;

                pIt->prev->next = &p->stdList[i];
                pIt->prev = &p->stdList[i];
            } else {
                p->stdList[i].prev = pIt;
                p->stdList[i].next = pIt->next;

                pIt->next->prev = &p->stdList[i];
                pIt->next = &p->stdList[i];
            }

            break;
        }
    }

    OSRestoreInterrupts(enabled);
}

void WUDiMoveTopOfUnusedStdDevice(WUDDevInfo* pInfo) {
    WUDCB* p = &__rvl_wudcb;
    int i;
    BOOL enabled;
    WUDDevInfoList* pIt;

    enabled = OSDisableInterrupts();

    for (i = 0; i < WUD_MAX_DEV_ENTRY_FOR_STD; i++) {
        if (WUD_BDCMP(p->stdList[i].devInfo->devAddr, pInfo->devAddr) != 0) {
            continue;
        }

        for (pIt = p->stdListHead; pIt; pIt = pIt->next) {
            if (WUD_BDCMP(pIt->devInfo->devAddr, pInfo->devAddr) == 0) {
                continue;
            }

            if (pIt->devInfo->status != 0 && pIt != p->stdListTail) {
                continue;
            }

            if (WUD_BDCMP(p->stdListHead->devInfo->devAddr, p->stdList[i].devInfo->devAddr) == 0) {
                if (pIt == p->stdListHead->next)
                    break;

                p->stdListHead = p->stdList[i].next;
            } else {
                p->stdList[i].prev->next = p->stdList[i].next;
            }

            p->stdList[i].next->prev = p->stdList[i].prev;

            if (pIt == p->stdListTail && pIt->devInfo->status != 0) {
                p->stdList[i].prev = pIt;
                p->stdList[i].next = NULL;

                pIt->next = &p->stdList[i];

                p->stdListTail = &p->stdList[i];
            } else if (pIt != p->stdListHead) {
                p->stdList[i].prev = pIt->prev;
                p->stdList[i].next = pIt;

                pIt->prev->next = &p->stdList[i];
                pIt->prev = &p->stdList[i];
            } else {
                p->stdList[i].prev = pIt;
                p->stdList[i].next = pIt->next;

                pIt->next->prev = &p->stdList[i];
                pIt->next = &p->stdList[i];
            }

            break;
        }
    }

    OSRestoreInterrupts(enabled);
}

BOOL WUDIsBusy() {
    WUDCB* p = &__rvl_wudcb;
    BOOL enabled;

    enabled = OSDisableInterrupts();

    if (p->syncState == WUD_STATE_SYNC_START && p->deleteState == WUD_STATE_DELETE_START && p->stackState == WUD_STATE_STACK_INITIALIZED &&
        p->initState == WUD_STATE_INIT_INITIALIZED) {
        OSRestoreInterrupts(enabled);
        return FALSE;
    }

    OSRestoreInterrupts(enabled);
    return TRUE;
}

static void __wudCleanupStackCallback(tBTA_STATUS status) {
    WUDCB* p = &__rvl_wudcb;

    if (status == BTA_SUCCESS) {
        _wudInitialized = FALSE;
        p->libStatus = WUD_LIB_STATUS_0;
        return;
    }

    DEBUGPrint("WARNING: USB_CLOSE_FAILURE!\n");
}

void __wudSecurityEventStackCallback(tBTA_DM_SEC_EVT event, tBTA_DM_SEC* pData) {
    WUDCB* p = &__rvl_wudcb;
    WUDDevInfo* pInfo;
    u8 i;

    tBTA_DM_ENABLE* pEnable;
    tBTA_DM_LINK_UP* pLinkUp;
    tBTA_DM_LINK_DOWN* pLinkDown;
    tBTA_DM_PIN_REQ* pPinReq;
    tBTA_DM_AUTH_CMPL* pAuthCmpl;

    DEBUGPrint("__wudCleanupStackCallback: ");

    switch (event) {
        case BTA_DM_ENABLE_EVT: {
            pEnable = &pData->enable;
            WUD_BDCPY(p->hostAddr, pEnable->bd_addr);

            DEBUGPrint("BTA_ENABLE_EVT\n");
            DEBUGPrint("host : %02x:%02x:%02x:%02x:%02x:%02x\n", p->hostAddr[0], p->hostAddr[1], p->hostAddr[2], p->hostAddr[3], p->hostAddr[4],
                       p->hostAddr[5]);

            __wudEnableStack();
            p->libStatus = WUD_LIB_STATUS_2;
            break;
        }
        case BTA_DM_DISABLE_EVT: {
            DEBUGPrint("BTA_DISABLE_EVT\n");

            BTA_CleanUp(__wudCleanupStackCallback);
            break;
        }
        case BTA_DM_PIN_REQ_EVT: {
            pPinReq = &pData->pin_req;

            DEBUGPrint("BTA_DM_PIN_REQ_EVT\n");

            __wudSetPinCode(pPinReq->bd_addr);
            break;
        }
        case BTA_DM_AUTH_CMPL_EVT: {
            pAuthCmpl = &pData->auth_cmpl;

            DEBUGPrint("BTA_DM_AUTH_CMPL_EVT\n");

            DEBUGPrint("  addr : %02x:%02x:%02x:%02x:%02x:%02x\n", pAuthCmpl->bd_addr[0], pAuthCmpl->bd_addr[1], pAuthCmpl->bd_addr[2],
                       pAuthCmpl->bd_addr[3], pAuthCmpl->bd_addr[4], pAuthCmpl->bd_addr[5]);

            DEBUGPrint("  key  : %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n", pAuthCmpl->key[0],
                       pAuthCmpl->key[1], pAuthCmpl->key[2], pAuthCmpl->key[3], pAuthCmpl->key[4], pAuthCmpl->key[5], pAuthCmpl->key[6],
                       pAuthCmpl->key[7], pAuthCmpl->key[8], pAuthCmpl->key[9], pAuthCmpl->key[10], pAuthCmpl->key[11], pAuthCmpl->key[12],
                       pAuthCmpl->key[13], pAuthCmpl->key[14], pAuthCmpl->key[15]);

            DEBUGPrint("  result = %d\n", pAuthCmpl->success);

            if (!pAuthCmpl->success) {
                break;
            }

            pInfo = WUDiGetDevInfo(pAuthCmpl->bd_addr);

            if (WUD_BDCMP(_wudDiscWork.devAddr, pAuthCmpl->bd_addr) == 0) {
                _wudDiscWork.status = 12;

                if (pInfo == NULL) {
                    pInfo = &_wudDiscWork;
                }
            }

            memcpy(pInfo->linkKey, pAuthCmpl->key, LINK_KEY_LEN);
            break;
        }
        case BTA_DM_AUTHORIZE_EVT: {
            DEBUGPrint("BTA_DM_AUTHORIZE_EVT\n");
            break;
        }
        case BTA_DM_LINK_UP_EVT: {
            pLinkUp = &pData->link_up;

            DEBUGPrint("BTA_DM_LINK_UP_EVT\n");

            DEBUGPrint("   addr : %02x:%02x:%02x:%02x:%02x:%02x\n", pLinkUp->bd_addr[0], pLinkUp->bd_addr[1], pLinkUp->bd_addr[2],
                       pLinkUp->bd_addr[3], pLinkUp->bd_addr[4], pLinkUp->bd_addr[5]);

            pInfo = WUDiGetDevInfo(pLinkUp->bd_addr);
            if (pInfo == NULL && WUD_BDCMP(pLinkUp->bd_addr, _wudDiscWork.devAddr) == 0) {
                pInfo = &_wudDiscWork;
            }

            if (pInfo == NULL || __wudGetLinkNumber() == WUD_MAX_CHANNELS) {
                btm_remove_acl(pLinkUp->bd_addr);

                DEBUGPrint("%s --> %02x:%02x:%02x:%02x:%02x:%02x\n", pInfo == NULL ? "not paired" : "4 links exist", pLinkUp->bd_addr[0],
                           pLinkUp->bd_addr[1], pLinkUp->bd_addr[2], pLinkUp->bd_addr[3], pLinkUp->bd_addr[4], pLinkUp->bd_addr[5]);
                break;
            }

            pInfo->status = pInfo->status == 2 ? 12 : 3;
            p->linkedNum++;
            break;
        }
        case BTA_DM_LINK_DOWN_EVT: {
            pLinkDown = &pData->link_down;

            DEBUGPrint("BTA_DM_LINK_DOWN_EVT\n");

            DEBUGPrint("   addr : %02x:%02x:%02x:%02x:%02x:%02x\n", pLinkDown->bd_addr[0], pLinkDown->bd_addr[1], pLinkDown->bd_addr[2],
                       pLinkDown->bd_addr[3], pLinkDown->bd_addr[4], pLinkDown->bd_addr[5]);

            DEBUGPrint("result: %d\n", pLinkDown->status);

            pInfo = WUDiGetDevInfo(pLinkDown->bd_addr);
            if (pInfo != NULL) {
                pInfo->status = 1;
                p->linkedNum--;

                if (WUD_BDCMP(_wudDiscWork.devAddr, pLinkDown->bd_addr) == 0) {
                    p->syncState = WUD_STATE_SYNC_ERROR;
                }

                if (pLinkDown->status == 0x15 /*HCI_ERR_PEER_POWER_OFF*/) {
                    for (i = 0; i < WUD_MAX_CHANNELS; i++) {
                        if (WUD_BDCMP(_scArray.devices[WUD_MAX_DEV_ENTRY_FOR_STD + i].addr, pLinkDown->bd_addr) == 0) {
                            memset(&_scArray.devices[WUD_MAX_DEV_ENTRY_FOR_STD + i], 0, sizeof(SCBtDeviceInfo));
                            _scFlush = TRUE;
                        }
                    }
                }
            } else if (WUD_BDCMP(_wudDiscWork.devAddr, pLinkDown->bd_addr) == 0) {
                p->syncState = WUD_STATE_SYNC_ERROR;
                p->linkedNum--;
            } else {
                DEBUGPrint("this device in not paired\n");
            }

            if (p->linkedNum <= 255 && p->linkedNum >= 250) {
                OSReport("WARNING: link num count is reset.\n");
                p->linkedNum = 0;
            }
            break;
        }
        case BTA_DM_SIG_STRENGTH_EVT: {
            DEBUGPrint("BTA_DM_SIG_STRENGTH_EVT\n");
            break;
        }
        case BTA_DM_BUSY_LEVEL_EVT: {
            DEBUGPrint("BTA_DM_BUSY_LEVEL_EVT\n");
            break;
        }
    }
}

void __wudSearchEventStackCallback(tBTA_DM_SEARCH_EVT event, tBTA_DM_SEARCH* pData) {
    s32 timeout;

    switch (event) {
        case BTA_DM_INQ_RES_EVT: {
            tBTA_DM_INQ_RES* pResp = &pData->inq_res;

            DEBUGPrint("INQUIRY RESULT: %02x:%02x:%02x:%02x:%02x:%02x   %02x%02x%02x   %d\n", pResp->bd_addr[0], pResp->bd_addr[1], pResp->bd_addr[2],
                       pResp->bd_addr[3], pResp->bd_addr[4], pResp->bd_addr[5], pResp->dev_class[0], pResp->dev_class[1], pResp->dev_class[2],
                       pResp->rssi);

            _wudDiscRssi = pResp->rssi;

            if (__rvl_wudcb.syncSkipChecks == TRUE || (__rvl_wudcb.syncSkipChecks == FALSE && __wudGetLinkNumber() < WUD_MAX_CHANNELS - 1)) {
                timeout = 6400;
            } else {
                timeout = 32768;
            }

            BTM_WritePageTimeout(timeout);
            break;
        }
        case BTA_DM_INQ_CMPL_EVT: {
            DEBUGPrint("INQUIRY_COMPLETED\n");
            break;
        }
        case BTA_DM_DISC_RES_EVT: {
            tBTA_DM_DISC_RES* dmDiscRes = &pData->disc_res;

            WUD_BDCPY(_wudDiscResp.devAddr, dmDiscRes->bd_addr);
            memcpy(_wudDiscResp.devName, dmDiscRes->bd_name, sizeof(_wudDiscResp.devName));

            _wudDiscResp.services = dmDiscRes->services;
            _wudDiscNumResps++;

            DEBUGPrint("DISCOVER RESULT:  %02x:%02x:%02x:%02x:%02x:%02x   %s (%04x)\n", _wudDiscResp.devAddr[0], _wudDiscResp.devAddr[1],
                       _wudDiscResp.devAddr[2], _wudDiscResp.devAddr[3], _wudDiscResp.devAddr[4], _wudDiscResp.devAddr[5], _wudDiscResp.devName, _wudDiscResp.services);
            break;
        }
        case BTA_DM_DISC_CMPL_EVT: {
            DEBUGPrint("DISCOVER COMPLETED\n");

            __rvl_wudcb.syncState = WUD_STATE_SYNC_CHECK_SEARCH_RESULT;
            break;
        }
        case BTA_DM_SEARCH_CANCEL_CMPL_EVT: {
            DEBUGPrint("SEARCH CANCEL\n\n");

            __wudResetAuthFailCount();

            _wudDiscNumResps = 0;
            memset(&_wudDiscResp, 0, sizeof(WUDDiscResp));

            __rvl_wudcb.syncState = WUD_STATE_SYNC_CHECK_SEARCH_RESULT;
            break;
        }
        default: {
            DEBUGPrint("Warning: Search Callback returns invalid event\n");
            break;
        }
    }
}

void __wudVendorSpecificEventStackCallback(UINT8 len, UINT8* pData) {

    u8 event = pData[0];
    WUDCB* p = &__rvl_wudcb;

    switch (event) {
        case WUD_VSE_INITIATE_PAIRING: {
            __wudProcSyncEvent();
            break;
        }
        case WUD_VSE_DELETE_ALL_KEYS: {
            __wudProcDeleteEvent();
            break;
        }
        case WUD_VSE_SI_PORT_STATUS: {
            p->serialPortStatus = pData[1];
            break;
        }
        case WUD_VSE_WATCH_DOG_RESET_HW: {
            OSErrorLine(5339, "MODULE FATAL ERROR\n");
            break;
        }
    }
}

void __wudDeviceStatusEventStackCallback(tBTM_DEV_STATUS status) {
    DEBUGPrint("__wudDeviceStatusEventStackCallback\n");

    if (status == BTM_DEV_STATUS_CMD_TOUT) {
        OSReport("---- WARNING: USB FATAL ERROR! ----\n");
    }
}

void __wudLinkKeyEventStackCallback(void* p1) {
    tBTM_BD_ADDR_LINK_KEY_PAIR* pPair;
    WUDCB* p = &__rvl_wudcb;
    WUDDevInfo* pInfo;
    int i;

    tBTM_STORED_LINK_KEYS_EVT* pData = p1;
    tBTM_READ_STORED_LINK_KEY_COMPLETE* pRead;
    tBTM_WRITE_STORED_LINK_KEY_COMPLETE* pWrite;
    tBTM_DELETE_STORED_LINK_KEY_COMPLETE* pDelete;
    tBTM_RETURN_LINK_KEYS_EVT* pReturn;

    switch (pData->event) {
        case BTM_CB_EVT_RETURN_LINK_KEYS: {
            pReturn = p1;

            DEBUGPrint("BTM_CB_EVT_RETURN_LINK_KEYS\n");

            pPair = (tBTM_BD_ADDR_LINK_KEY_PAIR*)(pReturn + 1);
            i = 0;

            while (i < pReturn->num_keys) {
                pInfo = WUDiGetDevInfo(pPair->bd_addr);

                DEBUGPrint("BD_ADDR:  %02x:%02x:%02x:%02x:%02x:%02x\n", pPair->bd_addr[0], pPair->bd_addr[1], pPair->bd_addr[2], pPair->bd_addr[3],
                           pPair->bd_addr[4], pPair->bd_addr[5]);

                DEBUGPrint("LINKKEY: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n", pPair->link_key[0],
                           pPair->link_key[1], pPair->link_key[2], pPair->link_key[3], pPair->link_key[4], pPair->link_key[5], pPair->link_key[6],
                           pPair->link_key[7], pPair->link_key[8], pPair->link_key[9], pPair->link_key[10], pPair->link_key[11], pPair->link_key[12],
                           pPair->link_key[13], pPair->link_key[14], pPair->link_key[15]);

                if (pInfo == NULL) {
                    DEBUGPrint("this device is not stored in NAND\n");

                    pInfo = __wudGetNewDevInfo();

                    if (pInfo == NULL) {
                        for (i = 0; i < WUD_MAX_DEV_ENTRY_FOR_SMP; i++) {
                            if (p->smpDevs[i].status == 0) {
                                pInfo = &p->smpDevs[i];
                                break;
                            }
                        }
                    }

                    if (pInfo) {
                        pInfo->UNK_0x5C = 1;
                        pInfo->status = 1;

                        WUD_BDCPY(pInfo->devAddr, pPair->bd_addr);
                        memcpy(pInfo->linkKey, pPair->link_key, LINK_KEY_LEN);
                    }
                } else {
                    WUD_BDCPY(pInfo->devAddr, pPair->bd_addr);
                    memcpy(pInfo->linkKey, pPair->link_key, LINK_KEY_LEN);

                    if (pInfo->UNK_0x5C == 2) {
                        pInfo->UNK_0x5C = 3;
                    }

                    WUD_BDCPY(p->pairAddr, pPair->bd_addr);
                    DEBUGPrint("   LAST: %02x:%02x:%02x:%02x:%02x:%02x\n", p->pairAddr[0], p->pairAddr[1], p->pairAddr[2], p->pairAddr[3],
                               p->pairAddr[4], p->pairAddr[5]);
                }

                pPair++;
                i++;
            }

            break;
        }
        case BTM_CB_EVT_READ_STORED_LINK_KEYS: {
            pRead = p1;

            DEBUGPrint("BTM_CB_EVT_READ_STORED_LINK_KEYS\n");
            DEBUGPrint("  status: %d  max_keys: %d  num_keys: %d\n", pRead->status, pRead->max_keys, pRead->read_keys);

            p->linkKeyState = WUD_STATE_LINK_KEY_START;
            break;
        }
        case BTM_CB_EVT_WRITE_STORED_LINK_KEYS: {
            pWrite = p1;

            DEBUGPrint("BTM_CB_EVT_WRITE_STORED_LINK_KEYS\n");
            DEBUGPrint("  status: %d  num_keys: %d\n", pWrite->status, pWrite->num_keys);

            p->linkKeyState = WUD_STATE_LINK_KEY_START;
            break;
        }
        case BTM_CB_EVT_DELETE_STORED_LINK_KEYS: {
            pDelete = p1;

            DEBUGPrint("BTM_CB_EVT_DELETE_STORED_LINK_KEYS\n");
            DEBUGPrint("  status: %d  num_keys: %d\n", pDelete->status, pDelete->num_keys);

            if (pDelete->num_keys == 0) {
                DEBUGPrint("WARNING: no entry is deleted\n");
            }

            p->linkKeyState = WUD_STATE_LINK_KEY_START;
            break;
        }

        default: {
            OSErrorLine(5495, "Unknown event\n");
        }
    }
}

void __wudPowerMangeEventStackCallback(BD_ADDR addr, tBTM_PM_STATUS status, UINT16 value, UINT8 hciStatus) {

    WUDDevInfo* pInfo;

    DEBUGPrint("__wudPowerMangeEventStackCallback\n");
    DEBUGPrint("hci_status = %d\n", hciStatus);

    pInfo = WUDiGetDevInfo(addr);
    if (pInfo == NULL) {
        if (WUD_BDCMP(_wudDiscWork.devAddr, addr) == 0) {
            pInfo = &_wudDiscWork;
        } else {
            WUD_DEBUGPrint("Unknown device is connected and changes the connection type!!!!\n");
            WUD_DEBUGPrint(" addr = %02x:%02x:%02x:%02x:%02x:%02x,  status = %d\n", addr[0], addr[1], addr[2], addr[3], addr[4], addr[5], status);
            return;
        }
    }

    switch (status) {
        case BTM_PM_STS_ACTIVE: {
            pInfo->status = 8;
            break;
        }
        case BTM_PM_STS_SNIFF: {
            pInfo->status = 9;
            break;
        }
    }

    DEBUGPrint(" addr = %02x:%02x:%02x:%02x:%02x:%02x,  status = %d\n", pInfo->devAddr[0], pInfo->devAddr[1], pInfo->devAddr[2], pInfo->devAddr[3],
               pInfo->devAddr[4], pInfo->devAddr[5], pInfo->status);
}

// not in the dol; body as in Petari
BOOL _WUDEnableTestMode(void) {
    tBTM_STATUS status;
    BOOL enabled;

    DEBUGPrint("_WUDEnableTestMode\n");

    status = BTM_EnableTestMode();
    enabled = (status == BTM_SUCCESS) ? TRUE : FALSE;

    return enabled;
}

// not in the dol; body as in Petari
void _WUDStartSyncDevice(BD_ADDR addr, u8* devName) {
    WUDCB* p = &__rvl_wudcb;
    BOOL enabled;
    u32 syncState;
    u32 deleteState;
    s32 libStatus;

    DEBUGPrint("_WUDStartSyncDevice()\n");

    enabled = OSDisableInterrupts();
    libStatus = p->libStatus;
    syncState = p->syncState;
    deleteState = p->deleteState;
    OSRestoreInterrupts(enabled);

    if (libStatus == WUD_LIB_STATUS_3) {
        if (WUDIsBusy() == FALSE) {
            enabled = OSDisableInterrupts();
            p->syncState = WUD_STATE_SYNC_IS_EXISTED_DEVICE;
            p->syncedNum = 0;
            memcpy(_wudDiscResp.devName, devName, sizeof(_wudDiscResp.devName));
            WUD_BDCPY(_wudDiscResp.devAddr, addr);
            OSCreateAlarm(&p->alarm);
            OSSetPeriodicAlarm(&p->alarm, OSGetTime(), OSMillisecondsToTicks(20), __wudSyncHandler0);
            OSRestoreInterrupts(enabled);
        }
    }
}

// not in the dol; body as in Petari
void _WUDDeleteStoredDevice(void) {
    WUDCB* p = &__rvl_wudcb;

    DEBUGPrint("_WUDDeleteStoreDevice()\n");

    __wudProcDeleteEvent();

    do {
        ;
    } while (p->deleteState != WUD_STATE_DELETE_START);

    DEBUGPrint("dev number = %d\n", __wudGetStdDevNumber());
}

// Retail's .data has four more strings here: "Not Found", "Found the registered WBC\n", "Removed\n" and
// "Not Found WBC\n". They belong to a function about the balance board that the linker removed and that
// no reference has, so it is missing and this file cannot be linked yet.

static void __wudResetAuthFailCount(void) {
    BTM_VendorSpecificCommand(BT_VSC_NINTENDO_WRITE_PATCH, (u8)sizeof(_wudResetAuthCountCmd), _wudResetAuthCountCmd, NULL);
}

static WUDDevInfo* __wudGetWbcDevice(void) {
    WUDCB* p = &__rvl_wudcb;
    WUDDevInfo* pInfo = NULL;
    WUDDevInfoList* pIt;
    BOOL enabled = OSDisableInterrupts();

    for (pIt = p->stdListHead; pIt; pIt = pIt->next) {
        if (WUD_DEV_NAME_IS_WBC(pIt->devInfo->conf.devName)) {
            pInfo = pIt->devInfo;
        }
    }

    OSRestoreInterrupts(enabled);
    return pInfo;
}

BD_ADDR_PTR _WUDGetDevAddr(UINT8 handle) {
    BD_ADDR_PTR pAddr;
    BOOL enabled = OSDisableInterrupts();

    if (handle < WUD_MAX_DEV_ENTRY) {
        pAddr = WUDiGetDevAddrForHandle(handle);
    } else {
        pAddr = NULL;
    }

    OSRestoreInterrupts(enabled);
    return pAddr;
}

u16 _WUDGetQueuedSize(s8 handle) {
    u16 queuedSize;
    BOOL enabled = OSDisableInterrupts();

    if (0 <= handle && handle < WUD_MAX_DEV_ENTRY) {
        queuedSize = WUDiGetQueueSizeForHandle(handle);
    } else {
        queuedSize = 0;
    }

    OSRestoreInterrupts(enabled);
    return queuedSize;
}

u16 _WUDGetNotAckedSize(s8 handle) {
    u16 notAckedSize;
    BOOL enabled = OSDisableInterrupts();

    if (0 <= handle && handle < WUD_MAX_DEV_ENTRY) {
        notAckedSize = WUDiGetNotAckNumForHandle(handle);
    } else {
        notAckedSize = 0;
    }

    OSRestoreInterrupts(enabled);
    return notAckedSize;
}

u8 _WUDGetLinkNumber(void) {
    WUDCB* p = &__rvl_wudcb;
    BOOL enabled = OSDisableInterrupts();

    u8 num = p->linkedNum;

    OSRestoreInterrupts(enabled);
    return num;
}

WUDDevInfo* WUDiGetDiscoverDevice(void) {
    return &_wudDiscWork;
}

// Remembers which remote was last used on a channel, so that it gets the same channel again.
void WUDSetDeviceHistory(WUDChannel chan, BD_ADDR addr) {
    if (addr == NULL) {
        memset(&_scArray.devices[WUD_MAX_DEV_ENTRY_FOR_STD + chan], 0, sizeof(SCBtDeviceInfo));
    } else {
        WUD_BDCPY(_scArray.devices[WUD_MAX_DEV_ENTRY_FOR_STD + chan].addr, addr);
    }

    _scFlush = TRUE;
}

BOOL WUDIsLatestDevice(WUDChannel chan, BD_ADDR addr) {
    if (addr == NULL) {
        return FALSE;
    }

    if (WUD_BDCMP(_scArray.devices[WUD_MAX_DEV_ENTRY_FOR_STD + chan].addr, addr) == 0) {
        return TRUE;
    } else {
        return FALSE;
    }
}

void WUDUpdateSCSetting(void) {
    if (!_scFlush) {
        return;
    }

    if (SCSetBtDeviceInfoArray(&_scArray)) {
        SCFlushAsync(NULL);
        _scFlush = FALSE;
    }
}

void WUDiSetDevAddrForHandle(UINT8 handle, BD_ADDR addr) {
    _dev_handle_to_bda[handle] = addr;
}

BD_ADDR_PTR WUDiGetDevAddrForHandle(UINT8 handle) {
    return _dev_handle_to_bda[handle];
}

void WUDiSetQueueSizeForHandle(UINT8 handle, u16 size) {
    _dev_handle_queue_size[handle] = size;
}

// not in the dol; _WUDGetQueuedSize has it inlined
u16 WUDiGetQueueSizeForHandle(UINT8 handle) {
    return _dev_handle_queue_size[handle];
}

void WUDiSetNotAckNumForHandle(UINT8 handle, u16 num) {
    _dev_handle_notack_num[handle] = num;
}

// not in the dol; _WUDGetNotAckedSize has it inlined
u16 WUDiGetNotAckNumForHandle(UINT8 handle) {
    return _dev_handle_notack_num[handle];
}
