/* WPAD control block from projectPiki/pik1wii (CC0), adapted to the September 2008 SDK. */
#ifndef REVOLUTION_WPAD_INTERNAL_H
#define REVOLUTION_WPAD_INTERNAL_H

#include "revolution/os.h"
#include "revolution/wpad.h"

#define WPAD_RX_DATASIZE         96
#define WPAD_COMMAND_CMD_MAX_LEN 24
#define WPAD_COMMAND_EXT_MAX_LEN 12
#define WPAD_COMMAND_BUF_LEN     22

#define WPAD_FILTER_ACC  0
#define WPAD_FILTER_DPD  1
#define WPAD_FILTER_EXT  2
#define WPAD_MAX_FILTERS 3

#define WPAD_HIDREP_VIBRATOR 0x10
#define WPAD_HIDREP_PORT     0x11
#define WPAD_HIDREP_REPTYPE  0x12
#define WPAD_HIDREP_ENDPD    0x13
#define WPAD_HIDREP_ENSPK    0x14
#define WPAD_HIDREP_GETSTAT  0x15
#define WPAD_HIDREP_WRDATA   0x16
#define WPAD_HIDREP_RDDATA   0x17
#define WPAD_HIDREP_STRM     0x18
#define WPAD_HIDREP_MUTE     0x19
#define WPAD_HIDREP_DPDCSB   0x1A
#define WPAD_HIDREP_WAIT     0xFF

#define WPAD_I2CDEV_SEL_MASK  (u32)(0xFF000000)
#define WPAD_I2CDEV_DADR_MASK (u32)(0x00FF0000)
#define WPAD_I2CDEV_UADR_MASK (u32)(0x0000FF00)
#define WPAD_I2CDEV_LADR_MASK (u32)(0x000000FF)
#define WPAD_WRITE_LEN_MASK   (u16)(0x001F)
#define WPAD_READ_ULEN_MASK   (u16)(0xFF00)
#define WPAD_READ_LLEN_MASK   (u16)(0x00FF)

#define SPK_CLK_REG   0x04a20009
#define SPK_CLK_ON    1
#define SPK_CLK_OFF   0
#define SPK_RST_REG   0x04a20001
#define SPK_DERESET   0x01
#define SPK_RESET     0x80
#define SPK_CTRL_REG  0x04a20008
#define SPK_CTRL_PLAY 1

typedef struct WPADCommand {
	u32 command;
	u8 data[22];
	u16 len;
	void* readBuf;
	u16 readLen;
	u32 readAddr;
	WPADInfo* info;
	WPADCallback callback;
} WPADCommand;

typedef struct WPADCmdQueue {
	s8 head;
	s8 tail;
	WPADCommand* cmd;
	u32 cmdlen;
} WPADCmdQueue;

typedef struct WPADConfig {
	DPDObject obj[WPAD_DPD_MAX_OBJECTS];
	WPADAcc acc_0g;
	WPADAcc acc_1g;
	u8 motor;
	u8 volume;
} WPADConfig;

typedef struct WPADStick {
	s16 x;
	s16 x_min;
	s16 x_max;
	s16 y;
	s16 y_min;
	s16 y_max;
} WPADStick;

typedef struct WPADFsConfig {
	WPADStick stick;
	WPADAcc acc_0g;
	WPADAcc acc_1g;
} WPADFsConfig;

typedef struct WPADClConfig {
	WPADStick lstk;
	WPADStick rstk;
	u8 triggerL;
	u8 triggerR;
} WPADClConfig;

typedef struct WPADExtConfig {
	union {
		WPADFsConfig fs;
		WPADClConfig cl;
	};
} WPADExtConfig;

typedef struct WPADMEMControlBlock {
	BOOL used;
	u8* p_buf;
	u16 len;
	u32 addr;
	WPADCallback callback;
} WPADMEMControlBlock;

typedef struct WPADControlBlock {
	WPADMEMGameInfo gameInfo;
	s32 gameInfoErr[2];
	u8 prBuf[WPAD_RX_DATASIZE] ATTRIBUTE_ALIGN(32);
	u8 rxBuf[WPAD_RX_DATASIZE * 2] ATTRIBUTE_ALIGN(32);
	WPADCmdQueue cmdq;
	WPADCommand cmds[WPAD_COMMAND_CMD_MAX_LEN];
	WPADCmdQueue extq;
	WPADCommand exts[WPAD_COMMAND_EXT_MAX_LEN];
	WPADInfo infos;
	WPADInfo* infoBuf;
	WPADConfig conf;
	WPADExtConfig extConf;
	WPADCallback resultCallback;
	WPADExtensionCallback extensionCallback;
	WPADConnectCallback connectCallback;
	WPADSamplingCallback sampleCallback;
	void* samplingBufs_ptr;
	u32 bufLatestIndex;
	u32 bufLength;
	u32 dataFormat;
	s32 status;
	u8 getStatFlag;
	u8 devType;
	u8 devMode;
	s8 dev_handle;
	WPADCallback reqVolCb;
	u8 rxBufIndex;
	u8 reqVolume;
	u8 dpdDummyObjSize;
	u8 dpdFormat;
	u8 dpdCommand;
	u8 audioStop;
	u8 audioStopCnt;
	u8 audioFrames;
	BOOL rumble;
	BOOL motor;
	BOOL used;
	BOOL setup;
	BOOL oldFw;
	OSThreadQueue threadq;
	OSTime lastUpdateTime;
	u16 filterDiff[WPAD_MAX_FILTERS];
	u16 filterSame[WPAD_MAX_FILTERS];
	OSTime cmdTimer;
	u8 cmdTimeoutAction;
	u8 isSetStickOrigin;
	u16 recalibrateCount;
	u8 key[16];
	u8 ft[8];
	u8 sb[8];
	u8 readBuf[64];
	void* readBufPtr;
	u32 readBaseAddr;
	s32 readError;
	u16 readLength;
	u8 keyIdx;
	u8 radioSense;
	u16 packetCnt;
	u8 disconnect;
	u8 cmdId;
	WPADCallback infoCallback;
	u8 infoLocked;
	u8 extEnc;
	u8 pwrSave;
	u8 balanceBoardBattery;
	u8 lastExtensionType;
	u8 keepExtensionCalibration;
	s16 calibrationTimeout;
	u8 dpdCommandPending;
	WPADMEMControlBlock mem;
} WPADControlBlock;

STATIC_ASSERT((size_t) & ((WPADControlBlock*)0)->status == 0x8BC);
STATIC_ASSERT((size_t) & ((WPADControlBlock*)0)->threadq == 0x8E4);
STATIC_ASSERT((size_t) & ((WPADControlBlock*)0)->key == 0x914);
STATIC_ASSERT((size_t) & ((WPADControlBlock*)0)->ft == 0x924);
STATIC_ASSERT((size_t) & ((WPADControlBlock*)0)->sb == 0x92C);
STATIC_ASSERT((size_t) & ((WPADControlBlock*)0)->mem == 0x998);
STATIC_ASSERT(sizeof(WPADControlBlock) == 0x9C0);

extern WPADControlBlock* _wpdcb[WPAD_MAX_CONTROLLERS];

void WPADiClearMemBlock(WPADChannel channel);
s32 WPADWriteExtReg(WPADChannel channel, const void* data, u16 length, u16 address, WPADCallback callback);
BOOL WPADiSendWriteData(WPADCmdQueue* queue, const void* data, u16 length, u32 address, WPADCallback callback);

void WPADiClearQueue(WPADCmdQueue* queue);
BOOL WPADiSendSetReportType(WPADCmdQueue* queue, u32 format, BOOL powerSave, WPADCallback callback);
BOOL WPADiSendWriteDataCmd(WPADCmdQueue* queue, u8 command, u32 address, WPADCallback callback);
BOOL WPADiSendReadData(WPADCmdQueue* queue, void* buffer, u16 length, u32 address, WPADCallback callback);
void WPADiCopyOut(s32 chan);
void WPADiExcludeButton(s32 chan);
s32 WPADiHIDParser(u8 chan, u8* report);

void DEBUGPrint(const char* format, ...);
void WPADiCreateKey(WPADChannel channel);
void WPADiCreateKeyFor3rd(WPADChannel channel);
void WPADiDecode(WPADChannel channel, void* data, u16 length, u16 offset);

#endif
