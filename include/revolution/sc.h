#ifndef REVOLUTION_SC_H
#define REVOLUTION_SC_H
#include "types.h"

#include "revolution/nand.h"

#include "Dolphin/os.h"

BEGIN_SCOPE_EXTERN_C

/* SC System */

typedef int SCStatus;

#define SC_STATUS_OK    0
#define SC_STATUS_BUSY  1
#define SC_STATUS_FATAL 2
#define SC_STATUS_PARSE 3

typedef void (*SCFlushCallback)(int status);

void SCInit();

u32 SCFlush();
void SCFlushAsync(SCFlushCallback callback);

u32 SCCheckStatus();

/* Product Configuration (setting.txt) */

#define SC_PRODUCT_AREA_UNKNOWN -1
#define SC_PRODUCT_AREA_JPN     0  /* Japan */
#define SC_PRODUCT_AREA_USA     1  /* United states of America */
#define SC_PRODUCT_AREA_EUR     2  /* Europe */
#define SC_PRODUCT_AREA_AUS     3  /* Austrailia */
#define SC_PRODUCT_AREA_BRA     4  /* Brazil */
#define SC_PRODUCT_AREA_TWN     5  /* Taiwan */
#define SC_PRODUCT_AREA_KOR     6  /* Korea */
#define SC_PRODUCT_AREA_HKG     7  /* Hong kong */
#define SC_PRODUCT_AREA_ASI     8  /* Asia */
#define SC_PRODUCT_AREA_LTN     9  /* Latin */
#define SC_PRODUCT_AREA_SAF     10 /* South Africa */
#define SC_PRODUCT_AREA_CHN     11 /* China */
#define SC_PRODUCT_AREA_MAX     12

BOOL SCGetProductAreaString(char* buf, u32 sz);
s8 SCGetProductArea();

#define SC_PRODUCT_GAME_REGION_UNKNOWN -1
#define SC_PRODUCT_GAME_REGION_JP      0
#define SC_PRODUCT_GAME_REGION_US      1
#define SC_PRODUCT_GAME_REGION_EU      2
// #define SC_PRODUCT_GAME_REGION_ALL      3   /* Not a thing */
#define SC_PRODUCT_GAME_REGION_KR 4
#define SC_PRODUCT_GAME_REGION_CN 5

BOOL SCGetProductGameRegionString(char* buf, u32 sz);
s8 SCGetProductGameRegion();

/* System Configuration (SYSCONF) */

#define SC_ASPECT_RATIO_4x3  0
#define SC_ASPECT_RATIO_16x9 1

u8 SCGetAspectRatio();
BOOL SCSetAspectRatio(u8 aspectRatio);

s8 SCGetDisplayOffsetH();
BOOL SCSetDisplayOffsetH(s8 displayOffsetH);

#define SC_EURGB60_MODE_OFF 0
#define SC_EURGB60_MODE_ON  1

u8 SCGetEuRgb60Mode();
BOOL SCSetEuRgb60Mode(u8 eurgb60Mode);

#define SC_IDLE_MODE_STANDBY_OFF 0
#define SC_IDLE_MODE_STANDBY_ON  1

#define SC_IDLE_MODE_LED_OFF    0
#define SC_IDLE_MODE_LED_DIM    1
#define SC_IDLE_MODE_LED_BRIGHT 2

typedef struct SCIdleModeInfo {
	u8 standby; // 0x00
	u8 led;     // 0x01
} SCIdleModeInfo;

BOOL SCGetIdleMode(SCIdleModeInfo* info);
BOOL SCSetIdleMode(SCIdleModeInfo* info);

#define SC_LANG_JAPANESE     0
#define SC_LANG_ENGLISH      1
#define SC_LANG_GERMAN       2
#define SC_LANG_FRENCH       3
#define SC_LANG_SPANISH      4
#define SC_LANG_ITALIAN      5
#define SC_LANG_DUTCH        6
#define SC_LANG_SIMP_CHINESE 7
#define SC_LANG_TRAD_CHINESE 8
#define SC_LANG_KOREAN       9
#define SC_LANG_MAX          10

u8 SCGetLanguage();
BOOL SCSetLanguage(u8 language);

#define SC_PARENTAL_PASSWORD_LENGTH      4
#define SC_PARENTAL_SECRET_ANSWER_LENGTH 32

typedef struct SCParentalControlsInfo {
	u8 enable; // 0x00
	u8 org;    // 0x01

	u8 rating; // 0x02

	char password[SC_PARENTAL_PASSWORD_LENGTH]; // 0x03

	u8 secretQuestion;                                  // 0x07
	u16 secretAnswer[SC_PARENTAL_SECRET_ANSWER_LENGTH]; // 0x08
	u16 secretAnswerLength;                             // 0x48
} SCParentalControlsInfo;

#define SC_PARENTAL_FLAG_ENABLED (1 << 7)
#define SC_PARENTAL_RATING_BITS  0x1F

#define SC_PARENTAL_ORG_CERO          0
#define SC_PARENTAL_ORG_ESRB          1
#define SC_PARENTAL_ORG_USK           3
#define SC_PARENTAL_ORG_PEGI          4
#define SC_PARENTAL_ORG_PEGI_FINLAND  5
#define SC_PARENTAL_ORG_PEGI_PORTUGAL 6
#define SC_PARENTAL_ORG_BBFC          7
#define SC_PARENTAL_ORG_OFLC          8
#define SC_PARENTAL_ORG_GRB           9
#define SC_PARENTAL_ORG_GRAC          SC_PARENTAL_ORG_GRB /* GRB is now called GRAC */

BOOL SCGetParentalControl(SCParentalControlsInfo* pcInfo);
BOOL SCSetParentalControl(SCParentalControlsInfo* pcInfo);

#define SC_PROGRESSIVE_MODE_OFF 0
#define SC_PROGRESSIVE_MODE_ON  1

u8 SCGetProgressiveMode();
BOOL SCSetProgressiveMode(u8 progressiveMode);

#define SC_SCREEN_SAVER_MODE_OFF 0
#define SC_SCREEN_SAVER_MODE_ON  1

u8 SCGetScreenSaverMode();
BOOL SCSetScreenSaverMode(u8 screenSaverMode);

#define SC_SOUND_MODE_MONO     0
#define SC_SOUND_MODE_STEREO   1
#define SC_SOUND_MODE_SURROUND 2

u8 SCGetSoundMode();
BOOL SCSetSoundMode(u8 soundMode);

u32 SCGetCounterBias();
BOOL SCSetCounterBias(u32 counterBias);

BOOL SCGetWwwRestriction();
BOOL SCSetWwwRestriction(BOOL wwwRestrictFlag);

#define SC_WPAD_MOTOR_MODE_OFF 0
#define SC_WPAD_MOTOR_MODE_ON  1

u8 SCGetWpadMotorMode();
BOOL SCSetWpadMotorMode(u8 motorMode);

#define SC_WPAD_SENSOR_BAR_POSITION_BOTTOM 0
#define SC_WPAD_SENSOR_BAR_POSITION_TOP    1

u8 SCGetWpadSensorBarPosition();
BOOL SCSetWpadSensorBarPosition(u8 sensorBarPosition);

u8 SCGetWpadSpeakerVolume();
BOOL SCSetWpadSpeakerVolume(u8 speakerVolume);

typedef struct SCOwnerNickname {
	u16 name[10]; // 0x00
	u16 length;   // 0x14
} SCOwnerNickname;

BOOL SCGetOwnerNickName(SCOwnerNickname* ownerNickname);
BOOL SCSetOwnerNickName(SCOwnerNickname* ownerNickname);

#define SC_SIMPLE_ADDRESS_ID_COUNTRY 24
#define SC_SIMPLE_ADDRESS_ID_REGION  16
#define SC_SIMPLE_ADDRESS_ID_CITY    0

enum {
	SC_COUNTRY_JP = 1,
	SC_COUNTRY_AI = 8,
	SC_COUNTRY_AG,
	SC_COUNTRY_AR,
	SC_COUNTRY_AW,
	SC_COUNTRY_BS,
	SC_COUNTRY_BB,
	SC_COUNTRY_BZ,
	SC_COUNTRY_BO,
	SC_COUNTRY_BR,
	SC_COUNTRY_VG,
	SC_COUNTRY_CA,
	SC_COUNTRY_KY,
	SC_COUNTRY_CL,
	SC_COUNTRY_CO,
	SC_COUNTRY_CR,
	SC_COUNTRY_DM,
	SC_COUNTRY_DO,
	SC_COUNTRY_EC,
	SC_COUNTRY_SV,
	SC_COUNTRY_GF,
	SC_COUNTRY_GD,
	SC_COUNTRY_GP,
	SC_COUNTRY_GT,
	SC_COUNTRY_GY,
	SC_COUNTRY_HT,
	SC_COUNTRY_HN,
	SC_COUNTRY_JM,
	SC_COUNTRY_MQ,
	SC_COUNTRY_MX,
	SC_COUNTRY_MS,
	SC_COUNTRY_AN,
	SC_COUNTRY_NI,
	SC_COUNTRY_PA,
	SC_COUNTRY_PY,
	SC_COUNTRY_PE,
	SC_COUNTRY_KN,
	SC_COUNTRY_LC,
	SC_COUNTRY_VC,
	SC_COUNTRY_SR,
	SC_COUNTRY_TT,
	SC_COUNTRY_TC,
	SC_COUNTRY_US,
	SC_COUNTRY_UY,
	SC_COUNTRY_VI,
	SC_COUNTRY_VE,
	SC_COUNTRY_AL = 64,
	SC_COUNTRY_AU,
	SC_COUNTRY_AT,
	SC_COUNTRY_BE,
	SC_COUNTRY_BA,
	SC_COUNTRY_BW,
	SC_COUNTRY_BG,
	SC_COUNTRY_HR,
	SC_COUNTRY_CY,
	SC_COUNTRY_CZ,
	SC_COUNTRY_DK,
	SC_COUNTRY_EE,
	SC_COUNTRY_FI,
	SC_COUNTRY_FR,
	SC_COUNTRY_DE,
	SC_COUNTRY_GR,
	SC_COUNTRY_HU,
	SC_COUNTRY_IS,
	SC_COUNTRY_IE,
	SC_COUNTRY_IT,
	SC_COUNTRY_LV,
	SC_COUNTRY_LS,
	SC_COUNTRY_LI,
	SC_COUNTRY_LT,
	SC_COUNTRY_LU,
	SC_COUNTRY_MK,
	SC_COUNTRY_MT,
	SC_COUNTRY_ME,
	SC_COUNTRY_MZ,
	SC_COUNTRY_NA,
	SC_COUNTRY_NL,
	SC_COUNTRY_NZ,
	SC_COUNTRY_NO,
	SC_COUNTRY_PL,
	SC_COUNTRY_PT,
	SC_COUNTRY_RO,
	SC_COUNTRY_RU,
	SC_COUNTRY_RS,
	SC_COUNTRY_SK,
	SC_COUNTRY_SI,
	SC_COUNTRY_ZA,
	SC_COUNTRY_ES,
	SC_COUNTRY_SZ,
	SC_COUNTRY_SE,
	SC_COUNTRY_CH,
	SC_COUNTRY_TR,
	SC_COUNTRY_GB,
	SC_COUNTRY_ZM,
	SC_COUNTRY_ZW,
	SC_COUNTRY_TW = 128,
	SC_COUNTRY_KR = 136,
	SC_COUNTRY_HK = 144,
	SC_COUNTRY_MO,
	SC_COUNTRY_ID = 152,
	SC_COUNTRY_SG,
	SC_COUNTRY_TH,
	SC_COUNTRY_PH,
	SC_COUNTRY_MY,
	SC_COUNTRY_CN = 160,
	SC_COUNTRY_AE = 168,
	SC_COUNTRY_IN,
	SC_COUNTRY_EG,
	SC_COUNTRY_OM,
	SC_COUNTRY_QA,
	SC_COUNTRY_KW,
	SC_COUNTRY_SA,
	SC_COUNTRY_SY,
	SC_COUNTRY_BH,
	SC_COUNTRY_JO,
};

u32 SCGetSimpleAddressID();

typedef struct SCSimpleAddress {
	u32 id;                  // 0x00
	u16 countryName[16][64]; // 0x04
	u16 regionName[16][64];  // 0x804
	u16 latitude;            // 0x1004
	u16 longitude;           // 0x1006
} SCSimpleAddress;

BOOL SCGetSimpleAddressData(SCSimpleAddress* address);
BOOL SCSetSimpleAddressData(SCSimpleAddress* address);

#define SC_NET_RESTRICTIONS_NONE      0
#define SC_NET_RESTRICTIONS_OPERA     (1 << 0)
#define SC_NET_RESTRICTIONS_MSG_BOARD (1 << 1)
#define SC_NET_RESTRICTIONS_SHOPPING  (1 << 2)

u32 SCGetNetContentRestrictions();
BOOL SCSetNetContentRestrictions(u32 netRestrictions);

BOOL SCGetConfigDoneFlag();
BOOL SCSetConfigDoneFlag(BOOL configDoneFlag);

BOOL SCGetConfigDoneFlag2();
BOOL SCSetConfigDoneFlag2(BOOL configDone2Flag);

#define SC_UPDATE_TYPE_NONE    0
#define SC_UPDATE_TYPE_DISC    1
#define SC_UPDATE_TYPE_NETWORK 2

u8 SCGetUpdateType();
BOOL SCSetUpdateType(u8 updateType);

BOOL SCGetEULA();
BOOL SCSetEULA(BOOL eulaFlag);

#define SC_WC_FLAGS_DISABLED 0
#define SC_WC_FLAGS_ENABLED  1

u32 SCGetWCFlags();
BOOL SCSetWCFlags(u32 wcFlags);

BOOL SCGetFreeChannelAppCount(u32* count);
BOOL SCSetFreeChannelAppCount(u32 count);

BOOL SCGetInstalledChannelAppCount(u32* count);
BOOL SCSetInstalledChannelAppCount(u32 count);

// private part of the library

// Bluetooth address and link key as the Bluetooth stack defines them (bt_types.h there)
#ifndef BD_ADDR_LEN
#define BD_ADDR_LEN  6
#define LINK_KEY_LEN 16
typedef u8 BD_ADDR[BD_ADDR_LEN];
typedef u8 LINK_KEY[LINK_KEY_LEN];
#endif

#define SC_CONFIG_FILE_PHYS_ADDRESS 0x3800
#define SC_CONFIG_FILE_LENGTH       0x100

typedef enum SCItemID {
	SC_ITEM_ID_IPL_COUNTER_BIAS,
	SC_ITEM_ID_IPL_ASPECT_RATIO,
	SC_ITEM_ID_IPL_AUTORUN_MODE,
	SC_ITEM_ID_IPL_CONFIG_DONE,
	SC_ITEM_ID_IPL_CONFIG_DONE2,
	SC_ITEM_ID_IPL_DISPLAY_OFFSET_H,
	SC_ITEM_ID_IPL_EURGB60_MODE,
	SC_ITEM_ID_IPL_EULA,
	SC_ITEM_ID_IPL_FREE_CHANNEL_APP_COUNT,
	SC_ITEM_ID_IPL_IDLE_MODE,
	SC_ITEM_ID_IPL_INSTALLED_CHANNEL_APP_COUNT,
	SC_ITEM_ID_IPL_LANGUAGE,
	SC_ITEM_ID_IPL_OWNER_NICKNAME,
	SC_ITEM_ID_IPL_PARENTAL_CONTROL,
	SC_ITEM_ID_IPL_PROGRESSIVE_MODE,
	SC_ITEM_ID_IPL_SCREEN_SAVER_MODE,
	SC_ITEM_ID_IPL_SIMPLE_ADDRESS,
	SC_ITEM_ID_IPL_SOUND_MODE,
	SC_ITEM_ID_IPL_UPDATE_TYPE,
	SC_ITEM_ID_NET_CONFIG,
	SC_ITEM_ID_NET_CONTENT_RESTRICTIONS,
	SC_ITEM_ID_NET_PROFILE,
	SC_ITEM_ID_NET_WC_RESTRICTION,
	SC_ITEM_ID_NET_WC_FLAGS,
	SC_ITEM_ID_DEV_BOOT_MODE,
	SC_ITEM_ID_DEV_VIDEO_MODE,
	SC_ITEM_ID_DEV_COUNTRY_CODE,
	SC_ITEM_ID_DEV_DRIVESAVING_MODE,
	SC_ITEM_ID_BT_DEVICE_INFO,
	SC_ITEM_ID_BT_CMPDEV_INFO,
	SC_ITEM_ID_BT_DPD_SENSIBILITY,
	SC_ITEM_ID_BT_SPEAKER_VOLUME,
	SC_ITEM_ID_BT_MOTOR_MODE,
	SC_ITEM_ID_BT_SENSOR_BAR_POSITION,
	SC_ITEM_ID_DVD_CONFIG,
	SC_ITEM_ID_WWW_RESTRICTION,
	SC_ITEM_ID_MAX
} SCItemID;

typedef void (*SCAsyncCallback)(s32 result);

#define SC_CONF_MIN_SIZE (sizeof(SCConfHeader) + sizeof(u32))
#define SC_CONF_MAX_SIZE 0x4000

typedef struct SCConfHeader {
	u32 magic;         // 0x00
	u16 numItems;      // 0x04
	u16 itemOffsets[]; // 0x06
} SCConfHeader;

typedef struct SCConfItem {
	u8 desc;     // 0x00
	char name[]; // 0x04
} SCConfItem;

typedef enum {
	SC_CONF_FILE_SYSTEM,  // SYSCONF
	SC_CONF_FILE_PRODUCT, // setting.txt
	SC_CONF_FILE_MAX
} SCConfFile;

typedef struct SCControl {
	OSThreadQueue threadQueue;     // 0x00
	NANDFileInfo fileInfo;         // 0x08
	NANDCommandBlock commandBlock; // 0x94

	union {
		NANDStatus fileAttr;
		u8 fileType;
	}; // 0x14C

	u8 nandCbState;  // 0x154
	u8 isFileOpen;   // 0x155
	u8 openFileType; // 0x156

	SCAsyncCallback asyncCallback; // 0x158
	s32 asyncResult;               // 0x15C

	const char* filePaths[SC_CONF_FILE_MAX]; // 0x160
	u8* fileBuffers[SC_CONF_FILE_MAX];       // 0x168
	u32 bufferSizes[SC_CONF_FILE_MAX];       // 0x170
	u32 fileSizes[SC_CONF_FILE_MAX];         // 0x178

	SCFlushCallback flushCallback; // 0x180
	SCStatus flushStatus;          // 0x184
	u32 flushSize;                 // 0x188
} SCControl;

typedef struct SCItem {
	char unk_0x00[0x8];

	u8 primType;  // 0x08
	u8 arrayType; // 0x09

	u32 nameLen;      // 0x0C
	u32 dataLen;      // 0x10
	const char* name; // 0x14
	u8* data;         // 0x18

	u32 itemLen; // 0x1C
} SCItem;

BOOL SCFindByteArrayItem(void* dst, u32 len, SCItemID id);
BOOL SCReplaceByteArrayItem(const void* src, u32 len, SCItemID id);

BOOL SCFindU8Item(u8* dst, SCItemID id);
BOOL SCFindS8Item(s8* dst, SCItemID id);
BOOL SCFindU32Item(u32* dst, SCItemID id);
BOOL SCFindU64Item(u64* dst, SCItemID id);
BOOL SCFindBoolItem(BOOL* dst, SCItemID id);

BOOL SCReplaceU8Item(u8 dst, SCItemID id);
BOOL SCReplaceS8Item(s8 dst, SCItemID id);
BOOL SCReplaceU32Item(u32 dst, SCItemID id);
BOOL SCReplaceU64Item(u64 dst, SCItemID id);
BOOL SCReplaceBoolItem(BOOL dst, SCItemID id);

BOOL __SCIsDirty();
void __SCClearConfBuf();
void __SCSetDirtyFlag();
void __SCClearDirtyFlag();
u8* __SCGetConfBuf();
u32 __SCGetConfBufSize();

/* SYSCONF BTE */

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

u32 SCGetBtDpdSensibility();
BOOL SCSetBtDpdSensibility(u32 dpdSensibility);

END_SCOPE_EXTERN_C

#endif
