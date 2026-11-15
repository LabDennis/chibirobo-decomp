/* After the CC0 projectPiki/pik1wii WPAD.c (11 Dec 2007), brought to the SDK of 11 Sep 2008. */
#include "WPADInternal.h"
#include "WPADExtern.h"
#include "revolution/os/OSReset.h"
#include <string.h>

const char* __WPADVersion = "<< RVL_SDK - WPAD \trelease build: Sep 11 2008 17:33:40 (0x4199_60831) >>";

extern BOOL __OSIsReturnToIdle;

static OSAlarm _managerAlarm;
WPADControlBlock* _wpdcb[WPAD_MAX_CONTROLLERS];
static u8 __WPADiManageHandlerStack[4096] ATTRIBUTE_ALIGN(32);
static s8 _dev_handle_index[WUD_MAX_DEV_ENTRY];
WPADControlBlock _wpd[WPAD_MAX_CONTROLLERS];

static int _recFlag = -1;

static u16 __WPAD_diff_count_threshold[WPAD_MAX_FILTERS] = { 6, 4, 6 };
static u16 __WPAD_hyst_count_threshold[WPAD_MAX_FILTERS] = { 30, 30, 30 };

static u8 _gametype;
static const char* _gamecode;
static u8 _sleepTime;
static u8 _dpdSensitivity;
static u8 _sensorBarPos;
static BOOL _rumble;
static u8 _speakerVolume;
static u8 _scSetting;
static u8 _shutdown;
static s8 _afhChannel;
static u8 _chan_active_state[WPAD_MAX_CONTROLLERS];
static BOOL _initialized;
static BOOL _usedCallback;
BOOL _enabledDRM;
BOOL _enabledGTR;
BOOL _enabledTRN;
BOOL _enabledVSM;
void (*_wpadDRMInit)(void);
void (*_wpadGTRInit)(void);
void (*_wpadTRNInit)(void);
void (*_wpadVSMInit)(void);
static int _recCnt;
static BOOL _startup;
static u8 _rumbleCnt[WPAD_MAX_CONTROLLERS];
static u8 _extCnt[WPAD_MAX_CONTROLLERS];
static u16 _afhCnt;
static u8 _checkCnt;
static u16 _senseCnt;
static u8 _regShutdown;

#define ACRReadReg(offset)       (*(vu32*)(0xCD000000 + (offset)))
#define ACRWriteReg(offset, val) (*(vu32*)(0xCD000000 + (offset)) = (val))

static void __wpadSendDataSub(s32 chan, WPADCommand command);

static void __wpadConnectionCallback(WUDDevInfo* info, u8 open);
static void __wpadReceiveCallback(u8 dev_handle, u8* p_rpt, u16);
static void __wpadInitConnectionCallback(s32 chan, s32 result);
BOOL WPADiSendSetPort(WPADCmdQueue*, u8, WPADCallback);
BOOL WPADiSendDPDCSB(WPADCmdQueue*, BOOL, WPADCallback);
BOOL WPADiSendGetContStat(WPADCmdQueue*, WPADInfo*, WPADCallback);
BOOL WPADiSendEnableDPD(WPADCmdQueue*, BOOL, WPADCallback);
BOOL WPADiSendEnableSpeaker(WPADCmdQueue*, BOOL, WPADCallback);
BOOL WPADiSendMuteSpeaker(WPADCmdQueue*, BOOL, WPADCallback);
BOOL WPADiSendStreamData(WPADCmdQueue*, void*, u16);

u16 _WPADGetStackBufferStatus(s32 chan);
u16 _WPADGetModuleBufferStatus(s32 chan);

static void __wpadShutdown(BOOL exec);
static void __wpadCleanup(void);
static void __wpadResetModule(BOOL exec);
static void __wpadReconnectStart(BOOL exec);
static s32 WPADiGetStatus(s32 chan);
static void WPADiDisconnect(s32 chan, BOOL sleep);

BOOL __CanPushCmdQueue(WPADCmdQueue* queue, s8 cmd_num);
s8 __GetCmdNumber(WPADCmdQueue* queue);
BOOL WPADiPushCommand(WPADCmdQueue* queue, WPADCommand cmd);
BOOL WPADiGetCommand(WPADCmdQueue* queue, WPADCommand* cmd);
BOOL WPADiPopCommand(WPADCmdQueue* queue);

static BOOL __SetSensorBarPower(BOOL flag);
static void __SetScreenSaverFlag(BOOL disable);
static u8 __GetDpdSensitivity(void);
static u8 __GetSensorBarPosition(void);
static BOOL __GetMotorMode(void);
static u8 __ClampSpeakerVolume(u8 volume);
static u8 __GetSpeakerVolume(void);

BOOL __SetSensorBarPower(BOOL flag)
{
	u32 reg;
	u32 regBak;
	BOOL enable;
	BOOL result;

	enable = OSDisableInterrupts();

	regBak = ACRReadReg(0xC0);

	if (flag) {
		reg = regBak | 0x100;
	} else {
		reg = regBak & ~0x100;
	}

	ACRWriteReg(0xC0, reg);
	result = (regBak & 0x100) ? TRUE : FALSE;
	OSRestoreInterrupts(enable);
	return result;
}

static void __SetScreenSaverFlag(BOOL flag)
{
	if (flag) {
		__VIResetRFIdle();
	}
}

static u8 __GetDpdSensitivity()
{
	u8 level;

	level = (u8)SCGetBtDpdSensibility();
	if (level < 1) {
		level = 1;
	}
	if (level > 5) {
		level = 5;
	}

	return level;
}

static u8 __GetSensorBarPosition()
{
	u8 pos;

	if (1 == SCGetWpadSensorBarPosition()) {
		pos = WPAD_SENSOR_BAR_POS_TOP;
	} else {
		pos = WPAD_SENSOR_BAR_POS_BOTTOM;
	}
	return pos;
}

static BOOL __GetMotorMode()
{
	BOOL rumble;

	if (1 == SCGetWpadMotorMode()) {
		rumble = TRUE;
	} else {
		rumble = FALSE;
	}
	return rumble;
}

static u8 __ClampSpeakerVolume(u8 vol)
{
	u8 v = vol;

	if (vol <= 0) {
		v = 0;
	}
	if (vol >= 127) {
		v = 127;
	}

	return v;
}

static u8 __GetSpeakerVolume()
{
	u8 vol;
	vol = SCGetWpadSpeakerVolume();
	vol = __ClampSpeakerVolume(vol);
	return vol;
}

static void* __wpadNoAlloc(u32 size)
{
	return NULL;
}

static BOOL __wpadNoFree(void* ptr)
{
	return FALSE;
}

BOOL WPADIsEnabledVSM(void)
{
	return _enabledVSM;
}

BOOL WPADIsEnabledTRN(void)
{
	return _enabledTRN;
}

BOOL WPADIsEnabledGTR(void)
{
	return _enabledGTR;
}

BOOL WPADIsEnabledDRM(void)
{
	return _enabledDRM;
}

s32 WBCSetupCalibration(void)
{
	return WPAD_ERR_NONE;
}

s32 WBCGetCalibrationStatus(void)
{
	return 0;
}

s32 WBCGetBatteryLevel(u8 battery)
{
	return 0;
}

s32 WBCReadDummy(WPADBLStatus* status, f64* weight, u32 size)
{
	DEBUGPrint("WBCReadDummy\n");
	return WPAD_ERR_NO_CONTROLLER;
}

s32 WBCSetZEROPointDummy(f64* press, u32 size)
{
	DEBUGPrint("WBCSetZEROPointDummy\n");
	return WPAD_ERR_NO_CONTROLLER;
}

s32 WBCGetTGCWeightDummy(f64 weight, f64* tgcWeight, WPADBLStatus* status)
{
	DEBUGPrint("WBCGetTGCWeightDummy\n");
	return WPAD_ERR_NO_CONTROLLER;
}

static BOOL OnShutdown(BOOL final, u32 event)
{
	BOOL cleanup;
	s32 status = WUDGetStatus();

	if (final != FALSE || status == 0) {
		return TRUE;
	}

	if (status == 4 || status == 1 || status == 2) {
		return FALSE;
	}

	if (WUDIsBusy()) {
		WPADCancelSyncDevice();
		return FALSE;
	}

	switch (event) {
	case 0:
		WPADRegisterAllocator(__wpadNoAlloc, __wpadNoFree);

	case 2:
	case 3:
		cleanup = TRUE;
		break;
	case 1:
	case 4:
	case 6:
		cleanup = FALSE;
		break;
	case 5:
		cleanup = __OSIsReturnToIdle ? TRUE : FALSE;
		break;
	}

	if (cleanup) {
		__wpadCleanup();
	} else {
		__wpadReconnectStart(TRUE);
	}

	return FALSE;
}

static OSShutdownFunctionInfo ShutdownFunctionInfo = { OnShutdown, 127 };

static void __wpadSendDataSub(s32 chan, WPADCommand cmd)
{
	BOOL enable;
	BOOL motor;
	BT_HDR* p_buf = NULL;
	u8* ptr;
	s8 handle;
	s32 status;
	WPADControlBlock* p_wpd;
	u8 rep_id;
	u8* p_data;
	u16 len;

	p_wpd  = _wpdcb[chan];
	rep_id = (u8)cmd.command;
	p_data = cmd.data;
	len    = cmd.len;

	enable = OSDisableInterrupts();
	status = p_wpd->status;
	handle = p_wpd->dev_handle;
	if (handle < 0) {
		OSRestoreInterrupts(enable);
		return;
	}
	p_wpd->status = WPAD_ERR_BUSY;
	motor         = p_wpd->motor & _rumble;

	if (rep_id == WPAD_HIDREP_VIBRATOR) {
		p_wpd->status = status;
	} else if (rep_id == WPAD_HIDREP_STRM) {
		p_wpd->status = status;
		p_wpd->audioFrames--;
	} else {
		switch (rep_id) {
		case WPAD_HIDREP_WRDATA:
			break;

		case WPAD_HIDREP_RDDATA:
			p_wpd->readError    = 0;
			p_wpd->readBaseAddr = cmd.readAddr;
			p_wpd->readLength   = cmd.readLen;
			p_wpd->readBufPtr   = cmd.readBuf;
			break;

		case WPAD_HIDREP_GETSTAT:
			p_wpd->status      = status;
			p_wpd->infoBuf     = cmd.info;
			p_wpd->getStatFlag = 1;
			break;

		default:
			p_data[0] |= 2;
			break;
		}

		p_wpd->resultCallback   = cmd.callback;
		p_wpd->cmdId            = rep_id;
		p_wpd->cmdTimer         = OSSecondsToTicks(2) + __OSGetSystemTime();
		p_wpd->cmdTimeoutAction = 0;
	}
	OSRestoreInterrupts(enable);

	DEBUGPrint("handle = %d, repid = %02x\n", handle, rep_id);

	p_buf         = GKI_getbuf((u8)(10 + len + sizeof(BT_HDR)));
	p_buf->len    = (u8)(len + 1);
	p_buf->offset = 10;
	ptr           = (u8*)(p_buf + 1) + p_buf->offset;

	ptr[0] = rep_id;

	memcpy(ptr + 1, p_data, len);

	if (motor) {
		ptr[1] |= 1;
	} else {
		ptr[1] &= ~1;
	}

	BTA_HhSendData((u8)handle, p_buf);
}

static s32 WPADiSendData(s32 chan, WPADCommand cmd)
{
	WPADControlBlock* p_wpd = _wpdcb[chan];
	s32 status;
	BOOL enable;

	enable = OSDisableInterrupts();
	status = p_wpd->getStatFlag ? WPAD_ERR_BUSY : WPADiGetStatus(chan);
	OSRestoreInterrupts(enable);

	if (status == WPAD_ERR_NONE) {
		__wpadSendDataSub(chan, cmd);
	}

	if (status == WPAD_ERR_BUSY) {
		if ((int)OSTicksToSeconds(__OSGetSystemTime() - p_wpd->cmdTimer) > 1 && _sleepTime > 0) {
			p_wpd->cmdTimer = __OSGetSystemTime();
			WPADiDisconnect(chan, FALSE);
		}
	}
	return status;
}

static void __wpadCalcRadioQuality(s32 chan)
{
	WPADControlBlock* p_wpd = _wpdcb[chan];
	u32 packet              = 2;
	u16 rs;

	if (_senseCnt != 10) {
		return;
	}

	rs = (u16)(p_wpd->radioSense * (10 - 1));
	rs += (u16)((p_wpd->packetCnt * 100) / packet);
	rs /= 10;
	rs = (u16)((rs > 100) ? 100 : rs);

	p_wpd->radioSense = (u8)rs;
	p_wpd->packetCnt  = 0;

	if (p_wpd->audioStop) {
		if (rs > 85) {
			p_wpd->audioStop    = 0;
			p_wpd->audioStopCnt = 0;
		} else if (rs > 80) {
			p_wpd->audioStopCnt++;
			if (p_wpd->audioStopCnt >= 20) {
				p_wpd->audioStop    = 0;
				p_wpd->audioStopCnt = 0;
			}
		}
	} else {
		if (rs < 75) {
			p_wpd->audioStop    = 1;
			p_wpd->audioStopCnt = 0;
		} else if (rs < 80) {
			p_wpd->audioStopCnt++;
			if (p_wpd->audioStopCnt >= 1) {
				p_wpd->audioStop    = 1;
				p_wpd->audioStopCnt = 0;
			}
		}
	}
}

static u32 __wpadFmt2Size(u32 fmt)
{
	u32 size;

	switch (fmt) {
	case WPAD_FMT_FREESTYLE:
	case WPAD_FMT_FREESTYLE_ACC:
	case WPAD_FMT_FREESTYLE_ACC_DPD:
		size = sizeof(WPADFSStatus);
		break;
	case WPAD_FMT_CLASSIC:
	case WPAD_FMT_CLASSIC_ACC:
	case WPAD_FMT_CLASSIC_ACC_DPD:
	case WPAD_FMT_GUITAR:
	case WPAD_FMT_DRUM:
		size = sizeof(WPADCLStatus);
		break;
	case WPAD_FMT_TRAIN:
		size = sizeof(WPADTRStatus);
		break;
	case WPAD_FMT_BALANCE_CHECKER:
		size = sizeof(WPADBLStatus);
		break;
	case WPAD_FMT_VSM:
	case WPAD_FMT_VSM_EX:
		size = sizeof(WPADVSStatus);
		break;
	case WPAD_FMT_CORE_ACC_DPD_FULL:
		size = sizeof(WPADStatusEx);
		break;
	default:
		size = sizeof(WPADStatus);
		break;
	}

	return size;
}

static u8 IsButtonChanged(u16 curr, u16 prev)
{
	return (u8)((curr != prev) ? 1 : 0);
}

static u8 IsAnalogChanged(s32 curr, s32 prev, s32 threshold)
{
	s32 diff = ((curr - prev) < 0) ? prev - curr : curr - prev;
	return (u8)((diff > threshold) ? 1 : 0);
}

static BOOL __wpadCalcAnalogNoise(WPADControlBlock* p_wpd, s32 filter, BOOL changed)
{
	if (changed) {
		p_wpd->filterDiff[filter]++;
		if (p_wpd->filterDiff[filter] > __WPAD_diff_count_threshold[filter]) {
			p_wpd->filterDiff[filter] = 0;
			p_wpd->filterSame[filter] = 0;
			return TRUE;
		}
	} else {
		p_wpd->filterSame[filter] = (u16)((p_wpd->filterSame[filter] + 1) % __WPAD_hyst_count_threshold[filter]);
		if (p_wpd->filterSame[filter] == __WPAD_hyst_count_threshold[filter] - 1) {
			if (p_wpd->filterDiff[filter] > 0) {
				p_wpd->filterDiff[filter]--;
			}
		}
	}
	return FALSE;
}

static BOOL __wpadIsControllerDataChanged(WPADControlBlock* p_wpd, void* p_curr, void* p_prev)
{
	WPADStatus* curr = (WPADStatus*)p_curr;
	WPADStatus* prev = (WPADStatus*)p_prev;
	u8 result        = 0;
	int i;

	if (p_wpd->dataFormat != WPAD_FMT_VSM_EX) {
		result |= IsButtonChanged(curr->button, prev->button);
		if (curr->err == WPAD_ERR_NONE && prev->err == WPAD_ERR_NONE) {
			result |= __wpadCalcAnalogNoise(p_wpd, WPAD_FILTER_ACC,
			                                IsAnalogChanged(curr->accX, prev->accX, 12) | IsAnalogChanged(curr->accY, prev->accY, 12)
			                                    | IsAnalogChanged(curr->accZ, prev->accZ, 12));

			result |= __wpadCalcAnalogNoise(
			    p_wpd, WPAD_FILTER_DPD,
			    IsAnalogChanged(curr->obj[0].x, prev->obj[0].x, 2) | IsAnalogChanged(curr->obj[0].y, prev->obj[0].y, 2)
			        | IsAnalogChanged(curr->obj[1].x, prev->obj[1].x, 2) | IsAnalogChanged(curr->obj[1].y, prev->obj[1].y, 2)
			        | IsAnalogChanged(curr->obj[2].x, prev->obj[2].x, 2) | IsAnalogChanged(curr->obj[2].y, prev->obj[2].y, 2)
			        | IsAnalogChanged(curr->obj[3].x, prev->obj[3].x, 2) | IsAnalogChanged(curr->obj[3].y, prev->obj[3].y, 2));
		}
	}

	if (curr->err == WPAD_ERR_NONE && prev->err == WPAD_ERR_NONE) {
		switch (p_wpd->dataFormat) {
		case WPAD_FMT_FREESTYLE:
		case WPAD_FMT_FREESTYLE_ACC:
		case WPAD_FMT_FREESTYLE_ACC_DPD: {
			WPADFSStatus* fsCurr = (WPADFSStatus*)p_curr;
			WPADFSStatus* fsPrev = (WPADFSStatus*)p_prev;

			result |= __wpadCalcAnalogNoise(p_wpd, WPAD_FILTER_EXT,
			                                IsAnalogChanged(fsCurr->fsAccX, fsPrev->fsAccX, 12)
			                                    | IsAnalogChanged(fsCurr->fsAccY, fsPrev->fsAccY, 12)
			                                    | IsAnalogChanged(fsCurr->fsAccZ, fsPrev->fsAccZ, 12));
			result |= IsAnalogChanged(fsCurr->fsStickX, fsPrev->fsStickX, 1);
			result |= IsAnalogChanged(fsCurr->fsStickY, fsPrev->fsStickY, 1);
		} break;

		case WPAD_FMT_CLASSIC:
		case WPAD_FMT_CLASSIC_ACC:
		case WPAD_FMT_CLASSIC_ACC_DPD:
		case WPAD_FMT_GUITAR:
		case WPAD_FMT_DRUM: {
			WPADCLStatus* clCurr = (WPADCLStatus*)p_curr;
			WPADCLStatus* clPrev = (WPADCLStatus*)p_prev;
			s32 lstk;
			s32 rstk;
			s32 trig;

			switch (p_wpd->devMode) {
			case 1:
				lstk = 16;
				rstk = 32;
				trig = 8;
				break;
			case 3:
				lstk = 4;
				rstk = 4;
				trig = 1;
				break;
			default:
				lstk = 1;
				rstk = 1;
				trig = 1;
				break;
			}

			result |= IsButtonChanged(clCurr->clButton, clPrev->clButton);
			result |= IsAnalogChanged(clCurr->clLStickX / lstk, clPrev->clLStickX / lstk, 1);
			result |= IsAnalogChanged(clCurr->clLStickY / lstk, clPrev->clLStickY / lstk, 1);
			result |= IsAnalogChanged(clCurr->clRStickX / rstk, clPrev->clRStickX / rstk, 1);
			result |= IsAnalogChanged(clCurr->clRStickY / rstk, clPrev->clRStickY / rstk, 1);
			result |= IsAnalogChanged(clCurr->clTriggerL / trig, clPrev->clTriggerL / trig, 1);
			result |= IsAnalogChanged(clCurr->clTriggerR / trig, clPrev->clTriggerR / trig, 1);
		} break;

		case WPAD_FMT_TRAIN: {
			WPADTRStatus* trCurr = (WPADTRStatus*)p_curr;
			WPADTRStatus* trPrev = (WPADTRStatus*)p_prev;

			result |= IsButtonChanged(trCurr->trButton, trPrev->trButton);
			result |= IsAnalogChanged(trCurr->brake, trPrev->brake, 1);
			result |= IsAnalogChanged(trCurr->mascon, trPrev->mascon, 1);
		} break;

		case WPAD_FMT_BALANCE_CHECKER: {
			WPADBLStatus* blCurr = (WPADBLStatus*)p_curr;
			WPADBLStatus* blPrev = (WPADBLStatus*)p_prev;

			result |= __wpadCalcAnalogNoise(
			    p_wpd, WPAD_FILTER_EXT,
			    IsAnalogChanged(blCurr->press[0], blPrev->press[0], 50) | IsAnalogChanged(blCurr->press[1], blPrev->press[1], 50)
			        | IsAnalogChanged(blCurr->press[2], blPrev->press[2], 50) | IsAnalogChanged(blCurr->press[3], blPrev->press[3], 50));
		} break;

		case WPAD_FMT_VSM:
		case WPAD_FMT_VSM_EX: {
			WPADVSStatus* vsCurr = (WPADVSStatus*)p_curr;
			WPADVSStatus* vsPrev = (WPADVSStatus*)p_prev;

			result |= IsButtonChanged(vsCurr->at_0x46, vsPrev->at_0x46);
			result |= IsAnalogChanged(vsCurr->at_0x44, vsPrev->at_0x44, 32);
			for (i = 0; i < 5; i++) {
				result |= IsAnalogChanged(vsCurr->at_0x2a[i], vsCurr->at_0x36[i], 32);
			}
		} break;
		}
	}

	return result;
}

static void __wpadCalcRecalibration(s32 chan, void* p_data)
{
	WPADControlBlock* p_wpd = _wpdcb[chan];
	WPADCLStatus* cp        = (WPADCLStatus*)p_data;
	s8 cnt                  = 0;

	if (p_wpd->dataFormat == WPAD_FMT_FREESTYLE || p_wpd->dataFormat == WPAD_FMT_FREESTYLE_ACC
	    || p_wpd->dataFormat == WPAD_FMT_FREESTYLE_ACC_DPD) {
		if (cp->button == 0x1C10) {
			cnt = 1;
		}
	}
	if (p_wpd->dataFormat == WPAD_FMT_CLASSIC || p_wpd->dataFormat == WPAD_FMT_CLASSIC_ACC
	    || p_wpd->dataFormat == WPAD_FMT_CLASSIC_ACC_DPD) {
		if (cp->button == 0x1C10 || (cp->clButton == 0x1450 && cp->err == WPAD_ERR_NONE)) {
			cnt = 1;
		}
	}

	p_wpd->recalibrateCount += cnt;

	if (p_wpd->recalibrateCount > 600) {
		WPADRecalibrate(chan);
	}
}

static void __wpadCalcControllerData(s32 chan)
{
	WPADControlBlock* p_wpd = _wpdcb[chan];
	BOOL diff               = FALSE;
	BOOL enable;
	BOOL screen = FALSE;
	void* p_data;
	u8 index;

	if (_checkCnt != 5) {
		return;
	}

	enable = OSDisableInterrupts();
	index  = (u8)((p_wpd->rxBufIndex) ? 0 : 1);
	p_data = (void*)(p_wpd->rxBuf + index * WPAD_RX_DATASIZE);
	OSRestoreInterrupts(enable);

	diff = __wpadIsControllerDataChanged(p_wpd, p_data, p_wpd->prBuf);
	__wpadCalcRecalibration(chan, p_data);

	if (diff) {
		screen                = TRUE;
		p_wpd->lastUpdateTime = __OSGetSystemTime();
		memcpy(p_wpd->prBuf, p_data, WPAD_RX_DATASIZE);
	} else {
		if (_sleepTime > 0) {
			int sec = (int)OSTicksToSeconds(__OSGetSystemTime() - p_wpd->lastUpdateTime);

			if (sec > 60 * _sleepTime) {
				WPADiDisconnect(chan, FALSE);
			}
		}
	}

	if (((WPADStatus*)p_wpd->prBuf)->err != WPAD_ERR_NONE) {
		memcpy(p_wpd->prBuf, p_data, WPAD_RX_DATASIZE);
	}

	__SetScreenSaverFlag(screen);
}

static BOOL WPADiProcessExtCommand(s32 chan, BOOL sendCmd)
{
	WPADControlBlock* p_wpd = _wpdcb[chan];
	WPADCommand cmd;
	BOOL result = FALSE;

	if (sendCmd) {
		return TRUE;
	}

	if (_extCnt[chan] == 5) {
		if (WPADiGetCommand(&p_wpd->extq, &cmd)) {
			if (cmd.command == WPAD_HIDREP_REPTYPE || p_wpd->infos.attach) {
				if (WPADiSendData(chan, cmd) == WPAD_ERR_NONE) {
					WPADiPopCommand(&p_wpd->extq);
					result        = TRUE;
					_extCnt[chan] = 0;
				}
			}
		}
	}

	_extCnt[chan] = (u8)((_extCnt[chan] == 5) ? _extCnt[chan] : _extCnt[chan] + 1);
	return result;
}

static void WPADiRumbleMotor(s32 chan, BOOL sendCmd)
{
	WPADCommand cmd;

	if (sendCmd == TRUE || __GetCmdNumber(&_wpdcb[chan]->cmdq) > 0) {
		_wpdcb[chan]->rumble = FALSE;
	} else if (_rumbleCnt[chan] == 5) {
		_wpdcb[chan]->rumble = FALSE;

		cmd.command  = WPAD_HIDREP_VIBRATOR;
		cmd.len      = 1;
		cmd.data[0]  = 0;
		cmd.callback = NULL;

		__wpadSendDataSub(chan, cmd);
	}
	_rumbleCnt[chan] = (u8)((_wpdcb[chan]->rumble) ? _rumbleCnt[chan] + 1 : 0);
}

static BOOL WPADiProcessCommand(s32 chan, BOOL sendCmd)
{
	WPADControlBlock* p_wpd = _wpdcb[chan];
	WPADCommand cmd;

	if (sendCmd) {
		return TRUE;
	}

	if (WPADiGetCommand(&p_wpd->cmdq, &cmd)) {
		if (WPADiSendData(chan, cmd) == WPAD_ERR_NONE) {
			WPADiPopCommand(&p_wpd->cmdq);
			return TRUE;
		}
	}
	return FALSE;
}

static void WPADiGetScSettings(void)
{
	if (SCCheckStatus() == 0) {
		WUDUpdateSCSetting();
		if (_scSetting) {
			_dpdSensitivity = __GetDpdSensitivity();
			_sensorBarPos   = __GetSensorBarPosition();
			_rumble         = __GetMotorMode();
			_speakerVolume  = __GetSpeakerVolume();
			_scSetting      = 0;
		}
	}
}

static void WPADiAfh(void)
{
	BOOL enable;
	u8* channel;

	if (_afhCnt != 60000) {
		return;
	}

	channel = (u8*)OSPhysicalToCached(0x31A2);
	DCInvalidateRange(channel, 1);
	DEBUGPrint("Check the update of WiFi using channel\n");
	DEBUGPrint("WiFi uses channel = %d\n", *channel);

	if (_afhChannel != *channel) {
		enable      = OSDisableInterrupts();
		_afhChannel = (s8)*channel;
		OSRestoreInterrupts(enable);
		WUDSetDisableChannel(_afhChannel);
	}
}

static BOOL __reconnect(void)
{
	if (_recFlag >= 0) {
		_recCnt--;
		if (_recCnt <= 0) {
			__wpadResetModule((BOOL)_recFlag);
		}
		return TRUE;
	}
	return FALSE;
}

static void __wpadManageHandler(OSAlarm* alarm, OSContext* context)
{
	s32 chan;
	s32 status;
	BOOL sendCmd;

	status = WPADGetStatus();

	if (status != WPAD_STATE_SETUP) {
		if (status == WPAD_STATE_ENABLED) {
			if (!_initialized) {
				_initialized = TRUE;
				_recCnt      = 50;
				WUDSetHidConnCallback(__wpadConnectionCallback);
				WUDSetHidRecvCallback(__wpadReceiveCallback);
			}
		}
		return;
	}

	if (__reconnect()) {
		return;
	}

	WPADiAfh();
	WPADiGetScSettings();

	for (chan = 0; chan < WPAD_MAX_CONTROLLERS; chan++) {
		sendCmd = FALSE;
		if (_wpdcb[chan]->used) {
			sendCmd |= WPADiProcessExtCommand(chan, sendCmd);
			sendCmd |= WPADiProcessCommand(chan, sendCmd);

			WPADiRumbleMotor(chan, sendCmd);
			__wpadCalcControllerData(chan);
			__wpadCalcRadioQuality(chan);
		}

		if (_wpdcb[chan]->keepExtensionCalibration && _wpdcb[chan]->calibrationTimeout-- < 0) {
			_wpdcb[chan]->keepExtensionCalibration = FALSE;
		}
	}

	_senseCnt = (u16)((_senseCnt == 10) ? 0 : _senseCnt + 1);
	_checkCnt = (u8)((_checkCnt == 5) ? 0 : _checkCnt + 1);
	_afhCnt   = (u16)((_afhCnt == 60000) ? 0 : _afhCnt + 1);

	BTA_HhGetAclQueueInfo();
}

static void __wpadManageHandler0(OSAlarm* alarm, OSContext* context)
{
	OSSwitchFiberEx((u32)alarm, (u32)context, 0, 0, (u32)__wpadManageHandler,
	                (u32)(__WPADiManageHandlerStack + sizeof(__WPADiManageHandlerStack)));
}

static void __wpadClearControlBlock(s32 chan)
{
	WPADControlBlock* p_wpd = _wpdcb[chan];
	int i;

	p_wpd->infoBuf = NULL;
	p_wpd->motor   = FALSE;

	p_wpd->resultCallback    = NULL;
	p_wpd->extensionCallback = NULL;
	p_wpd->sampleCallback    = NULL;
	p_wpd->samplingBufs_ptr  = NULL;
	p_wpd->bufLatestIndex    = 0;
	p_wpd->bufLength         = 0;
	p_wpd->dataFormat        = WPAD_FMT_CORE;
	p_wpd->status            = WPAD_ERR_NO_CONTROLLER;
	p_wpd->devType           = WPAD_DEV_NOT_FOUND;
	p_wpd->devMode           = 0;
	p_wpd->isSetStickOrigin  = 0;
	p_wpd->recalibrateCount  = 0;
	p_wpd->getStatFlag       = 0;
	p_wpd->dpdDummyObjSize   = 12;
	p_wpd->dpdFormat         = 0;
	p_wpd->dpdCommand        = 0;

	for (i = 0; i < WPAD_MAX_FILTERS; i++) {
		p_wpd->filterDiff[i] = 0;
		p_wpd->filterSame[i] = 0;
	}

	p_wpd->lastUpdateTime   = __OSGetSystemTime();
	p_wpd->cmdTimer         = __OSGetSystemTime();
	p_wpd->cmdTimeoutAction = 0;
	p_wpd->reqVolume        = 0;
	p_wpd->reqVolCb         = NULL;

	p_wpd->readBufPtr   = NULL;
	p_wpd->readBaseAddr = 0;
	p_wpd->readLength   = 0;
	p_wpd->readError    = 0;

	p_wpd->dev_handle   = -1;
	p_wpd->used         = FALSE;
	p_wpd->setup        = FALSE;
	p_wpd->oldFw        = FALSE;
	p_wpd->audioStop    = 1;
	p_wpd->audioStopCnt = 0;
	p_wpd->audioFrames  = 0;

	p_wpd->keyIdx     = 0;
	p_wpd->radioSense = 0;
	p_wpd->packetCnt  = 0;
	p_wpd->disconnect = 1;

	p_wpd->infoLocked   = 0;
	p_wpd->infoCallback = NULL;

	p_wpd->pwrSave                  = 0;
	p_wpd->balanceBoardBattery      = 4;
	p_wpd->lastExtensionType        = WPAD_DEV_NOT_FOUND;
	p_wpd->keepExtensionCalibration = FALSE;
	p_wpd->calibrationTimeout       = 0;

	memset(&p_wpd->infos, 0, sizeof(WPADInfo));
	memset(p_wpd->readBuf, 0, sizeof(p_wpd->readBuf));
	memset(&p_wpd->conf, 0, sizeof(WPADConfig));
	memset(&p_wpd->extConf, 0, sizeof(WPADExtConfig));
	memset(&p_wpd->key, 0, sizeof(p_wpd->key));
	memset(&p_wpd->ft, 0, sizeof(p_wpd->ft));
	memset(&p_wpd->sb, 0, sizeof(p_wpd->sb));
	memset(&p_wpd->gameInfo, 0, sizeof(WPADMEMGameInfo));

	p_wpd->rxBufIndex = 0;
	memset(p_wpd->rxBuf, 0, WPAD_RX_DATASIZE * 2);
	((WPADStatus*)p_wpd->rxBuf)->err                      = WPAD_ERR_NO_CONTROLLER;
	((WPADStatus*)(p_wpd->rxBuf + WPAD_RX_DATASIZE))->err = WPAD_ERR_NO_CONTROLLER;
	memcpy(p_wpd->prBuf, p_wpd->rxBuf, WPAD_RX_DATASIZE);

	p_wpd->gameInfoErr[0] = WPAD_ERR_NO_CONTROLLER;
	p_wpd->gameInfoErr[1] = WPAD_ERR_NO_CONTROLLER;
	p_wpd->cmdq.cmd       = p_wpd->cmds;
	p_wpd->cmdq.cmdlen    = (u32)(sizeof(p_wpd->cmds) / sizeof(WPADCommand));
	p_wpd->extq.cmd       = p_wpd->exts;
	p_wpd->extq.cmdlen    = (u32)(sizeof(p_wpd->exts) / sizeof(WPADCommand));
	WPADiClearQueue(&p_wpd->cmdq);
	WPADiClearQueue(&p_wpd->extq);

	WPADiClearMemBlock(chan);

	_extCnt[chan]    = 0;
	_rumbleCnt[chan] = 0;
}

static void __wpadInitSub(void)
{
	s32 chan;
	int i;

	__SetSensorBarPower(TRUE);

	for (i = 0; i < WUD_MAX_DEV_ENTRY; i++) {
		_dev_handle_index[i] = -1;
	}

	DEBUGPrint("WPADInit()\n");

	for (chan = 0; chan < WPAD_MAX_CONTROLLERS; ++chan) {
		_wpdcb[chan]                  = &_wpd[chan];
		_chan_active_state[chan]      = 0;
		_wpdcb[chan]->connectCallback = NULL;
		__wpadClearControlBlock(chan);
		OSInitThreadQueue(&_wpd[chan].threadq);

		_extCnt[chan]    = 0;
		_rumbleCnt[chan] = 0;
	}

	_sleepTime      = 5;
	_gamecode       = OSGetAppGamename();
	_gametype       = OSGetAppType();
	_dpdSensitivity = __GetDpdSensitivity();
	_sensorBarPos   = __GetSensorBarPosition();
	_rumble         = __GetMotorMode();
	_speakerVolume  = __GetSpeakerVolume();
	_senseCnt       = 0;
	_checkCnt       = 0;
	_afhCnt         = 0;
	_shutdown       = 0;
	_scSetting      = 1;
	_afhChannel     = -1;
	_usedCallback   = FALSE;

	OSRegisterVersion(__WPADVersion);

	if (_wpadVSMInit) {
		_wpadVSMInit();
	}
	if (_wpadTRNInit) {
		_wpadTRNInit();
	}
	if (_wpadGTRInit) {
		_wpadGTRInit();
	}
	if (_wpadDRMInit) {
		_wpadDRMInit();
	}

	OSCreateAlarm(&_managerAlarm);
	OSSetPeriodicAlarm(&_managerAlarm, OSGetTime(), OSMillisecondsToTicks(1), __wpadManageHandler0);
}

void WPADInit(void)
{
	BOOL result;

	_startup = TRUE;

	if (_regShutdown == 0) {
		OSRegisterShutdownFunction(&ShutdownFunctionInfo);
		_regShutdown = 1;
	}

	result = WUDInit();

	if (result) {
		_initialized = FALSE;
		_recFlag     = -1;
		_recCnt      = 50;
		__wpadInitSub();
	}
}

// not in the dol; body as in pik1wii
void WPADShutdown(void)
{
	return;
}

static void __wpadShutdown(BOOL exec)
{
	OSCancelAlarm(&_managerAlarm);
	WUDSetHidRecvCallback(NULL);
	WUDShutdown(exec);
}

static void __wpadCleanup(void)
{
	BOOL enable;
	s32 chan;

	enable = OSDisableInterrupts();

	if (_shutdown) {
		OSRestoreInterrupts(enable);
		return;
	}

	_shutdown = 1;
	WUDSetVisibility(0, 0);

	for (chan = 0; chan < WPAD_MAX_CONTROLLERS; chan++) {
		WUDSetDeviceHistory(chan, NULL);
	}

	__wpadShutdown(FALSE);
	OSRestoreInterrupts(enable);
}

static void __wpadResetModule(BOOL exec)
{
	DEBUGPrint("Reconnect Start!!\n");
	BTA_DmSendHciReset();
	__wpadShutdown(exec);
}

static void __wpadReconnectStart(BOOL exec)
{
	BOOL enable = OSDisableInterrupts();

	if (!_startup) {
		OSRestoreInterrupts(enable);
		return;
	}

	if (_shutdown) {
		OSRestoreInterrupts(enable);
		return;
	}

	_shutdown = 1;
	_recFlag  = (exec) ? 1 : 0;
	DEBUGPrint("Wait for %d ms until start reconnect!\n", _recCnt);
	OSRestoreInterrupts(enable);
}

BOOL WPADCancelSyncDevice(void)
{
	return WUDCancelSyncDevice();
}

BOOL WPADStartFastSimpleSync(void)
{
	return WUDStartFastSyncSimple();
}

BOOL WPADStopSimpleSync(void)
{
	return WUDStopSyncSimple();
}

// not in the dol; body as in pik1wii
WPADSyncDeviceCallback WPADSetSyncDeviceCallback(WPADSyncDeviceCallback callback)
{
	return WUDSetSyncDeviceCallback(callback);
}

WPADSimpleSyncCallback WPADSetSimpleSyncCallback(WPADSimpleSyncCallback callback)
{
	return WUDSetSyncSimpleCallback(callback);
}

// not in the dol; body as in pik1wii
WPADClearDeviceCallback WPADSetClearDeviceCallback(WPADClearDeviceCallback callback)
{
	return WUDSetClearDeviceCallback(callback);
}

void WPADRegisterAllocator(WPADAlloc alloc, WPADFree free)
{
	WUDRegisterAllocator(alloc, free);
}

// not in the dol; body as in pik1wii
u32 WPADGetWorkMemorySize(void)
{
	return WUDGetAllocatedMemSize();
}

s32 WPADGetStatus(void)
{
	return WUDGetStatus();
}

u8 WPADGetRadioSensitivity(s32 chan)
{
	WPADControlBlock* p_wpd = _wpdcb[chan];
	BOOL enable;
	u8 sensitivity;

	enable      = OSDisableInterrupts();
	sensitivity = p_wpd->radioSense;
	OSRestoreInterrupts(enable);

	return sensitivity;
}

void WPADGetAddress(s32 chan, u8* addr)
{
	WPADControlBlock* p_wpd = _wpdcb[chan];
	BOOL enable;
	s8 handle;
	u8* devAddr;

	enable = OSDisableInterrupts();
	handle = p_wpd->dev_handle;
	OSRestoreInterrupts(enable);

	devAddr = _WUDGetDevAddr((u8)handle);

	if (devAddr) {
		memcpy(addr, devAddr, 6);
	} else {
		memset(addr, 0, 6);
	}
}

u8 WPADGetSensorBarPosition(void)
{
	BOOL enable;
	u8 pos;

	enable = OSDisableInterrupts();
	pos    = _sensorBarPos;
	OSRestoreInterrupts(enable);
	return pos;
}

static void __wpadSetupConnectionCallback(s32 chan, s32 result)
{
	WPADControlBlock* p_wpd = _wpdcb[chan];

	if (result == WPAD_ERR_NONE) {
		p_wpd->setup = TRUE;

		if (p_wpd->connectCallback) {
			p_wpd->connectCallback(chan, result);
		}
	} else {
		WPADiDisconnect(chan, FALSE);
	}
}

static void __wpadAbortConnectionCallback(s32 chan, s32 result)
{
	WPADControlBlock* p_wpd = _wpdcb[chan];

	if (result != WPAD_ERR_NONE) {
		WPADiClearQueue(&p_wpd->cmdq);
		WPADiDisconnect(chan, FALSE);
	}
}

static void __wpadInitConnectionCallback(s32 chan, s32 result)
{
	WPADControlBlock* p_wpd = _wpdcb[chan];
	u16 size;
	u32 addr;
	u8 port;
	BOOL enable;

	if (result == WPAD_ERR_NO_CONTROLLER) {
		return;
	}

	enable        = OSDisableInterrupts();
	p_wpd->oldFw  = (result == WPAD_ERR_NONE) ? TRUE : FALSE;
	p_wpd->status = WPAD_ERR_NONE;
	OSRestoreInterrupts(enable);
	size = (u16)((result == WPAD_ERR_NONE) ? 20 : 42);
	addr = (u32)((result == WPAD_ERR_NONE) ? 0x176c : 0);

	DEBUGPrint(" ==>this error means that the firmware is for NDEV %s\n", (p_wpd->oldFw) ? "2.0" : "2.1 or later");
	port = (u8)((p_wpd->devType == WPAD_DEV_BALANCE_CHECKER) ? 0x01 : 0x01 << chan);

	WPADiSendSetReportType(&p_wpd->cmdq, WPAD_FMT_CORE, p_wpd->pwrSave, __wpadAbortConnectionCallback);
	WPADiSendDPDCSB(&p_wpd->cmdq, FALSE, __wpadAbortConnectionCallback);
	WPADiSendSetPort(&p_wpd->cmdq, port, __wpadAbortConnectionCallback);
	WPADiSendReadData(&p_wpd->cmdq, p_wpd->readBuf, sizeof(WPADMEMGameInfo), 0x2A, __wpadAbortConnectionCallback);
	WPADiSendReadData(&p_wpd->cmdq, p_wpd->readBuf, sizeof(WPADMEMGameInfo), 0x2A + sizeof(WPADMEMGameInfo), __wpadAbortConnectionCallback);
	WPADiSendReadData(&p_wpd->cmdq, p_wpd->readBuf, size, addr, __wpadSetupConnectionCallback);
	WPADiSendGetContStat(&p_wpd->cmdq, NULL, NULL);
}

static s32 __wpadRetrieveChannel(WUDDevInfo* info)
{
	u8* devAddr;
	int i;
	s32 chan = -1;

	devAddr = _WUDGetDevAddr(info->devHandle);

	if (!memcmp(info, "Nintendo RVL-WBC", 16)) {
		chan = WPAD_CHAN3;
		if (_wpdcb[chan]->used) {
			btm_remove_acl(devAddr);
			return -1;
		}
	} else {
		for (i = 0; i < WPAD_MAX_CONTROLLERS; i++) {
			if (WUDIsLatestDevice(i, devAddr) && _chan_active_state[i] == 0) {
				chan = i;
				break;
			}
			if (_chan_active_state[i] == 0 && chan < 0) {
				chan = i;
			}
		}
	}

	if (!WUDIsLatestDevice(chan, devAddr)) {
		WUDSetDeviceHistory(chan, devAddr);
	}

	_chan_active_state[chan] = 1;
	return chan;
}

static void __wpadConnectionCallback(WUDDevInfo* info, u8 open)
{
	u8 dev_handle = info->devHandle;
	s32 chan;
	WPADControlBlock* p_wpd;
	WPADCommand cmd;

	DEBUGPrint("connection is %s\n", (open) ? "opened" : "closed");

	if (open) {
		chan = __wpadRetrieveChannel(info);
		if (chan < 0) {
			return;
		}
		_dev_handle_index[dev_handle] = (s8)chan;
		__wpadClearControlBlock(chan);
		p_wpd = _wpdcb[chan];

		if (!memcmp(info, "Nintendo RVL-CNT", 16)) {
			p_wpd->devType    = WPAD_DEV_CORE;
			p_wpd->dataFormat = WPAD_FMT_CORE;
		} else if (!memcmp(info, "Nintendo RVL-WBC", 16) && WUDIsLinkedWBC()) {
			p_wpd->devType    = WPAD_DEV_BALANCE_CHECKER;
			p_wpd->dataFormat = WPAD_FMT_BALANCE_CHECKER;
		} else {
			p_wpd->devType    = WPAD_DEV_FUTURE;
			p_wpd->dataFormat = WPAD_FMT_CORE;
		}
		p_wpd->dev_handle        = (s8)dev_handle;
		p_wpd->used              = TRUE;
		p_wpd->status            = WPAD_ERR_NONE;
		p_wpd->radioSense        = 100;
		p_wpd->disconnect        = 0;
		p_wpd->extEnc            = 0;
		p_wpd->lastExtensionType = p_wpd->devType;
		WPADiSendReadData(&p_wpd->cmdq, p_wpd->readBuf, 1, 0x1770, __wpadInitConnectionCallback);
		__SetScreenSaverFlag(TRUE);
	} else {
		chan                          = _dev_handle_index[dev_handle];
		_dev_handle_index[dev_handle] = -1;

		if (chan != -1) {
			p_wpd         = _wpdcb[chan];
			p_wpd->status = WPAD_ERR_NO_CONTROLLER;

			if (p_wpd->resultCallback) {
				p_wpd->resultCallback(chan, WPAD_ERR_NO_CONTROLLER);
			} else if (p_wpd->mem.callback) {
				p_wpd->mem.callback(chan, WPAD_ERR_NO_CONTROLLER);
			}
			while (WPADiGetCommand(&p_wpd->cmdq, &cmd)) {
				if (cmd.callback) {
					cmd.callback(chan, WPAD_ERR_NO_CONTROLLER);
				}
				WPADiPopCommand(&p_wpd->cmdq);
			}
			DEBUGPrint("clean up command queue\n");

			if (p_wpd->samplingBufs_ptr) {
				WPADSetAutoSamplingBuf(chan, p_wpd->samplingBufs_ptr, p_wpd->bufLength);
			}

			__wpadClearControlBlock(chan);
			_chan_active_state[chan] = 0;

			if (p_wpd->connectCallback) {
				p_wpd->connectCallback(chan, WPAD_ERR_NO_CONTROLLER);
			}
		} else {
			DEBUGPrint("WARNING: disconnection for device handle not assigned to channel.\n");
		}
	}
}

static void __wpadReceiveCallback(u8 dev_handle, u8* p_rpt, u16)
{
	u8 chan;
	s32 err;

	chan = (u8)_dev_handle_index[dev_handle];
	err  = WPADiHIDParser(chan, p_rpt);
	if (err) {
		DEBUGPrint("HID Parser reports: %d\n", err);
	}
}

static s32 WPADiGetStatus(s32 chan)
{
	WPADControlBlock* p_wpd = _wpdcb[chan];
	BOOL enable;
	s32 status;

	enable = OSDisableInterrupts();
	status = p_wpd->status;
	OSRestoreInterrupts(enable);

	return status;
}

void WPADGetAccGravityUnit(s32 chan, u32 type, WPADAcc* acc)
{
	WPADControlBlock* p_wpd = _wpdcb[chan];
	BOOL enable;

	enable = OSDisableInterrupts();
	if (acc) {
		switch (type) {
		case WPAD_DEV_CORE:
			acc->x = (s16)(p_wpd->conf.acc_1g.x - p_wpd->conf.acc_0g.x);
			acc->y = (s16)(p_wpd->conf.acc_1g.y - p_wpd->conf.acc_0g.y);
			acc->z = (s16)(p_wpd->conf.acc_1g.z - p_wpd->conf.acc_0g.z);
			break;

		case WPAD_DEV_FREESTYLE:
			acc->x = (s16)(p_wpd->extConf.fs.acc_1g.x - p_wpd->extConf.fs.acc_0g.x);
			acc->y = (s16)(p_wpd->extConf.fs.acc_1g.y - p_wpd->extConf.fs.acc_0g.y);
			acc->z = (s16)(p_wpd->extConf.fs.acc_1g.z - p_wpd->extConf.fs.acc_0g.z);
			break;
		}
	}
	OSRestoreInterrupts(enable);
}

static void __wpadDisconnectCallback(s32 chan, s32 result)
{
	WPADControlBlock* p_wpd = _wpdcb[chan];

	if (result != WPAD_ERR_NO_CONTROLLER) {
		BTA_HhClose((u8)p_wpd->dev_handle);
	}
}

static void WPADiDisconnect(s32 chan, BOOL polite)
{
	WPADControlBlock* p_wpd = _wpdcb[chan];
	BOOL enable;
	s32 status;

	status = WPADiGetStatus(chan);

	if (status != WPAD_ERR_NO_CONTROLLER) {
		if (polite) {
			enable = OSDisableInterrupts();
			if (p_wpd->disconnect) {
				OSRestoreInterrupts(enable);
				return;
			}
			p_wpd->disconnect = 1;
			OSRestoreInterrupts(enable);
			WPADControlLed(chan, 0, __wpadDisconnectCallback);
		} else {
			u8 addr[6];
			WPADGetAddress(chan, addr);
			btm_remove_acl(addr);
		}
	}
}

void WPADDisconnect(s32 chan)
{
	if (WPADiGetStatus(chan) != WPAD_ERR_NO_CONTROLLER) {
		WUDSetDeviceHistory(chan, NULL);
		WPADiDisconnect(chan, TRUE);
	}
}

// not in the dol; body as in pik1wii
void WPADSetAutoSleepTime(u8 minute)
{
	BOOL enable = OSDisableInterrupts();
	_sleepTime  = minute;
	OSRestoreInterrupts(enable);
}

s32 WPADProbe(s32 chan, u32* type)
{
	WPADControlBlock* p_wpd = _wpdcb[chan];
	s32 status;
	BOOL enable = OSDisableInterrupts();
	if (type != NULL) {
		*type = p_wpd->devType;
	}
	status = p_wpd->status;
	if (status != WPAD_ERR_NO_CONTROLLER) {
		if (p_wpd->devType == WPAD_DEV_NOT_FOUND) {
			status = WPAD_ERR_NO_CONTROLLER;
		} else if (!p_wpd->setup) {
			status = WPAD_ERR_BUSY;
		}
	}
	OSRestoreInterrupts(enable);
	return status;
}

WPADSamplingCallback WPADSetSamplingCallback(s32 chan, WPADSamplingCallback callback)
{
	WPADControlBlock* p_wpd = _wpdcb[chan];
	WPADSamplingCallback prev;
	BOOL enable;

	if (_usedCallback) {
		OSReport("WARNING: Overwritten the callback needed by KPAD.\n");
		OSReport("         Please call KPADSetSamplingCallback instead of WPADSetSamplingCallback.\n");
	}

	enable                = OSDisableInterrupts();
	prev                  = p_wpd->sampleCallback;
	p_wpd->sampleCallback = callback;
	OSRestoreInterrupts(enable);
	return prev;
}

WPADConnectCallback WPADSetConnectCallback(s32 chan, WPADConnectCallback callback)
{
	WPADControlBlock* p_wpd = _wpdcb[chan];
	WPADConnectCallback prev;
	BOOL enable;

	if (_usedCallback) {
		OSReport("WARNING: Overwritten the callback needed by KPAD.\n");
		OSReport("         Please call KPADSetConnectCallback instead of WPADSetConnectCallback.\n");
	}

	enable                 = OSDisableInterrupts();
	prev                   = p_wpd->connectCallback;
	p_wpd->connectCallback = callback;
	OSRestoreInterrupts(enable);
	return prev;
}

WPADExtensionCallback WPADSetExtensionCallback(s32 chan, WPADExtensionCallback callback)
{
	WPADControlBlock* p_wpd = _wpdcb[chan];
	WPADExtensionCallback prev;
	BOOL enable;

	enable                   = OSDisableInterrupts();
	prev                     = p_wpd->extensionCallback;
	p_wpd->extensionCallback = callback;
	OSRestoreInterrupts(enable);

	return prev;
}

u32 WPADGetDataFormat(s32 chan)
{
	WPADControlBlock* p_wpd = _wpdcb[chan];
	u32 fmt;
	BOOL enable = OSDisableInterrupts();
	fmt         = p_wpd->dataFormat;
	OSRestoreInterrupts(enable);
	return fmt;
}

s32 WPADSetDataFormat(s32 chan, u32 fmt)
{
	WPADControlBlock* p_wpd = _wpdcb[chan];
	s32 ret;
	BOOL enable;
	BOOL setup;
	u32 prevFmt;

	enable  = OSDisableInterrupts();
	setup   = p_wpd->setup;
	ret     = p_wpd->status;
	prevFmt = p_wpd->dataFormat;
	OSRestoreInterrupts(enable);

	if (ret == WPAD_ERR_NO_CONTROLLER) {
		return WPAD_ERR_NO_CONTROLLER;
	}
	if (!setup) {
		return WPAD_ERR_BUSY;
	}
	if (prevFmt == fmt) {
		return WPAD_ERR_NONE;
	}
	if (WPADiSendSetReportType(&p_wpd->cmdq, fmt, p_wpd->pwrSave, NULL)) {
		enable            = OSDisableInterrupts();
		p_wpd->dataFormat = fmt;
		OSRestoreInterrupts(enable);
		return WPAD_ERR_NONE;
	}
	return WPAD_ERR_BUSY;
}

void __wpadInfoCallback(s32 chan, s32 result)
{
	WPADControlBlock* p_wpd = _wpdcb[chan];

	if (p_wpd->infoCallback) {
		p_wpd->infoCallback(chan, result);
	}
	p_wpd->infoCallback = NULL;
	p_wpd->infoLocked   = 0;
}

s32 WPADGetInfoAsync(s32 chan, WPADInfo* info, WPADCallback callback)
{
	WPADControlBlock* p_wpd = _wpdcb[chan];
	BOOL busy;
	BOOL enable;
	BOOL setup;
	s32 result;
	u8 lock;

	enable = OSDisableInterrupts();
	setup  = p_wpd->setup;
	result = p_wpd->status;
	lock   = p_wpd->infoLocked;
	OSRestoreInterrupts(enable);

	if (result != WPAD_ERR_NO_CONTROLLER) {
		if (!setup || lock) {
			result = WPAD_ERR_BUSY;
		} else {
			enable              = OSDisableInterrupts();
			p_wpd->infoLocked   = 1;
			p_wpd->infoCallback = callback;
			OSRestoreInterrupts(enable);
			busy = WPADiSendGetContStat(&p_wpd->cmdq, info, __wpadInfoCallback);

			if (busy) {
				result = WPAD_ERR_NONE;
			} else {
				result              = WPAD_ERR_BUSY;
				enable              = OSDisableInterrupts();
				p_wpd->infoLocked   = 0;
				p_wpd->infoCallback = NULL;
				OSRestoreInterrupts(enable);
			}
		}
	}

	if (result != WPAD_ERR_NONE) {
		if (callback) {
			callback(chan, result);
		}
	}

	return result;
}

void WPADControlMotor(s32 chan, u32 command)
{
	WPADControlBlock* p_wpd = _wpdcb[chan];
	BOOL enable;
	BOOL needed = FALSE;
	s32 status;

	enable = OSDisableInterrupts();
	status = p_wpd->status;

	if (status == WPAD_ERR_NO_CONTROLLER) {
		OSRestoreInterrupts(enable);
		return;
	}

	if (_rumble == FALSE) {
		if (command != WPAD_MOTOR_STOP || p_wpd->motor != TRUE) {
			OSRestoreInterrupts(enable);
			return;
		}
	}

	if ((command == WPAD_MOTOR_STOP && p_wpd->motor == FALSE) || (command == WPAD_MOTOR_RUMBLE && p_wpd->motor == TRUE)) {
		OSRestoreInterrupts(enable);
		return;
	}

	p_wpd->motor  = (command == WPAD_MOTOR_STOP) ? FALSE : TRUE;
	p_wpd->rumble = TRUE;
	OSRestoreInterrupts(enable);
}

void WPADEnableMotor(BOOL enable)
{
	BOOL intr = OSDisableInterrupts();
	_rumble   = enable;
	OSRestoreInterrupts(intr);
}

BOOL WPADIsMotorEnabled()
{
	BOOL result;
	BOOL enable = OSDisableInterrupts();
	result      = _rumble;
	OSRestoreInterrupts(enable);
	return result;
}

s32 WPADControlLed(s32 chan, u8 pattern, WPADCallback callback)
{
	WPADControlBlock* p_wpd = _wpdcb[chan];
	BOOL busy;
	BOOL enable;
	BOOL setup;
	s32 result = WPAD_ERR_NONE;

	enable = OSDisableInterrupts();
	result = p_wpd->status;
	setup  = p_wpd->setup;
	OSRestoreInterrupts(enable);

	if (result != WPAD_ERR_NO_CONTROLLER) {
		if (!setup) {
			result = WPAD_ERR_BUSY;
		} else {
			busy   = WPADiSendSetPort(&p_wpd->cmdq, pattern, callback);
			result = (busy) ? WPAD_ERR_NONE : WPAD_ERR_BUSY;
		}
	}

	if (result != WPAD_ERR_NONE) {
		if (callback) {
			callback(chan, result);
		}
	}
	return result;
}

BOOL WPADSaveConfig(WPADFlushCallback callback)
{
	BOOL result = TRUE;
	BOOL enable;
	u8 volume;
	u8 mode;

	if (SCCheckStatus() != 0) {
		return FALSE;
	}

	enable = OSDisableInterrupts();
	volume = _speakerVolume;
	mode   = (u8)((_rumble) ? 1 : 0);
	OSRestoreInterrupts(enable);

	result &= SCSetWpadSpeakerVolume(volume);
	result &= SCSetWpadMotorMode(mode);

	if (result) {
		SCFlushAsync(callback);
	} else if (callback) {
		callback(2);
	}
	return result;
}

void WPADRead(s32 chan, void* status)
{
	WPADControlBlock* p_wpd = _wpdcb[chan];
	void* p_data;
	BOOL enable;
	u8 index;
	u32 size;

	enable = OSDisableInterrupts();

	index  = (u8)((p_wpd->rxBufIndex) ? 0 : 1);
	p_data = (void*)(p_wpd->rxBuf + index * WPAD_RX_DATASIZE);
	size   = __wpadFmt2Size(p_wpd->dataFormat);

	switch (((WPADStatus*)p_data)->err) {
	case WPAD_ERR_NONE:
		memcpy(status, p_data, size);
		break;

	case WPAD_ERR_BUSY:
	case WPAD_ERR_CORRUPTED:
		memcpy(status, p_data, sizeof(WPADStatus));
		break;
	default:
		memset(status, 0, size);
		break;
	}

	((WPADStatus*)status)->err = ((WPADStatus*)p_data)->err;
	OSRestoreInterrupts(enable);
}

void WPADSetAutoSamplingBuf(s32 chan, void* buf, u32 length)
{
	WPADControlBlock* p_wpd = _wpdcb[chan];
	BOOL enable;
	s8 err;
	u32 size;
	int i;

	enable = OSDisableInterrupts();
	err    = (s8)((p_wpd->status == WPAD_ERR_NO_CONTROLLER) ? WPAD_ERR_NO_CONTROLLER : WPAD_ERR_INVALID);
	size   = __wpadFmt2Size(p_wpd->dataFormat);

	if (buf) {
		memset(buf, 0, size * length);
		for (i = 0; i < length; ++i) {
			((WPADStatus*)((u8*)buf + i * size))->err = err;
		}
		p_wpd->bufLatestIndex = 0xFFFFFFFF;
		p_wpd->bufLength      = length;
	}
	p_wpd->samplingBufs_ptr = buf;
	OSRestoreInterrupts(enable);
}

void WPADiExcludeButton(s32 chan)
{
	WPADControlBlock* p_wpd = _wpdcb[chan];
	BOOL enable;
	void* p_data;
	u8 index;
	WPADStatus* p_wp;
	WPADCLStatus* p_cp;
	WPADTRStatus* p_tp;

	enable = OSDisableInterrupts();
	index  = (u8)((p_wpd->rxBufIndex) ? 0 : 1);
	p_data = (void*)(p_wpd->rxBuf + index * WPAD_RX_DATASIZE);

	p_wp = (WPADStatus*)p_data;
	if ((p_wp->button & WPAD_BTN_HCURSOR_MASK) == WPAD_BTN_HCURSOR_MASK) {
		p_wp->button &= ~WPAD_BUTTON_RIGHT;
	}
	if ((p_wp->button & WPAD_BTN_VCURSOR_MASK) == WPAD_BTN_VCURSOR_MASK) {
		p_wp->button &= ~WPAD_BUTTON_DOWN;
	}

	if (p_wpd->dataFormat == WPAD_FMT_CLASSIC || p_wpd->dataFormat == WPAD_FMT_CLASSIC_ACC || p_wpd->dataFormat == WPAD_FMT_CLASSIC_ACC_DPD
	    || p_wpd->dataFormat == WPAD_FMT_GUITAR || p_wpd->dataFormat == WPAD_FMT_DRUM) {
		p_cp = (WPADCLStatus*)p_data;
		if ((p_cp->clButton & WPAD_CLB_HCURSOR_MASK) == WPAD_CLB_HCURSOR_MASK) {
			p_cp->clButton &= ~WPAD_CL_BUTTON_RIGHT;
		}
		if ((p_cp->clButton & WPAD_CLB_VCURSOR_MASK) == WPAD_CLB_VCURSOR_MASK) {
			p_cp->clButton &= ~WPAD_CL_BUTTON_DOWN;
		}
	}

	if (p_wpd->dataFormat == WPAD_FMT_TRAIN) {
		p_tp = (WPADTRStatus*)p_data;
		if ((p_tp->trButton & WPAD_CLB_HCURSOR_MASK) == WPAD_CLB_HCURSOR_MASK) {
			p_tp->trButton &= ~WPAD_CL_BUTTON_RIGHT;
		}
		if ((p_tp->trButton & WPAD_CLB_VCURSOR_MASK) == WPAD_CLB_VCURSOR_MASK) {
			p_tp->trButton &= ~WPAD_CL_BUTTON_DOWN;
		}
	}

	OSRestoreInterrupts(enable);
}

void WPADiCopyOut(s32 chan)
{
	WPADControlBlock* p_wpd = _wpdcb[chan];
	BOOL enable;
	void* p_data;
	void* p_stat;
	u8 index;
	u32 size;

	enable = OSDisableInterrupts();
	index  = (u8)((p_wpd->rxBufIndex) ? 0 : 1);
	p_data = (void*)(p_wpd->rxBuf + index * WPAD_RX_DATASIZE);
	size   = __wpadFmt2Size(p_wpd->dataFormat);

	if (p_wpd->samplingBufs_ptr) {
		p_wpd->bufLatestIndex++;
		if (p_wpd->bufLatestIndex >= p_wpd->bufLength) {
			p_wpd->bufLatestIndex = 0;
		}

		p_stat = (u8*)p_wpd->samplingBufs_ptr + p_wpd->bufLatestIndex * size;
		if (((WPADStatus*)p_data)->err != WPAD_ERR_NONE) {
			size = sizeof(WPADStatus);
		}
		memcpy(p_stat, p_data, size);
	}
	if (p_wpd->sampleCallback) {
		p_wpd->sampleCallback(chan);
	}

	p_wpd->packetCnt++;
	OSRestoreInterrupts(enable);
}

BOOL WPADIsSpeakerEnabled(s32 chan)
{
	BOOL state;
	BOOL enable;
	WPADControlBlock* p_wpd = _wpdcb[chan];
	enable                  = OSDisableInterrupts();
	state                   = p_wpd->infos.speaker;
	OSRestoreInterrupts(enable);
	return state;
}

BOOL WPADiSendMuteSpeaker(WPADCmdQueue* queue, BOOL enable, WPADCallback callback)
{
	WPADCommand cmd;
	BOOL result;
	cmd.command  = WPAD_HIDREP_MUTE;
	cmd.len      = 1;
	cmd.data[0]  = (u8)((enable) ? (u8)4 : (u8)0);
	cmd.callback = callback;
	result       = WPADiPushCommand(queue, cmd);
	return result;
}

BOOL WPADiSendEnableSpeaker(WPADCmdQueue* queue, BOOL enable, WPADCallback callback)
{
	WPADCommand cmd;
	BOOL result;
	cmd.command  = WPAD_HIDREP_ENSPK;
	cmd.len      = 1;
	cmd.data[0]  = (u8)((enable) ? (u8)4 : (u8)0);
	cmd.callback = callback;
	result       = WPADiPushCommand(queue, cmd);
	return result;
}

s32 WPADControlSpeaker(s32 chan, u32 command, WPADCallback callback)
{
	BOOL enable;
	BOOL spk;
	BOOL setup;
	s32 result;
	u8 initCmd[]            = { 0x00, 0x00, 0xd0, 0x07, 0x40, 0x0c, 0x0e };
	WPADControlBlock* p_wpd = _wpdcb[chan];

	enable = OSDisableInterrupts();
	spk    = p_wpd->infos.speaker;
	result = p_wpd->status;
	setup  = p_wpd->setup;
	OSRestoreInterrupts(enable);

	if (result != WPAD_ERR_NO_CONTROLLER) {
		if (!setup) {
			result = WPAD_ERR_BUSY;
		} else if (command == 0) {
			if (!spk) {
				result = WPAD_ERR_NONE;
			} else {
				enable = OSDisableInterrupts();
				if (__CanPushCmdQueue(&p_wpd->cmdq, 5)) {
					WPADiSendMuteSpeaker(&p_wpd->cmdq, TRUE, NULL);
					WPADiSendWriteDataCmd(&p_wpd->cmdq, SPK_DERESET, SPK_RST_REG, NULL);
					WPADiSendWriteDataCmd(&p_wpd->cmdq, SPK_CLK_OFF, SPK_CLK_REG, NULL);
					WPADiSendEnableSpeaker(&p_wpd->cmdq, FALSE, NULL);
					WPADiSendGetContStat(&p_wpd->cmdq, NULL, callback);
					OSRestoreInterrupts(enable);
					return WPAD_ERR_NONE;
				} else {
					result = WPAD_ERR_BUSY;
				}
				OSRestoreInterrupts(enable);
			}
		} else {
			switch (command) {
			case 1:
			case 5:
				enable = OSDisableInterrupts();
				if (__CanPushCmdQueue(&p_wpd->cmdq, 7)) {
					WPADiSendEnableSpeaker(&p_wpd->cmdq, TRUE, NULL);
					WPADiSendMuteSpeaker(&p_wpd->cmdq, TRUE, NULL);
					WPADiSendWriteDataCmd(&p_wpd->cmdq, SPK_CLK_ON, SPK_CLK_REG, NULL);
					WPADiSendWriteDataCmd(&p_wpd->cmdq, SPK_RESET, SPK_RST_REG, NULL);
					initCmd[4] = _speakerVolume;
					WPADiSendWriteData(&p_wpd->cmdq, initCmd, sizeof(initCmd), SPK_RST_REG, NULL);
					WPADiSendMuteSpeaker(&p_wpd->cmdq, FALSE, NULL);
					WPADiSendGetContStat(&p_wpd->cmdq, NULL, callback);
					OSRestoreInterrupts(enable);
					return WPAD_ERR_NONE;
				} else {
					result = WPAD_ERR_BUSY;
				}
				OSRestoreInterrupts(enable);
				break;

			case 2:
				if (!WPADiSendMuteSpeaker(&p_wpd->cmdq, TRUE, callback)) {
					result = WPAD_ERR_BUSY;
				} else {
					return WPAD_ERR_NONE;
				}
				break;

			case 3:
				if (!WPADiSendMuteSpeaker(&p_wpd->cmdq, FALSE, callback)) {
					result = WPAD_ERR_BUSY;
				} else {
					return WPAD_ERR_NONE;
				}
				break;

			case 4:
				if (!WPADiSendWriteDataCmd(&p_wpd->cmdq, SPK_CTRL_PLAY, SPK_CTRL_REG, callback)) {
					result = WPAD_ERR_BUSY;
				} else {
					return WPAD_ERR_NONE;
				}
				break;
			}
		}
	}

	if (callback) {
		callback(chan, result);
	}

	return result;
}

u8 WPADGetSpeakerVolume()
{
	BOOL enable;
	u8 vol;

	enable = OSDisableInterrupts();
	vol    = _speakerVolume;
	OSRestoreInterrupts(enable);

	return vol;
}

void WPADSetSpeakerVolume(u8 volume)
{
	BOOL enable;

	enable         = OSDisableInterrupts();
	_speakerVolume = __ClampSpeakerVolume(volume);
	OSRestoreInterrupts(enable);
}

BOOL __wpadIsBusyStream(s32 chan)
{
	BOOL enable;
	WPADControlBlock* p_wpd = _wpdcb[chan];
	u8 stop;
	u32 devType;
	u8 bufNum;
	u16 qSize;
	u16 notAck;
	u8 frames;
	s8 cmdNum;
	u8 linkNum;

	enable  = OSDisableInterrupts();
	stop    = p_wpd->audioStop;
	devType = p_wpd->devType;
	bufNum  = WUDGetBufferStatus();
	cmdNum  = __GetCmdNumber(&p_wpd->cmdq);
	qSize   = _WPADGetStackBufferStatus(chan);
	notAck  = _WPADGetModuleBufferStatus(chan);
	frames  = p_wpd->audioFrames;
	linkNum = _WUDGetLinkNumber();
	OSRestoreInterrupts(enable);

	if ((stop) || (notAck > 3) || (bufNum == 10) || (bufNum >= linkNum * 2 + 2) || (devType == WPAD_DEV_UNKNOWN)
	    || (cmdNum >= WPAD_COMMAND_CMD_MAX_LEN - 3) || (frames >= 1)) {
		return TRUE;
	} else {
		return FALSE;
	}
}

BOOL WPADCanSendStreamData(s32 chan)
{
	BOOL enable;
	BOOL setup;
	s32 status;
	WPADControlBlock* p_wpd = _wpdcb[chan];

	enable = OSDisableInterrupts();
	status = p_wpd->status;
	setup  = p_wpd->setup;
	OSRestoreInterrupts(enable);

	if ((status == WPAD_ERR_NO_CONTROLLER) || (setup == FALSE) || (__wpadIsBusyStream(chan))) {
		return FALSE;
	} else {
		return TRUE;
	}
}

BOOL WPADiSendStreamData(WPADCmdQueue* queue, void* p_buf, u16 len)
{
	WPADCommand cmd;
	u8 length = (u8)(len << 3);

	cmd.command  = WPAD_HIDREP_STRM;
	cmd.len      = 21;
	cmd.data[0]  = length;
	cmd.callback = NULL;
	memcpy(cmd.data + 1, p_buf, len);
	return WPADiPushCommand(queue, cmd);
}

s32 WPADSendStreamData(s32 chan, void* p_buf, u16 len)
{
	BOOL busy;
	BOOL enable;
	BOOL setup;
	s32 status;
	WPADControlBlock* p_wpd = _wpdcb[chan];

	enable = OSDisableInterrupts();
	status = p_wpd->status;
	setup  = p_wpd->setup;
	OSRestoreInterrupts(enable);

	if (status == WPAD_ERR_NO_CONTROLLER) {
		return WPAD_ERR_NO_CONTROLLER;
	}

	if (!setup) {
		return WPAD_ERR_BUSY;
	}

	if (__wpadIsBusyStream(chan)) {
		return WPAD_ERR_BUSY;
	}

	busy = WPADiSendStreamData(&p_wpd->cmdq, p_buf, len);
	if (!busy) {
		return WPAD_ERR_BUSY;
	}
	enable = OSDisableInterrupts();
	p_wpd->audioFrames++;
	OSRestoreInterrupts(enable);
	return WPAD_ERR_NONE;
}

u8 WPADGetDpdSensitivity(void)
{
	BOOL enable;
	u8 sensitivity;

	enable      = OSDisableInterrupts();
	sensitivity = _dpdSensitivity;
	OSRestoreInterrupts(enable);
	return sensitivity;
}

BOOL WPADIsDpdEnabled(s32 chan)
{
	BOOL state;
	BOOL enable;
	WPADControlBlock* p_wpd = _wpdcb[chan];

	enable = OSDisableInterrupts();
	state  = p_wpd->infos.dpd;
	OSRestoreInterrupts(enable);
	return state;
}

u8 WPADGetDpdFormat(s32 chan)
{
	WPADControlBlock* p_wpd = _wpdcb[chan];
	BOOL enable;
	u8 format;

	enable = OSDisableInterrupts();
	format = (u8)((p_wpd->infos.dpd) ? p_wpd->dpdFormat : 0);
	OSRestoreInterrupts(enable);
	return format;
}

static void __wpadDpdCallback(s32 chan, s32 result)
{
	WPADControlBlock* p_wpd  = _wpdcb[chan];
	p_wpd->dpdFormat         = p_wpd->dpdCommand;
	p_wpd->dpdCommandPending = 0;
	p_wpd->infos.dpd         = (p_wpd->dpdCommand == 0) ? FALSE : TRUE;
}

BOOL WPADiSendEnableDPD(WPADCmdQueue* queue, BOOL enable, WPADCallback callback)
{
	WPADCommand cmd;
	BOOL result;

	cmd.command  = WPAD_HIDREP_ENDPD;
	cmd.len      = 1;
	cmd.data[0]  = (u8)((enable) ? (u8)4 : (u8)0);
	cmd.callback = callback;

	return WPADiPushCommand(queue, cmd);
}

s32 WPADControlDpd(s32 chan, u32 command, WPADCallback callback)
{
	BOOL enable;
	BOOL dpd;
	BOOL setup;
	u8 format;
	u8 lastCmd;
	s32 result;
	WPADControlBlock* p_wpd = _wpdcb[chan];

	static const u8 cfg1[5][9] = {
		0x02, 0x00, 0x00, 0x71, 0x01, 0x00, 0x64, 0x00, 0xfe, 0x02, 0x00, 0x00, 0x71, 0x01, 0x00,
		0x96, 0x00, 0xb4, 0x02, 0x00, 0x00, 0x71, 0x01, 0x00, 0xaa, 0x00, 0x64, 0x02, 0x00, 0x00,
		0x71, 0x01, 0x00, 0xc8, 0x00, 0x36, 0x07, 0x00, 0x00, 0x71, 0x01, 0x00, 0x72, 0x00, 0x20,
	};
	static const u8 cfg2[5][2] = {
		0xfd, 0x05, 0xb3, 0x04, 0x63, 0x03, 0x35, 0x03, 0x1f, 0x03,
	};

	enable  = OSDisableInterrupts();
	dpd     = p_wpd->infos.dpd;
	format  = p_wpd->dpdFormat;
	lastCmd = p_wpd->dpdCommand;
	result  = p_wpd->status;
	setup   = p_wpd->setup;
	OSRestoreInterrupts(enable);

	if (result != WPAD_ERR_NO_CONTROLLER) {
		if (!setup) {
			result = WPAD_ERR_BUSY;
		} else if (command == 0) {
			if (!dpd) {
				result = WPAD_ERR_NONE;
			} else {
				enable = OSDisableInterrupts();

				if (__CanPushCmdQueue(&p_wpd->cmdq, 3)) {
					p_wpd->dpdCommand = (u8)command;

					WPADiSendEnableDPD(&p_wpd->cmdq, FALSE, NULL);
					WPADiSendDPDCSB(&p_wpd->cmdq, FALSE, __wpadDpdCallback);
					WPADiSendGetContStat(&p_wpd->cmdq, NULL, callback);

					OSRestoreInterrupts(enable);
					return WPAD_ERR_NONE;
				} else {
					result = WPAD_ERR_BUSY;
				}

				OSRestoreInterrupts(enable);
			}
		} else if (command != lastCmd) {
			enable = OSDisableInterrupts();

			if (__CanPushCmdQueue(&p_wpd->cmdq, 8)) {
				p_wpd->dpdCommand        = (u8)command;
				p_wpd->dpdCommandPending = 1;

				WPADiSendEnableDPD(&p_wpd->cmdq, TRUE, NULL);
				WPADiSendDPDCSB(&p_wpd->cmdq, TRUE, NULL);
				WPADiSendWriteDataCmd(&p_wpd->cmdq, 1, 0x04b00030, NULL);
				WPADiSendWriteData(&p_wpd->cmdq, (void*)cfg1[_dpdSensitivity - 1], 9, 0x04b00000, NULL);
				WPADiSendWriteData(&p_wpd->cmdq, (void*)cfg2[_dpdSensitivity - 1], 2, 0x04b0001a, NULL);
				WPADiSendWriteDataCmd(&p_wpd->cmdq, (u8)command, 0x04b00033, NULL);
				WPADiSendWriteDataCmd(&p_wpd->cmdq, 8, 0x04b00030, __wpadDpdCallback);
				WPADiSendGetContStat(&p_wpd->cmdq, NULL, callback);

				OSRestoreInterrupts(enable);
				return WPAD_ERR_NONE;
			} else {
				result = WPAD_ERR_BUSY;
			}
			OSRestoreInterrupts(enable);
		}
	}

	if (callback) {
		callback(chan, result);
	}

	return result;
}

s32 WPADControlBLC(s32 chan, u8 command, WPADCallback callback)
{
	WPADControlBlock* p_wpd = _wpdcb[chan];
	BOOL enable;
	BOOL setup;
	s32 result;
	u8 data[7];

	enable = OSDisableInterrupts();
	result = p_wpd->status;
	setup  = p_wpd->setup;
	OSRestoreInterrupts(enable);

	if (result != WPAD_ERR_NO_CONTROLLER) {
		if (!setup || !WUDIsLinkedWBC()) {
			result = WPAD_ERR_BUSY;
		} else {
			data[0] = data[1] = data[2] = 0xaa;
			data[3]                     = 0x55;
			data[4] = data[5] = data[6] = command;

			enable = OSDisableInterrupts();
			switch (command) {
			case WPAD_BLC_ENABLE:
				if (__CanPushCmdQueue(&p_wpd->cmdq, 4)) {
					WPADWriteExtReg(chan, data, sizeof(data), 0xf1, NULL);
					WPADWriteExtReg(chan, data, 1, 0xf1, NULL);
					WPADWriteExtReg(chan, data, 1, 0xf1, NULL);
					WPADWriteExtReg(chan, data, 1, 0xf1, callback);
					OSRestoreInterrupts(enable);
					return WPAD_ERR_NONE;
				} else {
					result = WPAD_ERR_BUSY;
				}
				break;

			case WPAD_BLC_CHECK:
				result = WPADWriteExtReg(chan, data, sizeof(data), 0xf1, callback);
				if (result == WPAD_ERR_NONE) {
					OSRestoreInterrupts(enable);
					return WPAD_ERR_NONE;
				}
				break;

			case WPAD_BLC_DISABLE:
				result = WPADWriteExtReg(chan, data, 1, 0xf1, callback);
				if (result == WPAD_ERR_NONE) {
					OSRestoreInterrupts(enable);
					return WPAD_ERR_NONE;
				}
				break;

			default:
				result = WPAD_ERR_BUSY;
				break;
			}
			OSRestoreInterrupts(enable);
		}
	}

	if (callback) {
		callback(chan, result);
	}

	return result;
}

BOOL WPADiSendSetReportType(WPADCmdQueue* queue, u32 fmt, BOOL powerSave, WPADCallback callback)
{
	WPADCommand cmd;
	BOOL result;

	cmd.command  = WPAD_HIDREP_REPTYPE;
	cmd.len      = 2;
	cmd.data[0]  = (u8)((powerSave) ? 0 : 4);
	cmd.callback = callback;

	switch (fmt) {
	case WPAD_FMT_CORE:
		cmd.data[1] = 0x30;
		break;
	case WPAD_FMT_CORE_ACC:
		cmd.data[1] = 0x31;
		break;
	case WPAD_FMT_CORE_ACC_DPD:
		cmd.data[1] = 0x33;
		break;
	case WPAD_FMT_FREESTYLE:
		cmd.data[1] = 0x32;
		break;
	case WPAD_FMT_FREESTYLE_ACC:
		cmd.data[1] = 0x35;
		break;
	case WPAD_FMT_FREESTYLE_ACC_DPD:
		cmd.data[1] = 0x37;
		break;
	case WPAD_FMT_CLASSIC:
		cmd.data[1] = 0x32;
		break;
	case WPAD_FMT_CLASSIC_ACC:
		cmd.data[1] = 0x35;
		break;
	case WPAD_FMT_CLASSIC_ACC_DPD:
		cmd.data[1] = 0x37;
		break;
	case WPAD_FMT_CORE_ACC_DPD_FULL:
		cmd.data[1] = 0x3e;
		break;
	case WPAD_FMT_TRAIN:
		cmd.data[1] = 0x32;
		break;
	case WPAD_FMT_GUITAR:
		cmd.data[1] = 0x37;
		break;
	case WPAD_FMT_DRUM:
		cmd.data[1] = 0x37;
		break;
	case WPAD_FMT_BALANCE_CHECKER:
		cmd.data[1] = 0x34;
		break;
	case WPAD_FMT_VSM:
		cmd.data[1] = 0x35;
		break;
	case WPAD_FMT_VSM_EX:
		cmd.data[1] = 0x3d;
		break;
	}

	return WPADiPushCommand(queue, cmd);
}

BOOL WPADiSendGetContStat(WPADCmdQueue* queue, WPADInfo* info, WPADCallback callback)
{
	WPADCommand cmd;
	BOOL result;

	cmd.command  = WPAD_HIDREP_GETSTAT;
	cmd.len      = 1;
	cmd.data[0]  = 0;
	cmd.callback = callback;
	cmd.info     = info;

	result = WPADiPushCommand(queue, cmd);
	return result;
}

BOOL WPADiSendWriteDataCmd(WPADCmdQueue* queue, u8 cmd, u32 addr, WPADCallback callback)
{
	return WPADiSendWriteData(queue, &cmd, 1, addr, callback);
}

BOOL WPADiSendWriteData(WPADCmdQueue* queue, const void* p_buf, u16 len, u32 addr, WPADCallback callback)
{
	WPADCommand cmd;
	BOOL result;
	u8 length    = (u8)(len & WPAD_WRITE_LEN_MASK);
	cmd.command  = WPAD_HIDREP_WRDATA;
	cmd.len      = 21;
	cmd.callback = callback;
	memcpy(cmd.data, &addr, sizeof(addr));
	memcpy(cmd.data + 4, &length, sizeof(length));
	memcpy(cmd.data + 5, p_buf, len);

	result = WPADiPushCommand(queue, cmd);
	return result;
}

BOOL WPADiSendSetPort(WPADCmdQueue* queue, u8 pattern, WPADCallback callback)
{
	WPADCommand cmd;
	BOOL result;

	cmd.command  = WPAD_HIDREP_PORT;
	cmd.len      = 1;
	cmd.data[0]  = (u8)(pattern << 4);
	cmd.callback = callback;

	result = WPADiPushCommand(queue, cmd);
	return result;
}

BOOL WPADiSendDPDCSB(WPADCmdQueue* queue, BOOL enable, WPADCallback callback)
{
	WPADCommand cmd;
	BOOL result;

	cmd.command  = WPAD_HIDREP_DPDCSB;
	cmd.len      = 1;
	cmd.data[0]  = (u8)((enable) ? (u8)4 : (u8)0);
	cmd.callback = callback;

	result = WPADiPushCommand(queue, cmd);
	return result;
}

BOOL WPADiSendReadData(WPADCmdQueue* queue, void* p_buf, u16 len, u32 addr, WPADCallback callback)
{
	WPADCommand cmd;
	BOOL result;

	cmd.command  = WPAD_HIDREP_RDDATA;
	cmd.len      = 6;
	cmd.callback = callback;

	memcpy(cmd.data, &addr, sizeof(addr));
	memcpy(cmd.data + 4, &len, sizeof(len));

	cmd.readBuf  = p_buf;
	cmd.readLen  = len;
	cmd.readAddr = addr;

	result = WPADiPushCommand(queue, cmd);
	return result;
}

s8 __GetCmdNumber(WPADCmdQueue* queue)
{
	BOOL enable;
	s8 num;

	enable = OSDisableInterrupts();

	num = (s8)(queue->tail - queue->head);
	if (num < 0) {
		num += queue->cmdlen;
	}

	OSRestoreInterrupts(enable);
	return num;
}

BOOL __CanPushCmdQueue(WPADCmdQueue* queue, s8 cmd_num)
{
	s8 curr = __GetCmdNumber(queue);

	if (curr + cmd_num <= queue->cmdlen - 1) {
		return TRUE;
	} else {
		return FALSE;
	}
}

void WPADiClearQueue(WPADCmdQueue* queue)
{
	BOOL enable;

	enable = OSDisableInterrupts();

	queue->head = 0;
	queue->tail = 0;
	memset(queue->cmd, 0, queue->cmdlen * sizeof(WPADCommand));

	OSRestoreInterrupts(enable);
}

BOOL WPADiPushCommand(WPADCmdQueue* queue, WPADCommand cmd)
{
	BOOL enable;

	enable = OSDisableInterrupts();

	if (__GetCmdNumber(queue) == queue->cmdlen - 1) {
		OSRestoreInterrupts(enable);
		return FALSE;
	}

	memset(queue->cmd + queue->tail, 0, sizeof(WPADCommand));
	memcpy(queue->cmd + queue->tail, &cmd, sizeof(WPADCommand));
	queue->tail = (s8)((queue->tail == queue->cmdlen - 1) ? 0 : queue->tail + 1);
	OSRestoreInterrupts(enable);
	return TRUE;
}

BOOL WPADiGetCommand(WPADCmdQueue* queue, WPADCommand* cmd)
{
	BOOL enable;

	if (__GetCmdNumber(queue) == 0) {
		return FALSE;
	}

	enable = OSDisableInterrupts();
	memcpy(cmd, queue->cmd + queue->head, sizeof(WPADCommand));

	OSRestoreInterrupts(enable);
	return TRUE;
}

BOOL WPADiPopCommand(WPADCmdQueue* queue)
{
	BOOL enable = OSDisableInterrupts();

	if (__GetCmdNumber(queue) == 0) {
		OSRestoreInterrupts(enable);
		return FALSE;
	}

	memset(queue->cmd + queue->head, 0, sizeof(WPADCommand));
	queue->head = (s8)((queue->head == queue->cmdlen - 1) ? 0 : queue->head + 1);

	OSRestoreInterrupts(enable);
	return TRUE;
}

u16 _WPADGetStackBufferStatus(s32 chan)
{
	WPADControlBlock* p_wpd = _wpdcb[chan];
	BOOL enable;
	s32 status;
	s8 handle;

	enable = OSDisableInterrupts();
	status = p_wpd->status;
	handle = p_wpd->dev_handle;
	OSRestoreInterrupts(enable);

	if (status == WPAD_ERR_NO_CONTROLLER) {
		return 0;
	} else {
		return _WUDGetQueuedSize(handle);
	}
}

u16 _WPADGetModuleBufferStatus(s32 chan)
{
	WPADControlBlock* p_wpd = _wpdcb[chan];
	BOOL enable;
	s32 status;
	s8 handle;

	enable = OSDisableInterrupts();
	status = p_wpd->status;
	handle = p_wpd->dev_handle;
	OSRestoreInterrupts(enable);

	if (status == WPAD_ERR_NO_CONTROLLER) {
		return 0;
	} else {
		return _WUDGetNotAckedSize(handle);
	}
}

void WPADRecalibrate(s32 chan)
{
	WPADControlBlock* p_wpd = _wpdcb[chan];
	BOOL enable             = OSDisableInterrupts();
	p_wpd->isSetStickOrigin = 0;
	p_wpd->recalibrateCount = 0;
	OSRestoreInterrupts(enable);
}

void WPADSetCallbackByKPAD(BOOL flag)
{
	_usedCallback = flag;
}
