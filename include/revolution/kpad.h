#ifndef KPAD_H
#define KPAD_H

#include "revolution/wpad.h"
#include "revolution/mtx.h"

#ifdef __cplusplus
extern "C" {
#endif

#define KPAD_BUTTON_MASK 0x0000ffff
#define KPAD_BUTTON_RPT  0x80000000
// the remote's own buttons, without the nunchuk's Z and C and the unused bits
#define KPAD_BUTTON_CORE_MASK 0x00009f1f

#define KPAD_BTN_RPT_TIME_MAX 20000
#define KPAD_BTN_NO_RPT_DELAY 40000

#define KPAD_CMOS_RESO_WX 128
#define KPAD_CMOS_RESO_WY 96
#define KPAD_CMOS_RESO_CX (KPAD_CMOS_RESO_WX / 2)
#define KPAD_CMOS_RESO_CY (KPAD_CMOS_RESO_WY / 2)

#define KPAD_DPD_RESO_WX WPAD_DPD_IMG_RESO_WX
#define KPAD_DPD_RESO_WY WPAD_DPD_IMG_RESO_WY
#define KPAD_DPD_RESO_CX (KPAD_DPD_RESO_WX / 2)
#define KPAD_DPD_RESO_CY (KPAD_DPD_RESO_WY / 2)

#define KPAD_USE_OBJECTS 2

typedef struct Vec2 {
	f32 x;
	f32 y;
} Vec2;

typedef enum KPADPlayMode { KPAD_PLAY_MODE_LOOSE = 0, KPAD_PLAY_MODE_TIGHT } KPADPlayMode;

typedef union KPADEXStatus {
	struct {
		Vec2 stick;
		Vec acc;
		f32 acc_value;
		f32 acc_speed;
	} fs;

	struct {
		u32 hold;
		u32 trig;
		u32 release;
		Vec2 lstick;
		Vec2 rstick;
		f32 ltrigger;
		f32 rtrigger;
	} cl;

	struct {
		f64 tgc_weight;
		f64 weight[4];
		f64 weight_ave[4];
		s32 weight_err;
		s32 tgc_weight_err;
	} bl;
} KPADEXStatus;

typedef struct KPADStatus {
	/* 0x00 */ u32 hold;
	/* 0x04 */ u32 trig;
	/* 0x08 */ u32 release;
	/* 0x0C */ Vec acc;
	/* 0x18 */ f32 acc_value;
	/* 0x1C */ f32 acc_speed;
	/* 0x20 */ Vec2 pos;
	/* 0x28 */ Vec2 vec;
	/* 0x30 */ f32 speed;
	/* 0x34 */ Vec2 horizon;
	/* 0x3C */ Vec2 hori_vec;
	/* 0x44 */ f32 hori_speed;
	/* 0x48 */ f32 dist;
	/* 0x4C */ f32 dist_vec;
	/* 0x50 */ f32 dist_speed;
	/* 0x54 */ Vec2 acc_vertical;
	/* 0x5C */ u8 dev_type;
	/* 0x5D */ s8 wpad_err;
	/* 0x5E */ s8 dpd_valid_fg;
	/* 0x5F */ u8 data_format;
	/* 0x60 */ KPADEXStatus ex_status;
} KPADStatus;

typedef struct KPADUnifiedWpadStatus {
	union {
		WPADStatus core;
		WPADFSStatus fs;
		WPADCLStatus cl;
		WPADTRStatus tr;
		WPADBLStatus bl;
	} u;
	u8 fmt;
	u8 padding;
} KPADUnifiedWpadStatus;

typedef struct {
	Vec2 center;
	s8 error_fg;
	s8 state_fg;
	s8 _fg_1;
	s8 _fg_2;
} KPADObject;

typedef union {
	KPADStatus k;
	KPADUnifiedWpadStatus w;
} KPADTmpStatus;

typedef void (*KPADControlDpdCallback)(s32 chan, s32 reason);
typedef WPADCallback KPADCallback;

#define KPAD_RING_BUFS 16

#define KPAD_READ_ERR_NONE          0
#define KPAD_READ_ERR_NO_DATA       -1
#define KPAD_READ_ERR_NO_CONTROLLER -2
#define KPAD_READ_ERR_SETUP         -3
#define KPAD_READ_ERR_LOCKED        -4
#define KPAD_READ_ERR_INIT          -5

// Fields without a counterpart in Petari's 2008 header are named after what the code does with them.
typedef struct {
	/* 0x000 */ KPADStatus status;

	/* 0x0B0 */ f32 pos_play_radius;
	/* 0x0B4 */ f32 pos_sensitivity;

	/* 0x0B8 */ f32 hori_play_radius;
	/* 0x0BC */ f32 hori_sensitivity;

	/* 0x0C0 */ f32 dist_play_radius;
	/* 0x0C4 */ f32 dist_sensitivity;

	/* 0x0C8 */ f32 acc_play_radius;
	/* 0x0CC */ f32 acc_sensitivity;
	/* 0x0D0 */ f32 dist_org;
	/* 0x0D4 */ Vec2 accXY_nrm_hori;
	/* 0x0DC */ Vec2 sec_nrm_hori;
	/* 0x0E4 */ Vec2 center_org;
	/* 0x0EC */ f32 dpd2pos_scale;
	/* 0x0F0 */ KPADObject kobj_sample[WPAD_DPD_MAX_OBJECTS];
	/* 0x120 */ KPADObject kobj_regular[KPAD_USE_OBJECTS];
	/* 0x138 */ s16 valid_objs;
	/* 0x13A */ u8 bufIdx;
	/* 0x13B */ u8 bufCount;
	/* 0x13C */ KPADUnifiedWpadStatus uniRingBuf[KPAD_RING_BUFS];
	/* 0x4BC */ KPADUnifiedWpadStatus* uniRingBufEx; // the application's share, behind the 16 own entries
	/* 0x4C0 */ u32 uniRingBufExLen;
	/* 0x4C4 */ f32 sec_length;
	/* 0x4C8 */ Vec2 sec_nrm;
	/* 0x4D0 */ f32 sec_dist;
	/* 0x4D4 */ f32 trust_sec_length;
	/* 0x4D8 */ Vec hard_acc;
	/* 0x4E4 */ Vec2 obj_horizon;
	/* 0x4EC */ Vec2 acc_horizon;
	/* 0x4F4 */ Vec2 ah_circle_pos;
	/* 0x4FC */ u16 ah_circle_ct;
	/* 0x4FE */ u8 dpd_valid2_ct;
	/* 0x4FF */ u8 readLocked;
	/* 0x500 */ u16 btn_repeat_time;
	/* 0x502 */ u16 btn_repeat_next;
	/* 0x504 */ u16 btn_repeat_delay;
	/* 0x506 */ u16 btn_repeat_pulse;
	/* 0x508 */ u16 btn_cl_repeat_time;
	/* 0x50A */ u16 btn_cl_repeat_next;
	/* 0x50C */ KPADControlDpdCallback dpd_ctrl_callback;
	/* 0x510 */ f32 acc_scale_x;
	/* 0x514 */ f32 acc_scale_y;
	/* 0x518 */ f32 acc_scale_z;
	/* 0x51C */ f32 fs_acc_scale_x;
	/* 0x520 */ f32 fs_acc_scale_y;
	/* 0x524 */ f32 fs_acc_scale_z;
	/* 0x528 */ Vec2 kobj_frame_min;
	/* 0x530 */ Vec2 kobj_frame_max;
	/* 0x538 */ f32 err_dist_speed_1;
	/* 0x53C */ f32 err_dist_speedM_1;
	/* 0x540 */ f32 ah_circle_radius2;
	/* 0x544 */ f32 dist_vv1;
	/* 0x548 */ f32 err_dist_min;
	/* 0x54C */ WPADSamplingCallback appSamplingCallback;
	/* 0x550 */ WPADConnectCallback appConnectCallback;
	/* 0x554 */ u8 resetReq;
	/* 0x555 */ u8 exResetReq;
	/* 0x556 */ u8 dpdEnabled;
	/* 0x557 */ u8 dpdState; // what WPADIsDpdEnabled said when the last DPD command came back
	/* 0x558 */ u8 dpdIssued;
	/* 0x559 */ u8 dpdCmd;
	/* 0x55A */ u8 dpdPreCallbackDone;
	/* 0x55B */ u8 dpdPostCallbackDone;
	/* 0x55C */ u8 aimReq;
	/* 0x55D */ u8 aimEnabled;
	/* 0x55E */ u8 fsAccRevise;
	/* 0x55F */ u8 btnProcMode; // 1: buttons are worked out for every sample, otherwise once per KPADRead
	/* 0x560 */ KPADPlayMode pos_play_mode;
	/* 0x564 */ KPADPlayMode hori_play_mode;
	/* 0x568 */ KPADPlayMode dist_play_mode;
	/* 0x56C */ KPADPlayMode acc_play_mode;
	// balance board: the steps from power-on to a usable weight, and the averages they collect
	/* 0x570 */ u8 wbcIssued;
	/* 0x571 */ u8 wbcPowerOn;
	/* 0x572 */ u8 wbcCalibrated;
	/* 0x573 */ u8 wbcTempWait;
	/* 0x574 */ u8 wbcTempUpdated;
	/* 0x575 */ u8 wbcTempStable;
	/* 0x576 */ u8 wbcZeroSet;
	/* 0x577 */ u8 wbcReady;
	/* 0x578 */ u16 wbcZeroCount;
	/* 0x57A */ u16 wbcAveCount;
	/* 0x580 */ f64 wbcZero[4];
	/* 0x5A0 */ f64 wbcAve[4];
} KPADInsideStatus;

void KPADInit(void);
void KPADInitEx(KPADUnifiedWpadStatus* uniRingBufs, u32 length);

void KPADSetFSStickClamp(s8 min, s8 max);
void KPADSetBtnRepeat(s32 chan, f32 delay_sec, f32 pulse_sec);
void KPADSetObjInterval(f32 interval);
void KPADSetAccParam(s32 chan, f32 play_radius, f32 sensitivity);
void KPADSetPosParam(s32 chan, f32 play_radius, f32 sensitivity);
void KPADSetHoriParam(s32 chan, f32 play_radius, f32 sensitivity);
void KPADSetDistParam(s32 chan, f32 play_radius, f32 sensitivity);
void KPADSetSensorHeight(s32 chan, f32 level);
s32 KPADCalibrateDPD(s32 chan);
void KPADEnableAimingMode(s32 chan);
void KPADDisableStickCrossClamp(void);

void KPADReset(void);
s32 KPADRead(s32 chan, KPADStatus samplingBufs[], u32 length);
void KPADGetUnifiedWpadStatus(s32 chan, KPADUnifiedWpadStatus* dst, u32 count);

void KPADSetConnectCallback(s32 chan, WPADConnectCallback callback);
void KPADEnableDPD(s32 chan);
void KPADDisableDPD(s32 chan);

extern KPADInsideStatus inside_kpads[];

#ifdef __cplusplus
}
#endif

#endif // KPAD_H
