/* The part of SC (system configuration) that WUD uses, after wii-ipl's private/sc.h (CC0).
 * Stand-in until revolution/sc.h exists. */
#ifndef REVOLUTION_WUD_SC_H
#define REVOLUTION_WUD_SC_H

#include "types.h"

#include "revolution/BTE/bt_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int SCStatus;

#define SC_STATUS_OK    0
#define SC_STATUS_BUSY  1
#define SC_STATUS_FATAL 2
#define SC_STATUS_PARSE 3

typedef void (*SCFlushCallback)(SCStatus status);

void SCInit(void);
void SCFlushAsync(SCFlushCallback callback);
u32 SCCheckStatus(void);
s8 SCGetProductArea(void);

#define SC_PRODUCT_AREA_JPN 0

#define SC_MAX_DEV_ENTRY_FOR_STD 10
#define SC_MAX_DEV_ENTRY_FOR_SMP 6
#define SC_MAX_DEV_ENTRY         (SC_MAX_DEV_ENTRY_FOR_STD + SC_MAX_DEV_ENTRY_FOR_SMP)

typedef struct SC_BT_DEV_INFO {
	u8 devName[20]; // 0x00
	u8 unk_0x14;
	u8 unk_0x15[0xB];
	LINK_KEY linkKey; // 0x20
	u8 unk_0x30[0x10];
} SC_BT_DEV_INFO;

typedef struct SCBtDeviceInfo {
	BD_ADDR addr;        // 0x00
	SC_BT_DEV_INFO info; // 0x06
} SCBtDeviceInfo;

typedef struct SCBtDeviceInfoArray {
	u8 numRegist; // 0x00
	union {
		struct {
			SCBtDeviceInfo regist[SC_MAX_DEV_ENTRY_FOR_STD]; // 0x1
			SCBtDeviceInfo active[4];                        // 0x2BD, the remote last used on each channel
			SCBtDeviceInfo wbc;                              // 0x3D5, the balance board
			SCBtDeviceInfo unk_0x41B;
		};

		SCBtDeviceInfo devices[SC_MAX_DEV_ENTRY];
	};
} SCBtDeviceInfoArray;

BOOL SCGetBtDeviceInfoArray(SCBtDeviceInfoArray* devInfo);
BOOL SCSetBtDeviceInfoArray(SCBtDeviceInfoArray* devInfo);

typedef struct SCBtCmpDevInfo {
	BD_ADDR addr;     // 0x00
	u8 name[64];      // 0x06
	LINK_KEY linkKey; // 0x30
} SCBtCmpDevInfo;

typedef struct SCBtCmpDevInfoArray {
	u8 numRegist;                                     // 0x00
	SCBtCmpDevInfo devices[SC_MAX_DEV_ENTRY_FOR_SMP]; // 0x01
} SCBtCmpDevInfoArray;

BOOL SCGetBtCmpDevInfoArray(SCBtCmpDevInfoArray* devInfo);
BOOL SCSetBtCmpDevInfoArray(SCBtCmpDevInfoArray* devInfo);

#ifdef __cplusplus
}
#endif

#endif // REVOLUTION_WUD_SC_H
