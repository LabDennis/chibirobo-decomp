/* After the CC0 projectPiki/pik1wii WPADHIDParser.c (11 Dec 2007), brought to the SDK of 11 Sep 2008. */
#include "WPADInternal.h"
#include "WPADExtern.h"
#include <math.h>
#include <string.h>

static void __a1_20_status_report(u8 chan, u8* data, void* rxBuffer);
static void __a1_21_user_data(u8 chan, u8* data, void* rxBuffer);
static void __a1_22_ack(u8 chan, u8* data, void* rxBuffer);
static void __a1_30_data_type(u8 chan, u8* data, void* rxBuffer);
static void __a1_31_data_type(u8 chan, u8* data, void* rxBuffer);
static void __a1_32_data_type(u8 chan, u8* data, void* rxBuffer);
static void __a1_33_data_type(u8 chan, u8* data, void* rxBuffer);
static void __a1_34_data_type(u8 chan, u8* data, void* rxBuffer);
static void __a1_35_data_type(u8 chan, u8* data, void* rxBuffer);
static void __a1_36_data_type(u8 chan, u8* data, void* rxBuffer);
static void __a1_37_data_type(u8 chan, u8* data, void* rxBuffer);
static void __a1_3d_data_type(u8 chan, u8* data, void* rxBuffer);
static void __a1_3e_data_type(u8 chan, u8* data, void* rxBuffer);
static void __a1_3f_data_type(u8 chan, u8* data, void* rxBuffer);
static void __a1_unused_report(u8 chan, u8* data, void* rxBuffer);

#define HID_WPAD_BUTTON_MASK  (u16)(0x9F1F)
#define HID_WPAD_BUTTON_CLEAR (u16)(0x60E0)
#define HID_NEAREMPTY_MASK    (u8)(0x80)
#define HID_DPD_MASK          (u8)(0x8)
#define HID_SPEAKER_MASK      (u8)(0x4)
#define HID_ATTACH_MASK       (u8)(0x2)
#define HID_LOWBAT_MASK       (u8)(0x1)
#define HID_LED_MASK          (u8)(0xF0)
#define HID_FW_MASK           (u8)(0xF0)
#define HID_PRT_MASK          (u8)(0xF)

#define HID_IDX_BYTE7  1
#define HID_IDX_BYTE8  2
#define HID_IDX_BYTE9  3
#define HID_IDX_BYTE10 4
#define HID_IDX_BYTE11 5
#define HID_IDX_BYTE12 6
#define HID_IDX_BYTE13 7
#define HID_IDX_BYTE14 8
#define HID_IDX_BYTE15 9
#define HID_IDX_BYTE16 10
#define HID_IDX_BYTE17 11
#define HID_IDX_BYTE18 12
#define HID_IDX_BYTE19 13
#define HID_IDX_BYTE20 14
#define HID_IDX_BYTE21 15
#define HID_IDX_BYTE22 16
#define HID_IDX_BYTE23 17
#define HID_IDX_BYTE24 18
#define HID_IDX_BYTE25 19
#define HID_IDX_BYTE26 20
#define HID_IDX_BYTE27 21

static const u8 checkInvalidData[21] = {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
};

static void (*__a1_input_reports_array[])(u8 chan, u8* data, void* rxBuffer)
    = { __a1_20_status_report, __a1_21_user_data,  __a1_22_ack,        __a1_unused_report, __a1_unused_report, __a1_unused_report,
	    __a1_unused_report,    __a1_unused_report, __a1_unused_report, __a1_unused_report, __a1_unused_report, __a1_unused_report,
	    __a1_unused_report,    __a1_unused_report, __a1_unused_report, __a1_unused_report, __a1_30_data_type,  __a1_31_data_type,
	    __a1_32_data_type,     __a1_33_data_type,  __a1_34_data_type,  __a1_35_data_type,  __a1_36_data_type,  __a1_37_data_type,
	    __a1_unused_report,    __a1_unused_report, __a1_unused_report, __a1_unused_report, __a1_unused_report, __a1_3d_data_type,
	    __a1_3e_data_type,     __a1_3f_data_type };

static f32 _wpadCalibrationX[WPAD_MAX_CONTROLLERS];
static f32 _wpadCalibrationY[WPAD_MAX_CONTROLLERS];
static f32 _wpadCenterX[WPAD_MAX_CONTROLLERS];
static f32 _wpadCenterY[WPAD_MAX_CONTROLLERS];
static f32 _wpadRolag[WPAD_MAX_CONTROLLERS];
static u8 checkBuffer[21];

static u8 _devType[WPAD_MAX_CONTROLLERS];
static u8 _devMode[WPAD_MAX_CONTROLLERS];
static u8 _devCmpt[WPAD_MAX_CONTROLLERS];
static u8 _retryCnt[WPAD_MAX_CONTROLLERS];
static u8 _recv3f[WPAD_MAX_CONTROLLERS];
static u8 _recv3e[WPAD_MAX_CONTROLLERS];

static void initExtension(s32 chan);

static void __wpadAbortInitExtension(s32 chan, s32 result)
{
	WPADControlBlock* p_wpd = _wpdcb[chan];
	u32 type;

	if (result != WPAD_ERR_NONE) {
		WPADiClearQueue(&p_wpd->extq);
		p_wpd->extEnc = 0;
		if (result == WPAD_ERR_NO_CONTROLLER) {
			type = WPAD_DEV_NOT_FOUND;
		}

		else if (p_wpd->infos.attach) {
			if (_retryCnt[chan]++ < 32) {
				initExtension(chan);
				return;
			}
			type = WPAD_DEV_NOT_SUPPORTED;
		}

		else {
			DEBUGPrint("detaching extension during initialization.\n");
			WPADiSendSetReportType(&p_wpd->extq, p_wpd->dataFormat, p_wpd->pwrSave, NULL);
			return;
		}
		p_wpd->devType                  = (u8)type;
		p_wpd->lastExtensionType        = (u8)type;
		p_wpd->keepExtensionCalibration = FALSE;
		if (p_wpd->extensionCallback) {
			p_wpd->extensionCallback(chan, (s32)type);
		}
	}
}

static void __wpadGetDevConfig(s32 chan, s32 result)
{
	WPADControlBlock* p_wpd = _wpdcb[chan];
	u8* p_buf               = (u8*)p_wpd->readBufPtr;
	u8 checkSum;
	u8 i;
	int j;
	int index;
	DPDObject obj[WPAD_DPD_MAX_OBJECTS];
	const s16 dummyObjX[] = { 127, 896, 896, 127 };
	const s16 dummyObjY[] = { 93, 93, 674, 674 };
	f32 difaveX;
	f32 difaveY;
	f32 deltaX;
	f32 deltaY;
	f32 x[WPAD_DPD_MAX_OBJECTS];
	f32 y[WPAD_DPD_MAX_OBJECTS];
	f32 ag[WPAD_DPD_MAX_OBJECTS];
	f32 ag_org[WPAD_DPD_MAX_OBJECTS];

	for (i = 0; i < WPAD_DPD_MAX_OBJECTS; i++) {
		p_wpd->conf.obj[i].x       = dummyObjX[i];
		p_wpd->conf.obj[i].y       = dummyObjY[i];
		p_wpd->conf.obj[i].size    = (u16)p_wpd->dpdDummyObjSize;
		p_wpd->conf.obj[i].traceId = i;
	}

	p_wpd->conf.acc_0g.x = 530;
	p_wpd->conf.acc_0g.y = 530;
	p_wpd->conf.acc_0g.z = 530;
	p_wpd->conf.acc_1g.x = 636;
	p_wpd->conf.acc_1g.y = 636;
	p_wpd->conf.acc_1g.z = 636;
	p_wpd->conf.volume   = 0;
	p_wpd->conf.motor    = 0;

	if (result != WPAD_ERR_NONE) {
		return;
	}

	if (!p_wpd->oldFw) {
		for (i = 0; i < 2; i++) {
			checkSum = 0;
			index    = i * (11);
			for (j = index; j < index + 10; j++) {
				checkSum += p_buf[j];
			}
			checkSum = (u8)(checkSum + 0x55);

			if (checkSum == p_buf[index + 10]) {
				DEBUGPrint("Dpd Setting is ok.\n");

				obj[0].x       = (s16)((s16)((s16)(((u16)(p_buf[index])) & 0xFF) | (u16)(((u16)(p_buf[index + 2] & 0x30)) << 4)));
				obj[0].y       = (s16)(WPAD_DPD_IMG_RESO_WY - 1
                                 - (s16)((s16)(((u16)(p_buf[index + 1])) & 0xFF) | (u16)(((u16)(p_buf[index + 2] & 0xC0)) << 2)));
				obj[0].size    = (u16)p_wpd->dpdDummyObjSize;
				obj[0].traceId = 0;

				obj[1].x = (s16)((s16)((s16)(((u16)(p_buf[index + 3])) & 0xFF) | (u16)(((u16)(p_buf[index + 2] & (u8)(0x30 >> 4))) << 8)));
				obj[1].y
				    = (s16)(WPAD_DPD_IMG_RESO_WY - 1
				            - (s16)((s16)(((u16)(p_buf[index + 4])) & 0xFF) | (u16)(((u16)(p_buf[index + 2] & (u8)(0xC0 >> 4))) << 6)));
				obj[1].size    = (u16)p_wpd->dpdDummyObjSize;
				obj[1].traceId = 1;

				obj[2].x       = (s16)((s16)((s16)(((u16)(p_buf[index + 5])) & 0xFF) | (u16)(((u16)(p_buf[index + 7] & 0x30)) << 4)));
				obj[2].y       = (s16)(WPAD_DPD_IMG_RESO_WY - 1
                                 - (s16)((s16)(((u16)(p_buf[index + 6])) & 0xFF) | (u16)(((u16)(p_buf[index + 7] & 0xC0)) << 2)));
				obj[2].size    = (u16)p_wpd->dpdDummyObjSize;
				obj[2].traceId = 2;

				obj[3].x = (s16)((s16)((s16)(((u16)(p_buf[index + 8])) & 0xFF) | (u16)(((u16)(p_buf[index + 7] & (u8)(0x30 >> 4))) << 8)));
				obj[3].y
				    = (s16)(WPAD_DPD_IMG_RESO_WY - 1
				            - (s16)((s16)(((u16)(p_buf[index + 9])) & 0xFF) | (u16)(((u16)(p_buf[index + 7] & (u8)(0xC0 >> 4))) << 6)));
				obj[3].size    = (u16)p_wpd->dpdDummyObjSize;
				obj[3].traceId = 3;

				for (j = 0; j < WPAD_DPD_MAX_OBJECTS; j++) {
					if ((obj[j].x < (s16)(WPAD_DPD_IMG_RESO_WX / 2)) && (obj[j].y < (s16)(WPAD_DPD_IMG_RESO_WY / 2))) {
						p_wpd->conf.obj[0].x       = obj[j].x;
						p_wpd->conf.obj[0].y       = obj[j].y;
						p_wpd->conf.obj[0].size    = obj[j].size;
						p_wpd->conf.obj[0].traceId = obj[j].traceId;
					}
					if ((obj[j].x > (s16)(WPAD_DPD_IMG_RESO_WX / 2)) && (obj[j].y < (s16)(WPAD_DPD_IMG_RESO_WY / 2))) {
						p_wpd->conf.obj[1].x       = obj[j].x;
						p_wpd->conf.obj[1].y       = obj[j].y;
						p_wpd->conf.obj[1].size    = obj[j].size;
						p_wpd->conf.obj[1].traceId = obj[j].traceId;
					}
					if ((obj[j].x > (s16)(WPAD_DPD_IMG_RESO_WX / 2)) && (obj[j].y > (s16)(WPAD_DPD_IMG_RESO_WY / 2))) {
						p_wpd->conf.obj[2].x       = obj[j].x;
						p_wpd->conf.obj[2].y       = obj[j].y;
						p_wpd->conf.obj[2].size    = obj[j].size;
						p_wpd->conf.obj[2].traceId = obj[j].traceId;
					}
					if ((obj[j].x < (s16)(WPAD_DPD_IMG_RESO_WX / 2)) && (obj[j].y > (s16)(WPAD_DPD_IMG_RESO_WY / 2))) {
						p_wpd->conf.obj[3].x       = obj[j].x;
						p_wpd->conf.obj[3].y       = obj[j].y;
						p_wpd->conf.obj[3].size    = obj[j].size;
						p_wpd->conf.obj[3].traceId = obj[j].traceId;
					}
				}

				break;
			} else {
				DEBUGPrint("Dpd Setting is broken.\n");
			}
		}
	} else {
		DEBUGPrint("Old Firmware uses default values because it has no Dpd Setting.\n");
	}

	difaveX = difaveY = 0.0f;
	for (i = 0; i < WPAD_DPD_MAX_OBJECTS; i++) {
		x[i] = (f32)(p_wpd->conf.obj[i].x);
		y[i] = (f32)(p_wpd->conf.obj[i].y);
		difaveX += p_wpd->conf.obj[i].x - dummyObjX[i];
		difaveY += p_wpd->conf.obj[i].y - dummyObjY[i];

		DEBUGPrint("x = %lf, y = %lf\n", x[i], y[i]);
	}

	difaveX                 = (f32)((f32)difaveX / (f32)4.0f);
	difaveY                 = (f32)((f32)difaveY / (f32)4.0f);
	deltaX                  = (f32)atan((f32)((f32)((f32)126.5f + difaveX) / (f32)(1332.5f))) - (f32)atan((f32)((f32)126.5f / 1332.5f));
	deltaY                  = (f32)atan((f32)((f32)((f32)93.0f + difaveY) / (f32)(1337.4f))) - (f32)atan((f32)((f32)93.0f / 1337.4f));
	_wpadCalibrationX[chan] = (f32)(-1.0f * (f32)tan(deltaX)) * 1332.5f;
	_wpadCalibrationY[chan] = (f32)(-1.0f * (f32)tan(deltaY)) * 1337.4f;

	DEBUGPrint("difaveX = %lf, difaveY = %lf\n", difaveX, difaveY);
	DEBUGPrint("deltaX  = %lf, deltaY  = %lf\n", deltaX, deltaY);
	DEBUGPrint("_wpadCalibrationX = %lf, _wpadCalibrationY = %lf\n", _wpadCalibrationX[chan], _wpadCalibrationY[chan]);

	_wpadCenterX[chan] = _wpadCenterY[chan] = 0.0f;
	for (i = 0; i < WPAD_DPD_MAX_OBJECTS; i++) {
		x[i] += _wpadCalibrationX[chan];
		y[i] += _wpadCalibrationY[chan];
		_wpadCenterX[chan] += x[i];
		_wpadCenterY[chan] += y[i];
	}

	_wpadCenterX[chan] = (f32)((f32)_wpadCenterX[chan] / (f32)4.0f);
	_wpadCenterY[chan] = (f32)((f32)_wpadCenterY[chan] / (f32)4.0f);
	DEBUGPrint("centX  = %lf, centY  = %lf\n", _wpadCenterX[chan], _wpadCenterY[chan]);

	_wpadRolag[chan] = 0.0f;
	for (i = 0; i < WPAD_DPD_MAX_OBJECTS; i++) {
		ag[i]     = (f32)atan((f32)(y[i] - _wpadCenterY[chan]) / (f32)(x[i] - _wpadCenterX[chan]));
		ag_org[i] = (f32)atan((f32)((f32)(dummyObjY[i]) - 383.5f) / (f32)((f32)(dummyObjX[i]) - 511.5f));

		_wpadRolag[chan] += ag[i] - ag_org[i];
	}

	_wpadRolag[chan] = (f32)((f32)_wpadRolag[chan] / (f32)4.0f);
	DEBUGPrint("_wpadRolag  = %lf\n", _wpadRolag[chan]);

	for (i = 0; i < 2; i++) {
		checkSum = 0;

		index = i * (10);
		if (!p_wpd->oldFw) {
			index += 22;
		}

		for (j = index; j < index + 9; j++) {
			checkSum += p_buf[j];
		}
		checkSum = (u8)(checkSum + 0x55);
		if (checkSum == p_buf[index + 9]) {
			DEBUGPrint("Acc is ok.\n");
			p_wpd->conf.acc_0g.x = (s16)((u16)(((u16)(p_buf[index + 0]) << 2) & 0xFFFC) | (u16)(((u16)(p_buf[index + 3]) >> 4) & 3));
			p_wpd->conf.acc_0g.y = (s16)((u16)(((u16)(p_buf[index + 1]) << 2) & 0xFFFC) | (u16)(((u16)(p_buf[index + 3]) >> 2) & 3));
			p_wpd->conf.acc_0g.z = (s16)((u16)(((u16)(p_buf[index + 2]) << 2) & 0xFFFC) | (u16)(((u16)(p_buf[index + 3])) & 3));
			p_wpd->conf.acc_1g.x = (s16)((u16)(((u16)(p_buf[index + 4]) << 2) & 0xFFFC) | (u16)(((u16)(p_buf[index + 7]) >> 4) & 3));
			p_wpd->conf.acc_1g.y = (s16)((u16)(((u16)(p_buf[index + 5]) << 2) & 0xFFFC) | (u16)(((u16)(p_buf[index + 7]) >> 2) & 3));
			p_wpd->conf.acc_1g.z = (s16)((u16)(((u16)(p_buf[index + 6]) << 2) & 0xFFFC) | (u16)(((u16)(p_buf[index + 7])) & 3));
			p_wpd->conf.volume   = (u8)(p_buf[index + 8] & (u8)0x7F);
			p_wpd->conf.motor    = (u8)(p_buf[index + 8] & (u8)0x80);
			break;
		} else {
			DEBUGPrint("Acc is broken.\n");
		}
	}
	DEBUGPrint("0G:  accX = %d,  accY = %d,  accZ = %d\n", p_wpd->conf.acc_0g.x, p_wpd->conf.acc_0g.y, p_wpd->conf.acc_0g.z);
	DEBUGPrint("1G:  accX = %d,  accY = %d,  accZ = %d\n", p_wpd->conf.acc_1g.x, p_wpd->conf.acc_1g.y, p_wpd->conf.acc_1g.z);
	DEBUGPrint("Volume: %d,   Motor: %d\n", p_wpd->conf.volume, p_wpd->conf.motor);
}

static void __wpadGetExtConfig(s32 chan, s32 result)
{
	WPADControlBlock* p_wpd = _wpdcb[chan];
	u8* p_buf               = (u8*)p_wpd->readBufPtr;
	int i, j, k;
	int index = -1;
	int checkSum;

	switch (result) {
	case WPAD_ERR_NO_CONTROLLER:
		p_wpd->devType                  = WPAD_DEV_NOT_FOUND;
		p_wpd->lastExtensionType        = WPAD_DEV_NOT_FOUND;
		p_wpd->keepExtensionCalibration = FALSE;
		break;

	case WPAD_ERR_NONE:
		p_wpd->devType = _devType[chan];
		break;

	default:
		p_wpd->devType                  = WPAD_DEV_NOT_SUPPORTED;
		p_wpd->lastExtensionType        = WPAD_DEV_NOT_SUPPORTED;
		p_wpd->keepExtensionCalibration = FALSE;
		break;
	}
	p_wpd->devMode = _devMode[chan];

	if (p_wpd->devType == WPAD_DEV_FREESTYLE || p_wpd->devType == WPAD_DEV_CLASSIC) {
		for (i = 0; i < 2; i++) {
			checkSum = 0;
			k        = i * 16;
			for (j = k; j < k + 14; j++) {
				checkSum += p_buf[j];
			}

			if (((u8)(checkSum + 0x55) == p_buf[k + 14]) && ((u8)(checkSum + 0xaa) == p_buf[k + 15])) {
				index = k;
			}
		}

		switch (p_wpd->devType) {
		case WPAD_DEV_FREESTYLE:
			if (index < 0) {
				DEBUGPrint("check sum error.\n");
				p_wpd->extConf.fs.acc_0g.x = 512;
				p_wpd->extConf.fs.acc_0g.y = 512;
				p_wpd->extConf.fs.acc_0g.z = 512;
				p_wpd->extConf.fs.acc_1g.x = 716;
				p_wpd->extConf.fs.acc_1g.y = 716;
				p_wpd->extConf.fs.acc_1g.z = 716;
			} else {
				p_wpd->extConf.fs.acc_0g.x
				    = (s16)((u16)(((u16)(p_buf[index + 0]) << 2) & 0xFFFC) | (u16)(((u16)(p_buf[index + 3]) >> 4) & 3));
				p_wpd->extConf.fs.acc_0g.y
				    = (s16)((u16)(((u16)(p_buf[index + 1]) << 2) & 0xFFFC) | (u16)(((u16)(p_buf[index + 3]) >> 2) & 3));
				p_wpd->extConf.fs.acc_0g.z = (s16)((u16)(((u16)(p_buf[index + 2]) << 2) & 0xFFFC) | (u16)(((u16)(p_buf[index + 3])) & 3));
				p_wpd->extConf.fs.acc_1g.x
				    = (s16)((u16)(((u16)(p_buf[index + 4]) << 2) & 0xFFFC) | (u16)(((u16)(p_buf[index + 7]) >> 4) & 3));
				p_wpd->extConf.fs.acc_1g.y
				    = (s16)((u16)(((u16)(p_buf[index + 5]) << 2) & 0xFFFC) | (u16)(((u16)(p_buf[index + 7]) >> 2) & 3));
				p_wpd->extConf.fs.acc_1g.z = (s16)((u16)(((u16)(p_buf[index + 6]) << 2) & 0xFFFC) | (u16)(((u16)(p_buf[index + 7])) & 3));
				p_wpd->extConf.fs.stick.x_max = (s8)p_buf[index + 8];
				p_wpd->extConf.fs.stick.x_min = (s8)p_buf[index + 9];
				p_wpd->extConf.fs.stick.y_max = (s8)p_buf[index + 11];
				p_wpd->extConf.fs.stick.y_min = (s8)p_buf[index + 12];
				if (!p_wpd->isSetStickOrigin) {
					p_wpd->extConf.fs.stick.x = (s8)p_buf[index + 10];
					p_wpd->extConf.fs.stick.y = (s8)p_buf[index + 13];
				}

				DEBUGPrint("0G:  fsaccX = %d,  fsaccY = %d,  fsaccZ = %d\n", p_wpd->extConf.fs.acc_0g.x, p_wpd->extConf.fs.acc_0g.y,
				           p_wpd->extConf.fs.acc_0g.z);
				DEBUGPrint("1G:  fsaccX = %d,  fsaccY = %d,  fsaccZ = %d\n", p_wpd->extConf.fs.acc_1g.x, p_wpd->extConf.fs.acc_1g.y,
				           p_wpd->extConf.fs.acc_1g.z);
				DEBUGPrint("FS:  X = %d,  X max = %d,  X min = %d\n", p_wpd->extConf.fs.stick.x, p_wpd->extConf.fs.stick.x_max,
				           p_wpd->extConf.fs.stick.x_min);
				DEBUGPrint("FS:  Y = %d,  Y max = %d,  Y min = %d\n", p_wpd->extConf.fs.stick.y, p_wpd->extConf.fs.stick.y_max,
				           p_wpd->extConf.fs.stick.y_min);
			}
			break;

		case WPAD_DEV_CLASSIC:
			p_wpd->extConf.cl.lstk.x_max = (s8)p_buf[index + 0];
			p_wpd->extConf.cl.lstk.x_min = (s8)p_buf[index + 1];
			p_wpd->extConf.cl.lstk.y_max = (s8)p_buf[index + 3];
			p_wpd->extConf.cl.lstk.y_min = (s8)p_buf[index + 4];
			p_wpd->extConf.cl.rstk.x_max = (s8)p_buf[index + 6];
			p_wpd->extConf.cl.rstk.x_min = (s8)p_buf[index + 7];
			p_wpd->extConf.cl.rstk.y_max = (s8)p_buf[index + 9];
			p_wpd->extConf.cl.rstk.y_min = (s8)p_buf[index + 10];
			if (!p_wpd->isSetStickOrigin) {
				p_wpd->extConf.cl.lstk.x   = (s8)p_buf[index + 2];
				p_wpd->extConf.cl.lstk.y   = (s8)p_buf[index + 5];
				p_wpd->extConf.cl.rstk.x   = (s8)p_buf[index + 8];
				p_wpd->extConf.cl.rstk.y   = (s8)p_buf[index + 11];
				p_wpd->extConf.cl.triggerL = p_buf[index + 12];
				p_wpd->extConf.cl.triggerR = p_buf[index + 13];
			}

			DEBUGPrint("CL:  X = %d,  X max = %d,  X min = %d\n", p_wpd->extConf.cl.lstk.x, p_wpd->extConf.cl.lstk.x_max,
			           p_wpd->extConf.cl.lstk.x_min);
			DEBUGPrint("CL:  Y = %d,  Y max = %d,  Y min = %d\n", p_wpd->extConf.cl.lstk.y, p_wpd->extConf.cl.lstk.y_max,
			           p_wpd->extConf.cl.lstk.y_min);
			DEBUGPrint("CR:  X = %d,  X max = %d,  X min = %d\n", p_wpd->extConf.cl.rstk.x, p_wpd->extConf.cl.rstk.x_max,
			           p_wpd->extConf.cl.rstk.x_min);
			DEBUGPrint("CR:  Y = %d,  Y max = %d,  Y min = %d\n", p_wpd->extConf.cl.rstk.y, p_wpd->extConf.cl.rstk.y_max,
			           p_wpd->extConf.cl.rstk.y_min);
			DEBUGPrint("LR:  L = %d,  R = %d\n", p_wpd->extConf.cl.triggerL, p_wpd->extConf.cl.triggerR);
			break;
		}
	}

	if (p_wpd->devType == WPAD_DEV_BALANCE_CHECKER && p_wpd->readLength == 1) {
		return;
	}

	if (p_wpd->resultCallback && p_wpd->resultCallback == p_wpd->extensionCallback) {
		p_wpd->resultCallback(chan, p_wpd->devType);
		p_wpd->resultCallback = NULL;
	}
}

static BOOL isExtEncMain(u8 type)
{
	if (type == WPAD_DEV_FREESTYLE || type == WPAD_DEV_CLASSIC || type == WPAD_DEV_BALANCE_CHECKER || type == WPAD_DEV_GUITAR
	    || type == WPAD_DEV_DRUM || type == WPAD_DEV_VSM) {
		return TRUE;
	} else {
		return FALSE;
	}
}

static void __wpadGetExtType(s32 chan, s32 result)
{
	WPADControlBlock* p_wpd = _wpdcb[chan];
	WPADCmdQueue* cmdq      = &p_wpd->extq;
	u8* p_buf               = (u8*)p_wpd->readBufPtr;

	if (result == WPAD_ERR_NONE) {
		DEBUGPrint("devId: %d, subId: %d\n", p_buf[5], p_buf[0]);
		_devCmpt[chan] = 0;
		_devMode[chan] = p_buf[4];
		switch (p_buf[5]) {
		case 0x00:
			_devType[chan] = WPAD_DEV_FREESTYLE;
			break;
		case 0x01:
			_devType[chan] = WPAD_DEV_CLASSIC;
			break;
		case 0x02:
			_devType[chan] = (u8)(WUDIsLinkedWBC() ? WPAD_DEV_BALANCE_CHECKER : WPAD_DEV_FUTURE);
			break;
		case 0x04:
			_devType[chan] = (u8)(WPADIsEnabledVSM() ? WPAD_DEV_VSM : WPAD_DEV_FUTURE);
			break;
		case 0x10:
			_devType[chan] = (u8)(WPADIsEnabledTRN() ? WPAD_DEV_TRAIN : WPAD_DEV_FUTURE);
			break;
		case 0x03:
			if (__OSInIPL) {
				_devType[chan] = WPAD_DEV_CLASSIC;
				_devMode[chan] = 1;
				_devCmpt[chan] = 1;
			} else {
				switch (p_buf[0]) {
				case 0x00:
					_devType[chan] = (u8)(WPADIsEnabledGTR() ? WPAD_DEV_GUITAR : WPAD_DEV_FUTURE);
					break;
				case 0x01:
					_devType[chan] = (u8)(WPADIsEnabledDRM() ? WPAD_DEV_DRUM : WPAD_DEV_FUTURE);
					break;
				default:
					_devType[chan] = WPAD_DEV_FUTURE;
					break;
				}
			}
			break;
		default:
			_devType[chan] = WPAD_DEV_FUTURE;
			break;
		}

		if (_devType[chan] == WPAD_DEV_CLASSIC) {
			if (_devMode[chan] < 1 || _devMode[chan] > 3) {
				_devType[chan] = WPAD_DEV_NOT_SUPPORTED;
			}
		}

		DEBUGPrint("type : %d\n", _devType[chan]);
		DEBUGPrint("mode : %d\n", _devMode[chan]);
		DEBUGPrint("last : %d\n", p_wpd->lastExtensionType);

		if (p_wpd->keepExtensionCalibration && p_wpd->lastExtensionType == _devType[chan]) {
			DEBUGPrint("Keep calibration!\n");
			p_wpd->isSetStickOrigin = 1;
		} else {
			DEBUGPrint("Recalibration!\n");
			p_wpd->isSetStickOrigin = 0;
		}
		p_wpd->lastExtensionType        = _devType[chan];
		p_wpd->keepExtensionCalibration = FALSE;

		if (_devType[chan] == WPAD_DEV_FUTURE || _devType[chan] == WPAD_DEV_NOT_SUPPORTED) {
			p_wpd->devType = _devType[chan];
			p_wpd->devMode = _devMode[chan];

			if (p_wpd->extensionCallback) {
				p_wpd->extensionCallback(chan, p_wpd->devType);
			}
		} else {
			if (isExtEncMain(_devType[chan])) {
				p_wpd->extEnc = 2;
				WPADiCreateKey(chan);
			} else {
				p_wpd->extEnc = 3;
				WPADiCreateKeyFor3rd(chan);
			}
			WPADiSendWriteDataCmd(cmdq, 0xAA, 0x4A400F0, __wpadAbortInitExtension);
			WPADiSendWriteData(cmdq, p_wpd->key, 6, 0x4A40040, __wpadAbortInitExtension);
			WPADiSendWriteData(cmdq, p_wpd->key + 6, 6, 0x4A40046, __wpadAbortInitExtension);
			WPADiSendWriteData(cmdq, p_wpd->key + 12, 4, 0x4A4004C, __wpadAbortInitExtension);
			if (_devType[chan] == WPAD_DEV_BALANCE_CHECKER) {
				WPADiSendWriteDataCmd(cmdq, 0xAA, 0x4A400F1, __wpadAbortInitExtension);
				WPADiSendWriteDataCmd(cmdq, 0xAA, 0x4A400F1, __wpadAbortInitExtension);
				WPADiSendWriteDataCmd(cmdq, 0xAA, 0x4A400F1, __wpadAbortInitExtension);
			}
			WPADiSendReadData(cmdq, p_wpd->readBuf, 32, 0x4A40020, p_wpd->extensionCallback);
		}
	} else {
		p_wpd->devType                  = WPAD_DEV_NOT_SUPPORTED;
		p_wpd->devMode                  = 0;
		p_wpd->lastExtensionType        = WPAD_DEV_NOT_SUPPORTED;
		p_wpd->keepExtensionCalibration = FALSE;
	}
}

static void __wpadGetGameInfo(s32 chan, s32 result, u8 index)
{
	WPADControlBlock* p_wpd = _wpdcb[chan];
	WPADMEMGameInfo* p_info = (WPADMEMGameInfo*)p_wpd->readBufPtr;
	u8* p_buf               = (u8*)p_wpd->readBufPtr;
	int i;
	u8 sum = 0;

	if (result == WPAD_ERR_NONE) {
		for (i = 0; i < 47; i++) {
			sum += p_buf[i];
		}
		sum += 0x55;

		if (p_info->checkSum == sum) {
			memcpy(&p_wpd->gameInfo, p_buf, sizeof(WPADMEMGameInfo));
			p_wpd->gameInfoErr[index] = WPAD_ERR_NONE;
		} else {
			p_wpd->gameInfoErr[index] = WPAD_ERR_INVALID;
		}
	} else {
		p_wpd->gameInfoErr[index] = WPAD_ERR_INVALID;
	}
}

static void initExtension(s32 chan)
{
	WPADControlBlock* p_wpd = _wpdcb[chan];
	u8 extcmd               = 0;
	WPADiClearQueue(&p_wpd->extq);
	WPADiSendSetReportType(&p_wpd->extq, p_wpd->dataFormat, p_wpd->pwrSave, __wpadAbortInitExtension);
	p_wpd->extEnc = 1;
	WPADiSendWriteDataCmd(&p_wpd->extq, 0x55, 0x04a400f0, __wpadAbortInitExtension);
	WPADiSendWriteDataCmd(&p_wpd->extq, extcmd, 0x04a400fb, __wpadAbortInitExtension);
	WPADiSendReadData(&p_wpd->extq, p_wpd->readBuf, 6, 0x04a400fa, __wpadAbortInitExtension);
}

static void __wpadInvertRxBuffer(u8 chan)
{
	WPADControlBlock* p_wpd = _wpdcb[chan];

	p_wpd->rxBufIndex = (u8)((p_wpd->rxBufIndex) ? 0 : 1);
}

static void* __wpadGetWorkRxBuffer(u8 chan)
{
	WPADControlBlock* p_wpd = _wpdcb[chan];

	return p_wpd->rxBuf + p_wpd->rxBufIndex * WPAD_RX_DATASIZE;
}

static void* __wpadGetLastRxBuffer(u8 chan)
{
	WPADControlBlock* p_wpd = _wpdcb[chan];
	u32 index               = (u32)((p_wpd->rxBufIndex) ? 0 : 1);

	return p_wpd->rxBuf + index * WPAD_RX_DATASIZE;
}

s32 WPADiHIDParser(u8 chan, u8* data)
{
	WPADControlBlock* p_wpd = _wpdcb[chan];
	int enable;
	s32 ret = 0;
	void* p_rxBuf;

	if ((data[0] >= 0x20) && (data[0] <= 0x3F)) {
		enable  = OSDisableInterrupts();
		p_rxBuf = __wpadGetWorkRxBuffer(chan);

		if (data[0] != 0x3E && data[0] != 0x3F) {
			memset(p_rxBuf, 0, WPAD_RX_DATASIZE);
		}

		(*(__a1_input_reports_array[data[0] - 0x20]))(chan, data, p_rxBuf);

		if (!p_wpd->setup) {
			((WPADStatus*)p_rxBuf)->err = WPAD_ERR_INVALID;
		}

		if (data[0] != 0x3E && data[0] != 0x3F) {
			__wpadInvertRxBuffer(chan);
		}

		OSRestoreInterrupts(enable);
		WPADiExcludeButton(chan);
		WPADiCopyOut(chan);
	} else {
		ret = -1;
	}

	return ret;
}

static void __a1_20_status_report(u8 chan, u8* data, void* rxBuffer)
{
	WPADControlBlock* p_wpd = _wpdcb[chan];
	WPADStatus* p_status    = (WPADStatus*)rxBuffer;
	BOOL enable;
	BOOL attach = p_wpd->infos.attach;
	WPADExtensionCallback extcb;

	DEBUGPrint("Received report 20\n");

	enable = OSDisableInterrupts();

	if (!p_wpd->setup) {
		OSRestoreInterrupts(enable);
		return;
	}

	p_wpd->infos.attach   = (BOOL)((data[HID_IDX_BYTE9] & HID_ATTACH_MASK) >> 1);
	p_wpd->infos.lowBat   = (BOOL)(data[HID_IDX_BYTE9] & HID_LOWBAT_MASK);
	p_wpd->infos.led      = (u8)((data[HID_IDX_BYTE9] & HID_LED_MASK) >> 4);
	p_wpd->infos.protocol = (u8)((data[HID_IDX_BYTE11] & HID_PRT_MASK) >> 4);
	p_wpd->infos.firmware = (u8)(data[HID_IDX_BYTE11] & HID_FW_MASK);

	p_wpd->infos.nearempty = (u8)((data[HID_IDX_BYTE7] & HID_NEAREMPTY_MASK) >> 7);
	p_wpd->infos.dpd       = (BOOL)((data[HID_IDX_BYTE9] & HID_DPD_MASK) >> 3);
	p_wpd->infos.speaker   = (BOOL)((data[HID_IDX_BYTE9] & HID_SPEAKER_MASK) >> 2);

	if (p_wpd->devType == WPAD_DEV_BALANCE_CHECKER) {
		p_wpd->infos.battery = p_wpd->balanceBoardBattery;
	} else if ((u8)(data[HID_IDX_BYTE12]) >= 0x55) {
		p_wpd->infos.battery = WPAD_BATTERY_LEVEL_MAX;
	} else if ((u8)(data[HID_IDX_BYTE12]) >= 0x44) {
		p_wpd->infos.battery = WPAD_BATTERY_LEVEL_HIGH;
	} else if ((u8)(data[HID_IDX_BYTE12]) >= 0x33) {
		p_wpd->infos.battery = WPAD_BATTERY_LEVEL_MEDIUM;
	} else if ((u8)(data[HID_IDX_BYTE12]) >= 0x3) {
		p_wpd->infos.battery = WPAD_BATTERY_LEVEL_LOW;
	} else {
		p_wpd->infos.battery = WPAD_BATTERY_LEVEL_CRITICAL;
	}

	if (p_wpd->infos.attach) {
		if (!attach) {
			DEBUGPrint("initialize attachment\n");

			_retryCnt[chan] = 0;
			extcb           = p_wpd->extensionCallback;
			initExtension(chan);
			p_wpd->devType = WPAD_DEV_UNKNOWN;
			p_wpd->devMode = 0;

			if (extcb) {
				extcb(chan, WPAD_DEV_UNKNOWN);
			}
		}
	} else {
		p_wpd->devType = WPAD_DEV_CORE;
		p_wpd->devMode = 0;

		WPADiClearQueue(&p_wpd->extq);
		WPADiSendSetReportType(&p_wpd->extq, p_wpd->dataFormat, p_wpd->pwrSave, NULL);

		if (attach) {
			p_wpd->keepExtensionCalibration = TRUE;
			p_wpd->calibrationTimeout       = 300;
			if (p_wpd->extensionCallback) {
				p_wpd->extensionCallback(chan, WPAD_DEV_CORE);
			}
		}
	}

	if (p_wpd->infoBuf) {
		memcpy(p_wpd->infoBuf, &p_wpd->infos, sizeof(WPADInfo));
		p_wpd->infoBuf = NULL;
	}

	memcpy(rxBuffer, __wpadGetLastRxBuffer(chan), WPAD_RX_DATASIZE);
	p_status->button = (u16)((u16)(((u16)((u16)(data[HID_IDX_BYTE7]) & 0xFF) | (u16)((u16)(data[HID_IDX_BYTE8] << 8) & 0xFF00)))
	                         & (u16)HID_WPAD_BUTTON_MASK);
	if (p_status->dev != p_wpd->devType) {
		p_status->dev = p_wpd->devType;
		p_status->err = WPAD_ERR_INVALID;
	}

	if (p_wpd->resultCallback && p_wpd->getStatFlag) {
		p_wpd->resultCallback(chan, WPAD_ERR_NONE);
		p_wpd->resultCallback = NULL;
	}

	p_wpd->getStatFlag = 0;

	OSRestoreInterrupts(enable);
}

static void __a1_21_user_data(u8 chan, u8* data, void* rxBuffer)
{
	WPADControlBlock* p_wpd = _wpdcb[chan];
	WPADStatus* p_status    = (WPADStatus*)rxBuffer;
	BOOL enable;
	u16 addr;
	u16 base;
	u16 i2c;
	u8 err;
	u8 len;
	s16 offset;
	s32 status;

	enable = OSDisableInterrupts();

	err = (u8)((u8)(data[HID_IDX_BYTE9]) & 0x0f);
	if (err != 0) {
		DEBUGPrint("read error happens!\n");
		p_wpd->readError = -1;

		if (p_wpd->resultCallback) {
			if (!p_wpd->extensionCallback || p_wpd->extensionCallback != p_wpd->resultCallback) {
				p_wpd->resultCallback(chan, WPAD_ERR_TRANSFER);
			}
			p_wpd->resultCallback = NULL;
		}
		p_wpd->status = WPAD_ERR_NONE;
	}

	len  = (u8)(((u8)((u8)(data[HID_IDX_BYTE9]) >> 4) & 0x0f) + 1);
	addr = (u16)((u16)((u16)((u16)(data[HID_IDX_BYTE10]) << 8) & (u16)0xFF00) | (u16)((u16)(data[HID_IDX_BYTE11]) & (u16)0xFF));

	base   = (u16)((p_wpd->readBaseAddr & 0xFFFF));
	i2c    = (u16)((p_wpd->readBaseAddr & 0xFFFF0000) >> 16);
	offset = (s16)(addr - base);

	if (addr >= base && addr <= base + p_wpd->readLength) {
		memcpy((u8*)p_wpd->readBufPtr + offset, data + HID_IDX_BYTE12, len);

		if (base + p_wpd->readLength == addr + len) {
			DEBUGPrint("base addr: %08x\n", p_wpd->readBaseAddr);
			DEBUGPrint("length   : %d\n", p_wpd->readLength);
			status = (p_wpd->readError < 0) ? WPAD_ERR_TRANSFER : WPAD_ERR_NONE;
			DEBUGPrint("i2c = %04x\n", i2c);
			DEBUGPrint("enc = %d\n", p_wpd->extEnc);

			if (i2c == 0x04a4) {
				DEBUGPrint("Access to extension register.\n");
				if (p_wpd->extEnc == 2 || p_wpd->extEnc == 3) {
					DEBUGPrint("Decode!!!!\n");
					DEBUGPrint("    len = %d, addr = %04x\n", len, base);

					WPADiDecode(chan, p_wpd->readBufPtr, len, base);
				}
			}

			if ((p_wpd->readBaseAddr == 0 && p_wpd->oldFw == FALSE) || (p_wpd->readBaseAddr == 0x176C && p_wpd->oldFw == TRUE)) {
				__wpadGetDevConfig(chan, status);
			}
			if (p_wpd->readBaseAddr == 0x04a40020) {
				__wpadGetExtConfig(chan, status);
			}
			if (p_wpd->readBaseAddr == 0x04a400fa) {
				__wpadGetExtType(chan, status);
			}
			if (p_wpd->readBaseAddr == 0x2A) {
				__wpadGetGameInfo(chan, status, 0);
			}
			if (p_wpd->readBaseAddr == 0x2A + sizeof(WPADMEMGameInfo)) {
				__wpadGetGameInfo(chan, status, 1);
			}

			if (p_wpd->resultCallback) {
				p_wpd->resultCallback(chan, status);
				p_wpd->resultCallback = NULL;
			}
			p_wpd->status = WPAD_ERR_NONE;
		}
	} else {
		DEBUGPrint("received data is out of range!\n");
	}

	memcpy(rxBuffer, __wpadGetLastRxBuffer(chan), WPAD_RX_DATASIZE);
	p_status->button = (u16)((u16)(((u16)((u16)(data[HID_IDX_BYTE7]) & 0xFF) | (u16)((u16)(data[HID_IDX_BYTE8] << 8) & 0xFF00)))
	                         & (u16)HID_WPAD_BUTTON_MASK);
	if (p_status->dev != p_wpd->devType) {
		p_status->dev = p_wpd->devType;
		p_status->err = WPAD_ERR_INVALID;
	}

	OSRestoreInterrupts(enable);
}

static void __a1_22_ack(u8 chan, u8* data, void* rxBuffer)
{
	WPADControlBlock* p_wpd = _wpdcb[chan];
	WPADStatus* p_status    = (WPADStatus*)rxBuffer;
	u8 type;
	u8 err;
	BOOL enable;
	s32 status;

	enable = OSDisableInterrupts();

	DEBUGPrint("Received ack!\n");

	memcpy(rxBuffer, __wpadGetLastRxBuffer(chan), WPAD_RX_DATASIZE);
	p_status->button = (u16)((u16)(((u16)((u16)(data[HID_IDX_BYTE7]) & 0xFF) | (u16)((u16)(data[HID_IDX_BYTE8] << 8) & 0xFF00)))
	                         & (u16)HID_WPAD_BUTTON_MASK);
	if (p_status->dev != p_wpd->devType) {
		p_status->dev = p_wpd->devType;
		p_status->err = WPAD_ERR_INVALID;
	}

	type = (u8)data[HID_IDX_BYTE9];
	err  = (u8)data[HID_IDX_BYTE10];

	status = (err == 0) ? WPAD_ERR_NONE : WPAD_ERR_TRANSFER;

	DEBUGPrint("ack --> report ID = %02x, error code = %d\n", type, err);

	if (p_wpd->cmdId == type) {
		if (p_wpd->resultCallback) {
			p_wpd->resultCallback(chan, status);
			p_wpd->resultCallback = NULL;
		}
		p_wpd->status = WPAD_ERR_NONE;
	} else {
		DEBUGPrint("invalid ack!\n");
	}

	OSRestoreInterrupts(enable);
}

static void __parse_dpd_data(s32 chan, WPADStatus** p_status, u8 fmt, u8* p_data, u8 len)
{
	u8 i;
	u8 x;
	u8 y;
	u8 xy;
	f32 a;
	f32 b;
	WPADControlBlock* p_wpd = _wpdcb[chan];

	if (p_wpd->dpdCommandPending) {
		(*p_status)->err = WPAD_ERR_INVALID;
		return;
	}

	if (fmt == 3) {
		for (i = 0; i < WPAD_DPD_MAX_OBJECTS; i++) {
			if (i * 3 + 2 < len) {
				x  = *(p_data + i * 3);
				y  = *(p_data + i * 3 + 1);
				xy = *(p_data + i * 3 + 2);

				(*p_status)->obj[i].x    = (s16)((s16)((s16)(((u16)x) & 0xFF) | (u16)(((u16)xy & 0x30) << 4)));
				(*p_status)->obj[i].y    = (s16)(WPAD_DPD_IMG_RESO_WY - 1 - (s16)((s16)(((u16)y) & 0xFF) | (u16)(((u16)xy & 0xC0) << 2)));
				(*p_status)->obj[i].size = (u16)(((xy & 0xF) & 0xFF));
				(*p_status)->obj[i].size = (u8)((f32)((*p_status)->obj[i].size) * (f32)((*p_status)->obj[i].size) * 3.141592f);
				if ((*p_status)->obj[i].size == 0 || (*p_status)->obj[i].x == (s16)(WPAD_DPD_IMG_RESO_WX - 1)
				    || (*p_status)->obj[i].y == (s16)(WPAD_DPD_IMG_RESO_WY - 1)) {
					(*p_status)->obj[i].x    = (s16)0;
					(*p_status)->obj[i].y    = (s16)(WPAD_DPD_IMG_RESO_WY - 1);
					(*p_status)->obj[i].size = 0;
				}
				(*p_status)->obj[i].traceId = i;
			} else {
				(*p_status)->obj[i].x       = (s16)0;
				(*p_status)->obj[i].y       = (s16)(WPAD_DPD_IMG_RESO_WY - 1);
				(*p_status)->obj[i].size    = 0;
				(*p_status)->obj[i].traceId = i;
			}
		}
	} else if (fmt == 1) {
		for (i = 0; i < WPAD_DPD_MAX_OBJECTS; i++) {
			x  = *(p_data + i * 3 - (i / 2));
			y  = *(p_data + i * 3 - (i / 2) + 1);
			xy = *(p_data + (i / 2) * 5 + 2);

			if ((i % 2) == 0) {
				(*p_status)->obj[i].x = (s16)((s16)((s16)(((u16)x) & 0xFF) | (u16)(((u16)(xy & 0x30)) << 4)));
				(*p_status)->obj[i].y = (s16)(WPAD_DPD_IMG_RESO_WY - 1 - (s16)((s16)(((u16)y) & 0xFF) | (u16)(((u16)(xy & 0xC0)) << 2)));
			} else {
				(*p_status)->obj[i].x = (s16)((s16)((s16)(((u16)x) & 0xFF) | (u16)(((u16)(xy & (u8)(0x30 >> 4))) << 8)));
				(*p_status)->obj[i].y
				    = (s16)(WPAD_DPD_IMG_RESO_WY - 1 - (s16)((s16)(((u16)y) & 0xFF) | (u16)(((u16)(xy & (u8)(0xC0 >> 4))) << 6)));
			}
			if ((*p_status)->obj[i].x == (s16)(WPAD_DPD_IMG_RESO_WX - 1) || (*p_status)->obj[i].y == (s16)(WPAD_DPD_IMG_RESO_WY - 1)) {
				(*p_status)->obj[i].x    = (s16)0;
				(*p_status)->obj[i].y    = (s16)(WPAD_DPD_IMG_RESO_WY - 1);
				(*p_status)->obj[i].size = 0;
			} else {
				(*p_status)->obj[i].size = 12;
			}
			(*p_status)->obj[i].traceId = i;
		}
	}

	for (i = 0; i < WPAD_DPD_MAX_OBJECTS; i++) {
		if ((*p_status)->obj[i].x != 0 || (*p_status)->obj[i].y != (WPAD_DPD_IMG_RESO_WY - 1)) {
			WPADStatus* p_cur = *p_status;
			a                 = (((f32)(*p_status)->obj[i].x) + _wpadCalibrationX[chan] - _wpadCenterX[chan]);
			b                 = (((f32)p_cur->obj[i].y) + _wpadCalibrationY[chan] - _wpadCenterY[chan]);
			p_cur->obj[i].x = (s16)((f32)((f32)a * (f32)cos(-1.0f * _wpadRolag[chan])) - (f32)((f32)b * (f32)sin(-1.0f * _wpadRolag[chan]))
			                        + _wpadCenterX[chan]);
			(*p_status)->obj[i].y = (s16)((f32)((f32)a * (f32)sin(-1.0f * _wpadRolag[chan]))
			                              + (f32)((f32)b * (f32)cos(-1.0f * _wpadRolag[chan])) + _wpadCenterY[chan]);
		}
	}
}

static void __parse_dpdex_data(s32 chan, WPADStatusEx** p_status, u8 index, u8* p_data, u8 len)
{
	(*p_status)->obj[index].x = (s16)((s16)((s16)(((u16)p_data[0]) & 0xFF) | (u16)(((u16)p_data[2] & 0x30) << 4)));
	(*p_status)->obj[index].y
	    = (s16)(WPAD_DPD_IMG_RESO_WY - 1 - (s16)((s16)(((u16)p_data[1]) & 0xFF) | (u16)(((u16)p_data[2] & 0xC0) << 2)));
	(*p_status)->exp[index].pixel = (u16)(((u16)((s16)((u16)((u16)(p_data[7]) << 8) & (u16)0xFF00) | (u16)(p_data[8])) & (u16)0x03FF) << 6);
	(*p_status)->exp[index].radius = (s8)(p_data[2] & 0xF);

	(*p_status)->exp[index].range_x1 = (s16)(((s8)p_data[3] == -1) ? 0 : p_data[3]);
	(*p_status)->exp[index].range_y1 = (s16)(((s8)p_data[4] == -1) ? 0 : p_data[4]);
	(*p_status)->exp[index].range_x2 = (s16)(((s8)p_data[5] == -1) ? 0 : p_data[5]);
	(*p_status)->exp[index].range_y2 = (s16)(((s8)p_data[6] == -1) ? 0 : p_data[6]);

	(*p_status)->exp[index].range_x1 = (s16)((*p_status)->exp[index].range_x1 << 3);
	(*p_status)->exp[index].range_y1 = (s16)(WPAD_DPD_IMG_RESO_WY - 1 - (s16)((*p_status)->exp[index].range_y1 << 3));
	(*p_status)->exp[index].range_x2 = (s16)((*p_status)->exp[index].range_x2 << 3);
	(*p_status)->exp[index].range_y2 = (s16)(WPAD_DPD_IMG_RESO_WY - 1 - (s16)((*p_status)->exp[index].range_y2 << 3));

	(*p_status)->obj[index].size = (u16)((f32)((*p_status)->exp[index].radius) * (f32)((*p_status)->exp[index].radius) * 3.141592f);

	if ((*p_status)->obj[index].size == 0 || (*p_status)->obj[index].x == (s16)(WPAD_DPD_IMG_RESO_WX - 1)
	    || (*p_status)->obj[index].y == (s16)(WPAD_DPD_IMG_RESO_WY - 1) || (*p_status)->exp[index].radius == 15) {
		(*p_status)->obj[index].x      = (s16)0;
		(*p_status)->obj[index].y      = (s16)(WPAD_DPD_IMG_RESO_WY - 1);
		(*p_status)->obj[index].size   = 0;
		(*p_status)->exp[index].pixel  = 0;
		(*p_status)->exp[index].radius = 0;
	}
	(*p_status)->obj[index].traceId = index;
}

static void __parse_acc_data(s32 chan, WPADStatus** p_status, u8 fmt, u8* p_data, u8 len)
{
	WPADControlBlock* p_wpd = _wpdcb[chan];

	switch (fmt) {
	case 0:
		(*p_status)->accX
		    = (s16)((s16)((s16)((s16)((s16)((s16)(p_data[2]) << 2) & (s16)0xFFFC) | (s16)((s16)((u16)(p_data[0] >> 5)) & (s16)0x0003)))
		            - (s16)(p_wpd->conf.acc_0g.x));
		(*p_status)->accY
		    = (s16)((s16)((s16)((s16)((s16)((s16)(p_data[3]) << 2) & (s16)0xFFFC) | (s16)((s16)((u16)(p_data[1] >> 4)) & (s16)0x0002)))
		            - (s16)(p_wpd->conf.acc_0g.y));
		(*p_status)->accZ
		    = (s16)((s16)((s16)((s16)((s16)((s16)(p_data[4]) << 2) & (s16)0xFFFC) | (s16)((s16)((u16)(p_data[1] >> 5)) & (s16)0x0002)))
		            - (s16)(p_wpd->conf.acc_0g.z));
		break;

	case 1:
		(*p_status)->accX
		    = (s16)((s16)((s16)((s16)((s16)((s16)(p_data[2]) << 2) & (s16)0xFFFC) | (s16)((s16)((u16)(p_data[0] >> 6)) & (s16)0x0002)))
		            - (s16)(p_wpd->conf.acc_0g.x));
		(*p_status)->accZ
		    = (s16)((*p_status)->accZ
		            | (s16)((s16)((s16)((s16)(p_data[1]) << 3) & (s16)0xFF00) | (s16)((s16)((u16)(p_data[0] << 1)) & (s16)0x00C0)));
		break;

	case 2:
		(*p_status)->accY
		    = (s16)((s16)((s16)((s16)((s16)((s16)(p_data[2]) << 2) & (s16)0xFFFC) | (s16)((s16)((u16)(p_data[0] >> 6)) & (s16)0x0002)))
		            - (s16)(p_wpd->conf.acc_0g.y));
		(*p_status)->accZ
		    = (s16)((*p_status)->accZ
		            | (s16)((s16)((s16)((u16)(p_data[1] >> 1)) & (s16)0x0030) | (s16)((s16)((u16)(p_data[0] >> 3)) & (s16)0x000C)));
		break;
	}
}

static s16 __clamp(s16 x, s16 min, s16 max)
{
	s16 result = x;

	if (x < min) {
		result = min;
	}
	if (x > max) {
		result = max;
	}

	return result;
}

static void __parse_fs_data(s32 chan, WPADFSStatus** p_status, u8 fmt, u8* p_data, u8 len)
{
	WPADControlBlock* p_wpd = _wpdcb[chan];

	(*p_status)->fsStickX = (s8)(p_data[0]);
	(*p_status)->fsStickY = (s8)(p_data[1]);
	(*p_status)->fsAccX   = (s16)((s16)((s16)((s16)((s16)(p_data[2]) << 2) & (s16)0xFFFC) | (s16)((s16)(p_data[5] >> 2) & (s16)(0x0003)))
                                - (s16)(p_wpd->extConf.fs.acc_0g.x));
	(*p_status)->fsAccY   = (s16)((s16)((s16)((s16)((s16)(p_data[3]) << 2) & (s16)0xFFFC) | (s16)((s16)(p_data[5] >> 4) & (s16)(0x0003)))
                                - (s16)(p_wpd->extConf.fs.acc_0g.y));
	(*p_status)->fsAccZ   = (s16)((s16)((s16)((s16)((s16)(p_data[4]) << 2) & (s16)0xFFFC) | (s16)((s16)(p_data[5] >> 6) & (s16)(0x0003)))
                                - (s16)(p_wpd->extConf.fs.acc_0g.z));

	(*p_status)->button |= (u16)(((u16)(~p_data[5]) << 13) & (u16)0x6000);

	if (!p_wpd->isSetStickOrigin) {
		p_wpd->isSetStickOrigin   = 1;
		p_wpd->extConf.fs.stick.x = (*p_status)->fsStickX;
		p_wpd->extConf.fs.stick.y = (*p_status)->fsStickY;
	}

	(*p_status)->fsStickX = (s8)__clamp((s16)((u8)(*p_status)->fsStickX - (u8)p_wpd->extConf.fs.stick.x), -128, 127);
	(*p_status)->fsStickY = (s8)__clamp((s16)((u8)(*p_status)->fsStickY - (u8)p_wpd->extConf.fs.stick.y), -128, 127);
}

static void __parse_cl_data(s32 chan, WPADCLStatus** p_status, u8 mode, u8* p_data, u8 len)
{
	WPADControlBlock* p_wpd = _wpdcb[chan];

	switch (mode) {
	case 2:
		(*p_status)->clLStickX = (s16)((s16)((s16)((s16)(p_data[0]) << 2) & (s16)0xFFFC) | (s16)((s16)((s16)(p_data[4])) & (s16)0x0003));
		(*p_status)->clRStickX
		    = (s16)((s16)((s16)((s16)(p_data[1]) << 2) & (s16)0xFFFC) | (s16)((s16)((s16)(p_data[4]) >> 2) & (s16)0x0003));
		(*p_status)->clLStickY
		    = (s16)((s16)((s16)((s16)(p_data[2]) << 2) & (s16)0xFFFC) | (s16)((s16)((s16)(p_data[4]) >> 4) & (s16)0x0003));
		(*p_status)->clRStickY
		    = (s16)((s16)((s16)((s16)(p_data[3]) << 2) & (s16)0xFFFC) | (s16)((s16)((s16)(p_data[4]) >> 6) & (s16)0x0003));
		(*p_status)->clTriggerL = (u8)(p_data[5]);
		(*p_status)->clTriggerR = (u8)((len < 9) ? 0 : p_data[6]);

		(*p_status)->clButton
		    = (u16)((len < 9) ? 0 : (u16)((u16)((u16)((u16)(p_data[7]) << 8) & (u16)0xFF00 | (u16)(p_data[8]) & (u16)0xFF) ^ 0xFFFF));
		break;

	case 3:
		(*p_status)->clLStickX  = (s16)((s16)((s16)(p_data[0]) & (s16)0xFF) << 2);
		(*p_status)->clRStickX  = (s16)((s16)((s16)(p_data[1]) & (s16)0xFF) << 2);
		(*p_status)->clLStickY  = (s16)((s16)((s16)(p_data[2]) & (s16)0xFF) << 2);
		(*p_status)->clRStickY  = (s16)((s16)((s16)(p_data[3]) & (s16)0xFF) << 2);
		(*p_status)->clTriggerL = (u8)(p_data[4]);
		(*p_status)->clTriggerR = (u8)(p_data[5]);

		(*p_status)->clButton
		    = (u16)((len < 8) ? 0 : (u16)((u16)((u16)((u16)(p_data[6]) << 8) & (u16)0xFF00 | (u16)(p_data[7]) & (u16)0xFF) ^ 0xFFFF));
		break;

	case 1:
	default:
		(*p_status)->clLStickX = (s16)((s16)((s16)(p_data[0]) & (s16)0x003F) << 4);
		(*p_status)->clLStickY = (s16)((s16)((s16)(p_data[1]) & (s16)0x003F) << 4);
		(*p_status)->clRStickX
		    = (s16)((s16)((s16)((s16)((s16)(p_data[0]) >> 3) & (s16)0x0018) | (s16)((s16)((s16)(p_data[1]) >> 5) & (s16)0x0006)
		                  | (s16)((s16)((s16)(p_data[2]) >> 7) & (s16)0x0001))
		            << 5);
		(*p_status)->clRStickY  = (s16)((s16)((s16)(p_data[2]) & (s16)0x001F) << 5);
		(*p_status)->clTriggerL = (u8)((u8)((u8)((u8)(p_data[2] >> 2) & (u8)0x18) | (u8)((u8)(p_data[3] >> 5) & (u8)0x07)) << 3);
		(*p_status)->clTriggerR = (u8)((u8)((u8)(p_data[3]) & (u8)0x1F) << 3);

		(*p_status)->clButton = (u16)((u16)((u16)((u16)(p_data[4]) << 8) & (u16)0xFF00 | (u16)(p_data[5]) & (u16)0xFF) ^ 0xFFFF);
		break;
	}

	if (p_wpd->devType == WPAD_DEV_CLASSIC) {
		(*p_status)->clLStickX -= (s16)(1024 / 2);
		(*p_status)->clLStickY -= (s16)(1024 / 2);
		(*p_status)->clRStickX -= (s16)(1024 / 2);
		(*p_status)->clRStickY -= (s16)(1024 / 2);
	} else {
		(*p_status)->clLStickX -= (s16)(1024 / 2);
		(*p_status)->clLStickY -= (s16)(1024 / 2);
	}

	if (!p_wpd->isSetStickOrigin) {
		p_wpd->isSetStickOrigin  = 1;
		p_wpd->extConf.cl.lstk.x = (*p_status)->clLStickX;
		p_wpd->extConf.cl.lstk.y = (*p_status)->clLStickY;
		if (p_wpd->devType == WPAD_DEV_CLASSIC) {
			p_wpd->extConf.cl.rstk.x   = (*p_status)->clRStickX;
			p_wpd->extConf.cl.rstk.y   = (*p_status)->clRStickY;
			p_wpd->extConf.cl.triggerL = (*p_status)->clTriggerL;
			p_wpd->extConf.cl.triggerR = (*p_status)->clTriggerR;
		} else {
			p_wpd->extConf.cl.rstk.x   = 0;
			p_wpd->extConf.cl.rstk.y   = 0;
			p_wpd->extConf.cl.triggerL = 0;
			p_wpd->extConf.cl.triggerR = 0;
		}
	}

	(*p_status)->clLStickX = __clamp((s16)((*p_status)->clLStickX - p_wpd->extConf.cl.lstk.x), -512, 511);
	(*p_status)->clLStickY = __clamp((s16)((*p_status)->clLStickY - p_wpd->extConf.cl.lstk.y), -512, 511);

	if (p_wpd->devType == WPAD_DEV_CLASSIC) {
		(*p_status)->clRStickX  = __clamp((s16)((*p_status)->clRStickX - p_wpd->extConf.cl.rstk.x), -512, 511);
		(*p_status)->clRStickY  = __clamp((s16)((*p_status)->clRStickY - p_wpd->extConf.cl.rstk.y), -512, 511);
		(*p_status)->clTriggerL = (u8)__clamp((s16)((u8)(*p_status)->clTriggerL - (u8)p_wpd->extConf.cl.triggerL), 0, 255);
		(*p_status)->clTriggerR = (u8)__clamp((s16)((u8)(*p_status)->clTriggerR - (u8)p_wpd->extConf.cl.triggerR), 0, 255);
	}

	if (_devCmpt[chan]) {
		(*p_status)->clRStickX  = 0;
		(*p_status)->clRStickY  = 0;
		(*p_status)->clTriggerL = 0;
		(*p_status)->clTriggerR = 0;
	}
}

static void __parse_vs_data(s32 chan, WPADVSStatus** p_status, u8 fmt, u8* p_data, u8 len)
{
	(*p_status)->at_0x40    = p_data[0];
	(*p_status)->at_0x36[0] = (u16)((u16)((u16)((u16)(p_data[1]) << 2) & (u16)0xFFFC) | (u16)((u16)(p_data[6] >> 6) & (u16)0x0003));
	(*p_status)->at_0x36[1] = (u16)((u16)((u16)((u16)(p_data[2]) << 2) & (u16)0xFFFC) | (u16)((u16)(p_data[6] >> 4) & (u16)0x0003));
	(*p_status)->at_0x36[2] = (u16)((u16)((u16)((u16)(p_data[3]) << 2) & (u16)0xFFFC) | (u16)((u16)(p_data[6] >> 2) & (u16)0x0003));
	(*p_status)->at_0x36[3] = (u16)((u16)((u16)((u16)(p_data[4]) << 2) & (u16)0xFFFC) | (u16)((u16)(p_data[6]) & (u16)0x0003));
	(*p_status)->at_0x36[4] = (u16)((u16)((u16)((u16)(p_data[5]) << 2) & (u16)0xFFFC) | (u16)((u16)(p_data[7] >> 6) & (u16)0x0003));

	(*p_status)->at_0x34    = p_data[8];
	(*p_status)->at_0x2a[0] = (u16)((u16)((u16)((u16)(p_data[9]) << 2) & (u16)0xFFFC) | (u16)((u16)(p_data[14] >> 6) & (u16)0x0003));
	(*p_status)->at_0x2a[1] = (u16)((u16)((u16)((u16)(p_data[10]) << 2) & (u16)0xFFFC) | (u16)((u16)(p_data[14] >> 4) & (u16)0x0003));
	(*p_status)->at_0x2a[2] = (u16)((u16)((u16)((u16)(p_data[11]) << 2) & (u16)0xFFFC) | (u16)((u16)(p_data[14] >> 2) & (u16)0x0003));
	(*p_status)->at_0x2a[3] = (u16)((u16)((u16)((u16)(p_data[12]) << 2) & (u16)0xFFFC) | (u16)((u16)(p_data[14]) & (u16)0x0003));
	(*p_status)->at_0x2a[4] = (u16)((u16)((u16)((u16)(p_data[13]) << 2) & (u16)0xFFFC) | (u16)((u16)(p_data[15] >> 6) & (u16)0x0003));

	(*p_status)->at_0x42 = (u16)((u16)((u16)((u16)(p_data[7]) << 4) & (u16)0xF3F0) | (u16)((u16)(p_data[15] >> 2) & (u16)0x000F));
	(*p_status)->at_0x46 = (u8)(p_data[15] & 3);

	if (len < 21) {
		(*p_status)->at_0x44 = 0;
		(*p_status)->at_0x47 = 0;
		(*p_status)->at_0x48 = 0;
		(*p_status)->at_0x49 = 0;
	} else {
		(*p_status)->at_0x44 = (u16)((u16)((u16)((u16)(p_data[16]) << 2) & (u16)0xFFFC) | (u16)((u16)(p_data[17] >> 6) & (u16)0x0003));
		(*p_status)->at_0x47 = (u8)(p_data[18] & 7);
		(*p_status)->at_0x48 = p_data[19];
		(*p_status)->at_0x49 = p_data[20];
	}
}

static void __a1_30_data_type(u8 chan, u8* data, void* rxBuffer)
{
	WPADControlBlock* p_cb = _wpdcb[chan];
	WPADStatus* p_status   = (WPADStatus*)rxBuffer;

	p_status->button = (u16)((u16)(((u16)((u16)(data[HID_IDX_BYTE7]) & 0xFF) | (u16)((u16)(data[HID_IDX_BYTE8] << 8) & 0xFF00)))
	                         & (u16)HID_WPAD_BUTTON_MASK);
	p_status->dev    = p_cb->devType;

	if (p_cb->dataFormat == WPAD_FMT_CORE) {
		p_status->err = WPAD_ERR_NONE;
	} else {
		p_status->err = WPAD_ERR_INVALID;
	}

	p_cb->infos.nearempty = (u8)((data[HID_IDX_BYTE7] & HID_NEAREMPTY_MASK) >> 7);
}

static void __a1_31_data_type(u8 chan, u8* data, void* rxBuffer)
{
	WPADControlBlock* p_cb = _wpdcb[chan];
	WPADStatus* p_status   = (WPADStatus*)rxBuffer;

	p_status->button = (u16)((u16)(((u16)((u16)(data[HID_IDX_BYTE7]) & 0xFF) | (u16)((u16)(data[HID_IDX_BYTE8] << 8) & 0xFF00)))
	                         & (u16)HID_WPAD_BUTTON_MASK);
	p_status->dev    = p_cb->devType;

	if (p_cb->dataFormat == WPAD_FMT_CORE || p_cb->dataFormat == WPAD_FMT_CORE_ACC) {
		p_status->err = WPAD_ERR_NONE;
	} else {
		p_status->err = WPAD_ERR_INVALID;
	}

	p_cb->infos.nearempty = (u8)((data[HID_IDX_BYTE7] & HID_NEAREMPTY_MASK) >> 7);
	__parse_acc_data(chan, &p_status, 0, data + HID_IDX_BYTE7, 5);
}

static void __a1_32_data_type(u8 chan, u8* data, void* rxBuffer)
{
	WPADControlBlock* p_cb = _wpdcb[chan];
	WPADStatus* p_status   = (WPADStatus*)rxBuffer;
	void* p_extStat        = rxBuffer;
	WPADTRStatus* p_trStat;

	p_status->button = (u16)((u16)(((u16)((u16)(data[HID_IDX_BYTE7]) & 0xFF) | (u16)((u16)(data[HID_IDX_BYTE8] << 8) & 0xFF00)))
	                         & (u16)HID_WPAD_BUTTON_MASK);

	if (p_cb->dataFormat == WPAD_FMT_CORE || p_cb->dataFormat == WPAD_FMT_FREESTYLE || p_cb->dataFormat == WPAD_FMT_CLASSIC
	    || p_cb->dataFormat == WPAD_FMT_TRAIN) {
		p_status->err = WPAD_ERR_NONE;
	} else {
		p_status->err = WPAD_ERR_INVALID;
	}

	p_status->dev         = p_cb->devType;
	p_cb->infos.nearempty = (u8)((data[HID_IDX_BYTE7] & HID_NEAREMPTY_MASK) >> 7);

	memcpy(checkBuffer, data + HID_IDX_BYTE9, 8);
	WPADiDecode(chan, data + HID_IDX_BYTE9, 8, 0);

	if (p_cb->infos.attach) {
		if (p_cb->devType == WPAD_DEV_FREESTYLE) {
			__parse_fs_data(chan, (WPADFSStatus**)&p_extStat, 0, data + HID_IDX_BYTE9, 8);
		} else if (p_cb->devType == WPAD_DEV_CLASSIC) {
			__parse_cl_data(chan, (WPADCLStatus**)&p_extStat, p_cb->devMode, data + HID_IDX_BYTE9, 8);
		} else if (p_cb->devType == WPAD_DEV_TRAIN) {
			p_trStat         = (WPADTRStatus*)p_extStat;
			p_trStat->brake  = data[HID_IDX_BYTE11];
			p_trStat->mascon = data[HID_IDX_BYTE12];
			p_trStat->trButton
			    = (u16)((u16)((u16)((u16)(data[HID_IDX_BYTE15]) << 8) & (u16)0xFF00 | (u16)(data[HID_IDX_BYTE16]) & (u16)0xFF) ^ 0xFFFF);
		}

		if (!memcmp(checkBuffer, checkInvalidData, 8)) {
			if (p_status->err == WPAD_ERR_NONE) {
				p_status->err = WPAD_ERR_CORRUPTED;
			}
		}
	}
}

static void __a1_33_data_type(u8 chan, u8* data, void* rxBuffer)
{
	WPADControlBlock* p_cb = _wpdcb[chan];
	WPADStatus* p_status   = (WPADStatus*)rxBuffer;

	p_status->button = (u16)((u16)(((u16)((u16)(data[HID_IDX_BYTE7]) & 0xFF) | (u16)((u16)(data[HID_IDX_BYTE8] << 8) & 0xFF00)))
	                         & (u16)HID_WPAD_BUTTON_MASK);
	p_status->dev    = p_cb->devType;

	if (p_cb->dataFormat == WPAD_FMT_CORE || p_cb->dataFormat == WPAD_FMT_CORE_ACC || p_cb->dataFormat == WPAD_FMT_CORE_ACC_DPD) {
		p_status->err = WPAD_ERR_NONE;
	} else {
		p_status->err = WPAD_ERR_INVALID;
	}

	p_cb->infos.nearempty = (u8)((data[HID_IDX_BYTE7] & HID_NEAREMPTY_MASK) >> 7);
	__parse_acc_data(chan, &p_status, 0, data + HID_IDX_BYTE7, 5);
	__parse_dpd_data(chan, &p_status, p_cb->dpdFormat, data + HID_IDX_BYTE12, 12);
}

static void __a1_34_data_type(u8 chan, u8* data, void* rxBuffer)
{
	WPADControlBlock* p_cb = _wpdcb[chan];
	WPADStatus* p_status   = (WPADStatus*)rxBuffer;
	WPADBLStatus* p_blStat = (WPADBLStatus*)rxBuffer;
	int level;

	p_status->button = (u16)((u16)(((u16)((u16)(data[HID_IDX_BYTE7]) & 0xFF) | (u16)((u16)(data[HID_IDX_BYTE8] << 8) & 0xFF00)))
	                         & (u16)HID_WPAD_BUTTON_MASK);

	if (p_cb->dataFormat == WPAD_FMT_BALANCE_CHECKER) {
		p_status->err = WPAD_ERR_NONE;
	} else {
		p_status->err = WPAD_ERR_INVALID;
	}

	p_status->dev         = p_cb->devType;
	p_cb->infos.nearempty = (u8)((data[HID_IDX_BYTE7] & HID_NEAREMPTY_MASK) >> 7);

	memcpy(checkBuffer, data + HID_IDX_BYTE9, 19);
	WPADiDecode(chan, data + HID_IDX_BYTE9, 19, 0);

	if (p_cb->infos.attach) {
		if (p_cb->devType == WPAD_DEV_BALANCE_CHECKER) {
			p_blStat->press[0] = (u16)((u16)((u16)(data[HID_IDX_BYTE9]) << 8) & (u16)0xFF00 | (u16)(data[HID_IDX_BYTE10]) & (u16)0xFF);
			p_blStat->press[1] = (u16)((u16)((u16)(data[HID_IDX_BYTE11]) << 8) & (u16)0xFF00 | (u16)(data[HID_IDX_BYTE12]) & (u16)0xFF);
			p_blStat->press[2] = (u16)((u16)((u16)(data[HID_IDX_BYTE13]) << 8) & (u16)0xFF00 | (u16)(data[HID_IDX_BYTE14]) & (u16)0xFF);
			p_blStat->press[3] = (u16)((u16)((u16)(data[HID_IDX_BYTE15]) << 8) & (u16)0xFF00 | (u16)(data[HID_IDX_BYTE16]) & (u16)0xFF);
			p_blStat->temp     = (s8)data[HID_IDX_BYTE17];
			p_blStat->battery  = data[HID_IDX_BYTE19];

			level = p_blStat->battery * 2;
			if (level >= 260) {
				p_cb->balanceBoardBattery = WPAD_BATTERY_LEVEL_MAX;
			} else if (level < 260 && level >= 250) {
				p_cb->balanceBoardBattery = WPAD_BATTERY_LEVEL_HIGH;
			} else if (level < 250 && level >= 240) {
				p_cb->balanceBoardBattery = WPAD_BATTERY_LEVEL_MEDIUM;
			} else if (level < 240 && level >= 212) {
				p_cb->balanceBoardBattery = WPAD_BATTERY_LEVEL_LOW;
			} else {
				p_cb->balanceBoardBattery = WPAD_BATTERY_LEVEL_CRITICAL;
			}

			if (!p_cb->isSetStickOrigin) {
				p_cb->isSetStickOrigin = 1;
			}
		}

		if (!memcmp(checkBuffer, checkInvalidData, 19)) {
			if (p_status->err == WPAD_ERR_NONE) {
				p_status->err = WPAD_ERR_CORRUPTED;
			}
		}
	}
}

static void __a1_35_data_type(u8 chan, u8* data, void* rxBuffer)
{
	WPADControlBlock* p_cb = _wpdcb[chan];
	WPADStatus* p_status   = (WPADStatus*)rxBuffer;
	void* p_extStat        = rxBuffer;

	p_status->button = (u16)((u16)(((u16)((u16)(data[HID_IDX_BYTE7]) & 0xFF) | (u16)((u16)(data[HID_IDX_BYTE8] << 8) & 0xFF00)))
	                         & (u16)HID_WPAD_BUTTON_MASK);

	if (p_cb->dataFormat == WPAD_FMT_CORE || p_cb->dataFormat == WPAD_FMT_CORE_ACC || p_cb->dataFormat == WPAD_FMT_FREESTYLE
	    || p_cb->dataFormat == WPAD_FMT_FREESTYLE_ACC || p_cb->dataFormat == WPAD_FMT_CLASSIC || p_cb->dataFormat == WPAD_FMT_CLASSIC_ACC
	    || p_cb->dataFormat == WPAD_FMT_VSM) {
		p_status->err = WPAD_ERR_NONE;
	} else {
		p_status->err = WPAD_ERR_INVALID;
	}

	p_status->dev         = p_cb->devType;
	p_cb->infos.nearempty = (u8)((data[HID_IDX_BYTE7] & HID_NEAREMPTY_MASK) >> 7);
	__parse_acc_data(chan, &p_status, 0, data + HID_IDX_BYTE7, 5);

	memcpy(checkBuffer, data + HID_IDX_BYTE12, 16);
	WPADiDecode(chan, data + HID_IDX_BYTE12, 16, 0);

	if (p_cb->infos.attach) {
		if (p_cb->devType == WPAD_DEV_FREESTYLE) {
			__parse_fs_data(chan, (WPADFSStatus**)&p_extStat, 0, data + HID_IDX_BYTE12, 16);
		} else if (p_cb->devType == WPAD_DEV_CLASSIC) {
			__parse_cl_data(chan, (WPADCLStatus**)&p_extStat, p_cb->devMode, data + HID_IDX_BYTE12, 16);
		} else if (p_cb->devType == WPAD_DEV_VSM) {
			__parse_vs_data(chan, (WPADVSStatus**)&p_extStat, (u8)p_cb->dataFormat, data + HID_IDX_BYTE12, 16);
		}

		if (!memcmp(checkBuffer, checkInvalidData, 16)) {
			if (p_status->err == WPAD_ERR_NONE) {
				p_status->err = WPAD_ERR_CORRUPTED;
			}
		}
	}
}

static void __a1_36_data_type(u8 chan, u8* data, void* rxBuffer)
{
	WPADControlBlock* p_cb = _wpdcb[chan];
	WPADStatus* p_status   = (WPADStatus*)rxBuffer;

	p_status->button = (u16)((u16)(((u16)((u16)(data[HID_IDX_BYTE7]) & 0xFF) | (u16)((u16)(data[HID_IDX_BYTE8] << 8) & 0xFF00)))
	                         & (u16)HID_WPAD_BUTTON_MASK);
	p_status->err    = WPAD_ERR_INVALID;
	p_status->dev    = p_cb->devType;

	p_cb->infos.nearempty = (u8)((data[HID_IDX_BYTE7] & HID_NEAREMPTY_MASK) >> 7);
	__parse_dpd_data(chan, &p_status, p_cb->dpdFormat, data + HID_IDX_BYTE9, 10);

	memcpy(checkBuffer, data + HID_IDX_BYTE19, 9);
	WPADiDecode(chan, data + HID_IDX_BYTE19, 9, 0);

	if (p_cb->infos.attach) {
		if (!memcmp(checkBuffer, checkInvalidData, 9)) {
			if (p_status->err == WPAD_ERR_NONE) {
				p_status->err = WPAD_ERR_CORRUPTED;
			}
		}
	}
}

static void __a1_37_data_type(u8 chan, u8* data, void* rxBuffer)
{
	WPADControlBlock* p_cb = _wpdcb[chan];
	WPADStatus* p_status;
	void* p_extStat;

	p_extStat = rxBuffer;
	p_status  = (WPADStatus*)rxBuffer;

	p_status->button = (u16)((u16)(((u16)((u16)(data[HID_IDX_BYTE7]) & 0xFF) | (u16)((u16)(data[HID_IDX_BYTE8] << 8) & 0xFF00)))
	                         & (u16)HID_WPAD_BUTTON_MASK);

	if (p_cb->dataFormat == WPAD_FMT_CORE || p_cb->dataFormat == WPAD_FMT_CORE_ACC || p_cb->dataFormat == WPAD_FMT_CORE_ACC_DPD
	    || p_cb->dataFormat == WPAD_FMT_FREESTYLE || p_cb->dataFormat == WPAD_FMT_FREESTYLE_ACC
	    || p_cb->dataFormat == WPAD_FMT_FREESTYLE_ACC_DPD || p_cb->dataFormat == WPAD_FMT_CLASSIC
	    || p_cb->dataFormat == WPAD_FMT_CLASSIC_ACC || p_cb->dataFormat == WPAD_FMT_CLASSIC_ACC_DPD || p_cb->dataFormat == WPAD_FMT_GUITAR
	    || p_cb->dataFormat == WPAD_FMT_DRUM) {
		p_status->err = WPAD_ERR_NONE;
	} else {
		p_status->err = WPAD_ERR_INVALID;
	}

	p_status->dev         = p_cb->devType;
	p_cb->infos.nearempty = (u8)((data[HID_IDX_BYTE7] & HID_NEAREMPTY_MASK) >> 7);
	__parse_acc_data(chan, &p_status, 0, data + HID_IDX_BYTE7, 5);
	__parse_dpd_data(chan, &p_status, p_cb->dpdFormat, data + HID_IDX_BYTE12, 10);

	memcpy(checkBuffer, data + HID_IDX_BYTE22, 6);
	WPADiDecode(chan, data + HID_IDX_BYTE22, 6, 0);

	if (p_cb->infos.attach) {
		if (p_cb->devType == WPAD_DEV_FREESTYLE) {
			__parse_fs_data(chan, (WPADFSStatus**)&p_extStat, 0, data + HID_IDX_BYTE22, 6);
		} else if (p_cb->devType == WPAD_DEV_CLASSIC || p_cb->devType == WPAD_DEV_GUITAR || p_cb->devType == WPAD_DEV_DRUM) {
			__parse_cl_data(chan, (WPADCLStatus**)&p_extStat, p_cb->devMode, data + HID_IDX_BYTE22, 6);
		}

		if (!memcmp(checkBuffer, checkInvalidData, 6)) {
			if (p_status->err == WPAD_ERR_NONE) {
				p_status->err = WPAD_ERR_CORRUPTED;
			}
		}
	}
}

static void __a1_3d_data_type(u8 chan, u8* data, void* rxBuffer)
{
	WPADControlBlock* p_cb = _wpdcb[chan];
	WPADStatus* p_status   = (WPADStatus*)rxBuffer;
	void* p_extStat        = rxBuffer;

	if (p_cb->dataFormat == WPAD_FMT_VSM_EX) {
		p_status->err = WPAD_ERR_NONE;
	} else {
		p_status->err = WPAD_ERR_INVALID;
	}

	p_status->dev = p_cb->devType;

	memcpy(checkBuffer, data + HID_IDX_BYTE7, 21);
	WPADiDecode(chan, data + HID_IDX_BYTE7, 21, 0);

	if (p_cb->infos.attach) {
		if (p_cb->devType == WPAD_DEV_VSM) {
			__parse_vs_data(chan, (WPADVSStatus**)&p_extStat, (u8)p_cb->dataFormat, data + HID_IDX_BYTE7, 21);
		}

		if (!memcmp(checkBuffer, checkInvalidData, 21)) {
			if (p_status->err == WPAD_ERR_NONE) {
				p_status->err = WPAD_ERR_CORRUPTED;
			}
		}
	}
}

// NONMATCHING: same instructions, but retail keeps p_cb in r28 and the status pointer in r29 (ours the other way
// round) and loads accZ after the store to the first axis. Declaration order, assignment as statements and the
// accelerometer code written out in the handler were tried.
static void __a1_3e_data_type(u8 chan, u8* data, void* rxBuffer)
{
	WPADControlBlock* p_cb = _wpdcb[chan];
	WPADStatus* p_status;
	WPADStatusEx* p_exStat;
	BOOL enable;

	p_exStat = (WPADStatusEx*)rxBuffer;
	p_status = (WPADStatus*)rxBuffer;

	if (!_recv3e[chan] && !_recv3f[chan]) {
		memset(rxBuffer, 0, WPAD_RX_DATASIZE);
	}

	p_status->button = (u16)((u16)(((u16)((u16)(data[HID_IDX_BYTE7]) & 0xFF) | (u16)((u16)(data[HID_IDX_BYTE8] << 8) & 0xFF00)))
	                         & (u16)HID_WPAD_BUTTON_MASK);

	if (p_cb->dataFormat == WPAD_FMT_CORE || p_cb->dataFormat == WPAD_FMT_CORE_ACC || p_cb->dataFormat == WPAD_FMT_CORE_ACC_DPD_FULL) {
		p_status->err = WPAD_ERR_NONE;
	} else {
		p_status->err = WPAD_ERR_INVALID;
	}

	p_status->dev         = p_cb->devType;
	p_cb->infos.nearempty = 0;
	__parse_acc_data(chan, &p_status, 1, data + HID_IDX_BYTE7, 3);
	__parse_dpdex_data(chan, &p_exStat, 0, data + HID_IDX_BYTE10, 0);
	__parse_dpdex_data(chan, &p_exStat, 1, data + HID_IDX_BYTE19, 0);

	enable        = OSDisableInterrupts();
	_recv3e[chan] = 1;
	if (_recv3e[chan] && _recv3f[chan]) {
		p_status->accZ -= p_cb->conf.acc_0g.z;
		p_cb->rxBufIndex = (u8)((p_cb->rxBufIndex) ? 0 : 1);
		_recv3f[chan]    = 0;
		_recv3e[chan]    = 0;
	}
	OSRestoreInterrupts(enable);
}

// NONMATCHING: same instructions, but retail keeps p_cb in r28 and the status pointer in r29 (ours the other way
// round) and loads accZ after the store to the first axis. Declaration order, assignment as statements and the
// accelerometer code written out in the handler were tried.
static void __a1_3f_data_type(u8 chan, u8* data, void* rxBuffer)
{
	WPADControlBlock* p_cb = _wpdcb[chan];
	WPADStatus* p_status;
	WPADStatusEx* p_exStat;
	BOOL enable;

	p_exStat = (WPADStatusEx*)rxBuffer;
	p_status = (WPADStatus*)rxBuffer;

	if (!_recv3e[chan] && !_recv3f[chan]) {
		memset(rxBuffer, 0, WPAD_RX_DATASIZE);
	}

	p_status->button = (u16)((u16)(((u16)((u16)(data[HID_IDX_BYTE7]) & 0xFF) | (u16)((u16)(data[HID_IDX_BYTE8] << 8) & 0xFF00)))
	                         & (u16)HID_WPAD_BUTTON_MASK);

	if (p_cb->dataFormat == WPAD_FMT_CORE || p_cb->dataFormat == WPAD_FMT_CORE_ACC || p_cb->dataFormat == WPAD_FMT_CORE_ACC_DPD_FULL) {
		p_status->err = WPAD_ERR_NONE;
	} else {
		p_status->err = WPAD_ERR_INVALID;
	}

	p_status->dev         = p_cb->devType;
	p_cb->infos.nearempty = 0;
	__parse_acc_data(chan, &p_status, 2, data + HID_IDX_BYTE7, 3);
	__parse_dpdex_data(chan, &p_exStat, 2, data + HID_IDX_BYTE10, 0);
	__parse_dpdex_data(chan, &p_exStat, 3, data + HID_IDX_BYTE19, 0);

	enable        = OSDisableInterrupts();
	_recv3f[chan] = 1;
	if (_recv3e[chan] && _recv3f[chan]) {
		p_status->accZ -= p_cb->conf.acc_0g.z;
		p_cb->rxBufIndex = (u8)((p_cb->rxBufIndex) ? 0 : 1);
		_recv3f[chan]    = 0;
		_recv3e[chan]    = 0;
	}
	OSRestoreInterrupts(enable);
}

static void __a1_unused_report(u8 chan, u8* data, void* rxBuffer)
{
}
