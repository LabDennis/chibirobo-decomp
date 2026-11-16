/* Public WPAD interface, after projectPiki/pik1wii (CC0), extended to the September 2008 SDK. */
#ifndef REVOLUTION_WPAD_H
#define REVOLUTION_WPAD_H

#include "types.h"
#include "revolution/os.h"

BEGIN_SCOPE_EXTERN_C

typedef u32 WPADDeviceType;
typedef s32 WPADChannel;

typedef void (*WPADExtensionCallback)(WPADChannel chan, s32 result);
typedef void (*WPADConnectCallback)(WPADChannel chan, s32 reason);
typedef void (*WPADCallback)(WPADChannel chan, s32 result);
typedef void* (*WPADAlloc)(u32 size);
typedef BOOL (*WPADFree)(void* ptr);
typedef void (*WPADSyncDeviceCallback)(s32 result, s32 num);
typedef void (*WPADSamplingCallback)(s32 chan);
typedef void (*WPADFlushCallback)(u32 result);
typedef void (*WPADSimpleSyncCallback)(s32 result, s32 num);
typedef void (*WPADClearDeviceCallback)(s32 result);

#define WPAD_CHAN0 0
#define WPAD_CHAN1 1
#define WPAD_CHAN2 2
#define WPAD_CHAN3 3

#define WPAD_MAX_CONTROLLERS 4

#define WPAD_MOTOR_STOP   0
#define WPAD_MOTOR_RUMBLE 1

#define WPAD_BUTTON_LEFT  0x0001
#define WPAD_BUTTON_RIGHT 0x0002
#define WPAD_BUTTON_DOWN  0x0004
#define WPAD_BUTTON_UP    0x0008
#define WPAD_BUTTON_PLUS  0x0010
#define WPAD_BUTTON_2     0x0100
#define WPAD_BUTTON_1     0x0200
#define WPAD_BUTTON_B     0x0400
#define WPAD_BUTTON_A     0x0800
#define WPAD_BUTTON_MINUS 0x1000
#define WPAD_BUTTON_Z     0x2000
#define WPAD_BUTTON_C     0x4000
#define WPAD_BUTTON_HOME  0x8000

#define WPAD_CL_BUTTON_UP    0x0001
#define WPAD_CL_BUTTON_LEFT  0x0002
#define WPAD_CL_TRIGGER_ZR   0x0004
#define WPAD_CL_BUTTON_X     0x0008
#define WPAD_CL_BUTTON_A     0x0010
#define WPAD_CL_BUTTON_Y     0x0020
#define WPAD_CL_BUTTON_B     0x0040
#define WPAD_CL_TRIGGER_ZL   0x0080
#define WPAD_CL_RESERVED     0x0100
#define WPAD_CL_TRIGGER_R    0x0200
#define WPAD_CL_BUTTON_PLUS  0x0400
#define WPAD_CL_BUTTON_HOME  0x0800
#define WPAD_CL_BUTTON_MINUS 0x1000
#define WPAD_CL_TRIGGER_L    0x2000
#define WPAD_CL_BUTTON_DOWN  0x4000
#define WPAD_CL_BUTTON_RIGHT 0x8000

#define WPAD_DPD_MAX_OBJECTS 4
#define WPAD_DPD_IMG_RESO_WX 1024
#define WPAD_DPD_IMG_RESO_WY 768

#define WPAD_DEV_CORE            0
#define WPAD_DEV_FREESTYLE       1
#define WPAD_DEV_CLASSIC         2
#define WPAD_DEV_BALANCE_CHECKER 3
#define WPAD_DEV_VSM             4
#define WPAD_DEV_MPLS            5
#define WPAD_DEV_MPLS_FREESTYLE  6
#define WPAD_DEV_MPLS_CLASSIC    7
#define WPAD_DEV_TRAIN           16
#define WPAD_DEV_GUITAR          17
#define WPAD_DEV_DRUM            18
#define WPAD_DEV_MPLS_FUTURE     250
#define WPAD_DEV_FUTURE          251
#define WPAD_DEV_NOT_SUPPORTED   252
#define WPAD_DEV_NOT_FOUND       253
#define WPAD_DEV_NULL            254
#define WPAD_DEV_UNKNOWN         255

#define WPAD_FMT_CORE              0
#define WPAD_FMT_CORE_ACC          1
#define WPAD_FMT_CORE_ACC_DPD      2
#define WPAD_FMT_FREESTYLE         3
#define WPAD_FMT_FREESTYLE_ACC     4
#define WPAD_FMT_FREESTYLE_ACC_DPD 5
#define WPAD_FMT_CLASSIC           6
#define WPAD_FMT_CLASSIC_ACC       7
#define WPAD_FMT_CLASSIC_ACC_DPD   8
#define WPAD_FMT_CORE_ACC_DPD_FULL 9
#define WPAD_FMT_TRAIN             10
#define WPAD_FMT_GUITAR            11
#define WPAD_FMT_BALANCE_CHECKER   12
#define WPAD_FMT_VSM               13
#define WPAD_FMT_VSM_EX            14
#define WPAD_FMT_DRUM              15

#define WPAD_STATE_DISABLED  0
#define WPAD_STATE_ENABLING  1
#define WPAD_STATE_ENABLED   2
#define WPAD_STATE_SETUP     3
#define WPAD_STATE_DISABLING 4

#define WPAD_BATTERY_LEVEL_CRITICAL 0
#define WPAD_BATTERY_LEVEL_LOW      1
#define WPAD_BATTERY_LEVEL_MEDIUM   2
#define WPAD_BATTERY_LEVEL_HIGH     3
#define WPAD_BATTERY_LEVEL_MAX      4

#define WPAD_DPD_OFF   0
#define WPAD_DPD_BASIC 1
#define WPAD_DPD_STD   3
#define WPAD_DPD_FULL  5

#define WPAD_BLC_DISABLE 0x00
#define WPAD_BLC_CHECK   0x55
#define WPAD_BLC_ENABLE  0xAA

#define WPAD_SENSOR_BAR_POS_BOTTOM 0
#define WPAD_SENSOR_BAR_POS_TOP    1

#define WPAD_ERR_NONE          0
#define WPAD_ERR_NO_CONTROLLER -1
#define WPAD_ERR_BUSY          -2
#define WPAD_ERR_TRANSFER      -3
#define WPAD_ERR_INVALID       -4
#define WPAD_ERR_NOPERM        -5
#define WPAD_ERR_BROKEN        -6
#define WPAD_ERR_CORRUPTED     -7

#define WPAD_MAX_SPEAKER_VOLUME 127

#define WPAD_BTN_HCURSOR_MASK (WPAD_BUTTON_LEFT | WPAD_BUTTON_RIGHT)
#define WPAD_BTN_VCURSOR_MASK (WPAD_BUTTON_UP | WPAD_BUTTON_DOWN)
#define WPAD_CLB_HCURSOR_MASK (WPAD_CL_BUTTON_LEFT | WPAD_CL_BUTTON_RIGHT)
#define WPAD_CLB_VCURSOR_MASK (WPAD_CL_BUTTON_UP | WPAD_CL_BUTTON_DOWN)

typedef u32 WPADSpeakerCommand;
enum WPADSpeakerCommand_et {
	WPAD_SPEAKER_DISABLE = 0,
	WPAD_SPEAKER_ENABLE  = 1, // might be ON? see HBMRemoteSpk.cpp
	WPAD_SPEAKER_MUTE    = 2,
	WPAD_SPEAKER_UNMUTE  = 3,
	WPAD_SPEAKER_PLAY    = 4, // figured out from HBM usage

	// does the same thing as ENABLE? unless i'm missing something
	// not used so i don't know the context
	WPAD_SPEAKER_CMD_05 = 5,
};

typedef struct DPDObject {
	s16 x;
	s16 y;
	u16 size;
	u8 traceId;
} DPDObject;

typedef struct DPDObjEx {
	s16 range_x1;
	s16 range_y1;
	s16 range_x2;
	s16 range_y2;
	u16 pixel;
	s8 radius;
} DPDObjEx;

typedef struct WPADStatusEx {
	u16 button;
	s16 accX;
	s16 accY;
	s16 accZ;
	DPDObject obj[WPAD_DPD_MAX_OBJECTS];
	u8 dev;
	s8 err;
	DPDObjEx exp[WPAD_DPD_MAX_OBJECTS];

} WPADStatusEx;

typedef struct WPADInfo {
	BOOL dpd;
	BOOL speaker;
	BOOL attach;
	BOOL lowBat;
	BOOL nearempty;
	u8 battery;
	u8 led;
	u8 protocol;
	u8 firmware;
} WPADInfo;

typedef enum {
	WPAD_RESULT_ERR_3 = -3,
	WPAD_RESULT_ERR_2,
	WPAD_RESULT_ERR_1,

	WPAD_RESULT_SUCCESS = 0,
} WPADResult;

typedef struct WPADStatus {
	u16 button;
	s16 accX;
	s16 accY;
	s16 accZ;
	DPDObject obj[4];
	u8 dev;
	s8 err;
} WPADStatus;

typedef struct WPADFSStatus {
	u16 button;
	s16 accX;
	s16 accY;
	s16 accZ;
	DPDObject obj[4];
	u8 dev;
	s8 err;
	s16 fsAccX;
	s16 fsAccY;
	s16 fsAccZ;
	s8 fsStickX;
	s8 fsStickY;
} WPADFSStatus;

typedef struct WPADCLStatus {
	u16 button;
	s16 accX;
	s16 accY;
	s16 accZ;
	DPDObject obj[4];
	u8 dev;
	s8 err;
	u16 clButton;
	s16 clLStickX;
	s16 clLStickY;
	s16 clRStickX;
	s16 clRStickY;
	u8 clTriggerL;
	u8 clTriggerR;
} WPADCLStatus;

typedef struct WPADTRStatus {
	u16 button;
	s16 accX;
	s16 accY;
	s16 accZ;
	DPDObject obj[WPAD_DPD_MAX_OBJECTS];
	u8 dev;
	s8 err;
	u16 trButton;
	u8 brake;
	u8 mascon;
} WPADTRStatus;

typedef struct WPADBLStatus {
	u16 button;
	s16 accX;
	s16 accY;
	s16 accZ;
	DPDObject obj[WPAD_DPD_MAX_OBJECTS];
	u8 dev;
	s8 err;
	u16 press[4];
	s8 temp;
	u8 battery;
} WPADBLStatus;

typedef struct WPADVSStatus {
	u16 button;
	s16 accX;
	s16 accY;
	s16 accZ;
	DPDObject obj[WPAD_DPD_MAX_OBJECTS];
	u8 dev;
	s8 err;
	u16 at_0x2a[5];
	u8 at_0x34;
	u16 at_0x36[5];
	u8 at_0x40;
	u16 at_0x42;
	u16 at_0x44;
	u8 at_0x46;
	u8 at_0x47;
	u8 at_0x48;
	u8 at_0x49;
} WPADVSStatus;

typedef struct WPADAcc {
	s16 x;
	s16 y;
	s16 z;
} WPADAcc;

typedef struct WPADMEMGameInfo {
	OSTime calendarTimeTick;
	u16 gameTitle[17];
	char gameId[4];
	u8 gameType;
	u8 checkSum;
	u8 reserved[8];
} WPADMEMGameInfo;

void WPADRegisterAllocator(WPADAlloc, WPADFree);
u32 WPADGetWorkMemorySize(void);

void WPADDisconnect(s32);
s32 WPADGetInfoAsync(s32, WPADInfo*, WPADCallback);

BOOL WPADIsSpeakerEnabled(s32);
s32 WPADControlSpeaker(s32, u32, WPADCallback);
u8 WPADGetSpeakerVolume(void);
s32 WPADSendStreamData(s32, void*, u16);
BOOL WPADCanSendStreamData(s32);
void WPADGetAccGravityUnit(s32 chan, u32 type, WPADAcc* acc);
void WPADControlMotor(s32, u32);
BOOL WPADStartSyncDevice(void);
BOOL WPADStartFastSyncDevice(void);
BOOL WPADStartSimpleSync(void);
BOOL WPADStartFastSimpleSync(void);
BOOL WPADStopSimpleSync(void);

BOOL WPADIsDpdEnabled(s32 chan);
s32 WPADControlDpd(s32 chan, u32 command, WPADCallback callback);
u32 WPADGetDataFormat(s32 chan);
s32 WPADSetDataFormat(s32 chan, u32 fmt);
void WPADRead(s32 chan, void* status);
void WPADSetAutoSleepTime(u8);
u8 WPADGetSensorBarPosition(void);
s32 WPADProbe(s32 chan, u32* type);
s32 WPADGetStatus(void);

void WPADInit(void);
u8 WPADGetDpdFormat(s32 chan);
s32 WPADControlBLC(s32 chan, u8 command, WPADCallback callback);
void WPADSetCallbackByKPAD(BOOL flag);
BOOL WPADIsEnabledVSM(void);
BOOL WPADIsEnabledTRN(void);
BOOL WPADIsEnabledGTR(void);
BOOL WPADIsEnabledDRM(void);
void WPADRecalibrate(s32 chan);
BOOL WPADCancelSyncDevice(void);
u8 WPADGetDpdSensitivity(void);

WPADExtensionCallback WPADSetExtensionCallback(s32, WPADExtensionCallback);
WPADConnectCallback WPADSetConnectCallback(s32, WPADConnectCallback callback);
WPADSamplingCallback WPADSetSamplingCallback(s32 chan, WPADSamplingCallback callback);

WPADSyncDeviceCallback WPADSetSyncDeviceCallback(WPADSyncDeviceCallback cb);
WPADSimpleSyncCallback WPADSetSimpleSyncCallback(WPADSimpleSyncCallback cb);
WPADClearDeviceCallback WPADSetClearDeviceCallback(WPADClearDeviceCallback cb);

u8 WPADGetRadioSensitivity(s32);

void WPADSetSpeakerVolume(u8 volume);
void WPADShutdown(void);

void WPADEnableMotor(BOOL enabled);
BOOL WPADIsMotorEnabled(void);

s32 WPADControlLed(s32, u8, WPADCallback);
BOOL WPADSaveConfig(WPADFlushCallback callback);
void WPADGetAddress(s32, u8*);

void WPADSetAutoSamplingBuf(s32, void*, u32);

#define WPADStartMotor(chan) WPADControlMotor((chan), WPAD_MOTOR_RUMBLE)
#define WPADStopMotor(chan)  WPADControlMotor((chan), WPAD_MOTOR_STOP)

// balance board (the WBC entry points live in WPAD.c in this build)
s32 WBCSetupCalibration(void);
s32 WBCGetCalibrationStatus(void);
s32 WBCGetBatteryLevel(u8 battery);
s32 WBCReadDummy(WPADBLStatus* status, f64* weight, u32 size);
s32 WBCSetZEROPointDummy(f64* press, u32 size);
s32 WBCGetTGCWeightDummy(f64 weight, f64* tgcWeight, WPADBLStatus* status);

////////////////////////////////////

END_SCOPE_EXTERN_C

#endif
