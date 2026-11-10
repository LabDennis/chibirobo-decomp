#include <global.h>

#include <dolphin/ostime.h>
#include <dolphin/pad.h>
#include <kar/math.h>
#include <sysdolphin/controller.h>
#include <sysdolphin/forward.h>
#include <sysdolphin/rumble.h>

extern u32 OSDisableInterrupts(void);
extern void OSRestoreInterrupts(u32 level);
extern bool OSGetResetSwitchState(void);
extern int PADRead(PADStatus* status);
extern void PADReset(u32 mask);
extern BOOL PADRecalibrate(u32 mask);
extern BOOL PADInit(void);
extern f64 atan2(f64 y, f64 x);

extern HSD_RumbleData gPadRumbleStates[4]; /* rumble.c per port state */

HSD_PadStatus default_status_data = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 };
PadLibData default_libinfo_data   = { 0, 0, 0, 0, 0, 0, 0x2D, 8, 0, 0x1E, 0, 0, 0, 0x7F, 0, 0, 0xFF, 0, 0, 0xFF, 0, 0x7F, 0xFF, 0xFF };
PadLibData HSD_PadLibData;
HSD_PadStatus HSD_PadMasterStatus[4];
HSD_PadStatus HSD_PadCopyStatus[4];
const u32 pad_bit[4] = { PAD_CHAN0_BIT, PAD_CHAN1_BIT, PAD_CHAN2_BIT, PAD_CHAN3_BIT };

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline f32 controller_sqrtf(f32 x)
{
	volatile f32 y;

	if (x > 0.0F) {
		f64 guess = __frsqrte((f64)x);
		guess     = 0.5 * guess * __fnmsub(x, guess * guess, 3.0);
		guess     = 0.5 * guess * __fnmsub(x, guess * guess, 3.0);
		guess     = 0.5 * guess * __fnmsub(x, guess * guess, 3.0);
		y         = (f32)(x * guess);
		return y;
	}
	return x;
}

/**
 * @note Address: 0x801CC0A0
 * @note Size: 0x3C
 */
// symbols.txt calls this SetPadQueueType but it sets qtype, the name is off
void SetPadQueueType(u8 qtype)
{
	u32 intr;

	intr                 = OSDisableInterrupts();
	HSD_PadLibData.qtype = qtype;
	OSRestoreInterrupts(intr);
}

/**
 * @note Address: 0x801CC0DC
 * @note Size: 0x1C
 */
s32 HSD_PadGetResetSwitch(void)
{
	PadLibData* p = &HSD_PadLibData;

	return p->reset_switch != 0 ? TRUE : FALSE;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline void HSD_PadRawQueueShift(u8 qnum, u8* qptr)
{
	*qptr = (*qptr + 1) % qnum;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline void HSD_PadRawMerge(PADStatus* src1, PADStatus* src2, PADStatus* dst)
{
	int i;

	for (i = 0; i < 4; i++) {
		dst[i].button = src1[i].button | src2[i].button;
	}
}

/**
 * @note Address: 0x801CC0F8
 * @note Size: 0x2AC
 */
void HSD_PadRenewRawStatus(void)
{
	int i;
	u32 mask;
	PadLibData* p = &HSD_PadLibData;
	HSD_PadData* qwrite;
	PADStatus* qread;
	HSD_PadData now;

	HSD_PadRumbleInterpret();
	now.connect_mask = PADRead(now.stat);

	qwrite = &p->queue[p->qwrite];
	if (p->qcount == p->qnum) {
		switch (p->qtype) {
		case 0:
			HSD_PadRawQueueShift(p->qnum, &p->qread);
			qread = p->queue[p->qread].stat;
			if (p->qnum != 1) {
				HSD_PadRawMerge(qwrite->stat, qread, qread);
			} else {
				HSD_PadRawMerge(now.stat, qread, now.stat);
			}
			break;
		case 1:
			HSD_PadRawQueueShift(p->qnum, &p->qread);
			break;
		case 2:
			goto skip;
		}
	} else {
		p->qcount += 1;
	}

	*qwrite = now;
	HSD_PadRawQueueShift(p->qnum, &p->qwrite);

skip:
	mask = 0;
	for (i = 0; i < 4; i++) {
		if (now.stat[i].err == -1) {
			mask |= pad_bit[i];
		}
	}
	if (mask != 0) {
		PADReset(mask);
	}
	if (OSGetResetSwitchState()) {
		p->reset_switch_status = 1;
	} else if (p->reset_switch_status != 0) {
		p->reset_switch        = 1;
		p->reset_switch_status = 0;
	}
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline void HSD_PadClampCheck1(u8* val, u8 shift, u8 min, u8 max)
{
	if (*val < min) {
		*val = 0;
		return;
	}
	if (*val > max) {
		*val = max;
	}
	if (shift != 1) {
		return;
	}
	*val = *val - min;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline void HSD_PadClampCheck3(s8* x, s8* y, u8 shift, s8 min, s8 max)
{
	f32 r;

	r = controller_sqrtf((f32)*x * (f32)*x + (f32)*y * (f32)*y);

	if (r < min) {
		*y = 0;
		*x = 0;
		return;
	}
	if (r > max) {
		*x = (s8)(((f32)*x * (f32)max) / r);
		*y = (s8)(((f32)*y * (f32)max) / r);
		r  = controller_sqrtf((f32)*x * (f32)*x + (f32)*y * (f32)*y);
	}

	if (shift == 1 && r > 1e-10F) {
		*x = (s8)((f32)*x - (((f32)*x * (f32)min) / r));
		*y = (s8)((f32)*y - (((f32)*y * (f32)min) / r));
	}
}

/**
 * @note Address: 0x801CC3A4
 * @note Size: 0x7B4
 */
void HSD_PadClamp(HSD_PadStatus* mp)
{
	PadLibData* p = &HSD_PadLibData;

	switch (p->clamp_stickType) {
	case 0:
		HSD_PadClampCheck3(&mp->stickX, &mp->stickY, p->clamp_stickShift, p->clamp_stickMin, p->clamp_stickMax);
		HSD_PadClampCheck3(&mp->subStickX, &mp->subStickY, p->clamp_stickShift, p->clamp_stickMin, p->clamp_stickMax);
		break;
	default:
		break;
	}
	HSD_PadClampCheck1(&mp->analogL, p->clamp_analogLRShift, p->clamp_analogLRMin, p->clamp_analogLRMax);
	HSD_PadClampCheck1(&mp->analogR, p->clamp_analogLRShift, p->clamp_analogLRMin, p->clamp_analogLRMax);
	HSD_PadClampCheck1(&mp->analogA, p->clamp_analogABShift, p->clamp_analogABMin, p->clamp_analogABMax);
	HSD_PadClampCheck1(&mp->analogB, p->clamp_analogABShift, p->clamp_analogABMin, p->clamp_analogABMax);
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline void HSD_PadADConvertCheck1(HSD_PadStatus* mp, s8 x, s8 y, u32 up, u32 down, u32 left, u32 right)
{
	PadLibData* p = &HSD_PadLibData;
	f32 r;
	f32 a;
	f32 ha;

	r = controller_sqrtf(__fmadds((f32)x, (f32)x, (f32)y * (f32)y));

	if (0.0F == (f32)x) {
		if ((f32)y >= 0.0F) {
			a = 1.5707963267948966F;
		} else {
			a = -1.5707963267948966F;
		}
	} else {
		a = (f32)atan2(y, x);
	}

	ha = 0.5F * p->adc_angle;
	if (r < p->adc_th) {
		return;
	}

	if (a < -2.356194490192345 + ha) {
		mp->button |= left;
	}
	if (a >= -2.356194490192345 - ha && a <= -0.7853981633974483 + ha) {
		mp->button |= down;
	}
	if (a > -0.7853981633974483 - ha && a < 0.7853981633974483 + ha) {
		mp->button |= right;
	}
	if (a >= 0.7853981633974483 - ha && a <= 2.356194490192345 + ha) {
		mp->button |= up;
	}
	if (a > 2.356194490192345 - ha) {
		mp->button |= left;
	}
}

/**
 * @note Address: N/A
 * @note Size: 0x508
 */
void HSD_PadADConvert(HSD_PadStatus* mp)
{
	PadLibData* p = &HSD_PadLibData;

	switch (p->adc_type) {
	case 0:
		HSD_PadADConvertCheck1(mp, mp->stickX, mp->stickY, 0x10000, 0x20000, 0x40000, 0x80000);
		HSD_PadADConvertCheck1(mp, mp->subStickX, mp->subStickY, 0x100000, 0x200000, 0x400000, 0x800000);
		break;
	default:
		return;
	}
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline void HSD_PadScale(HSD_PadStatus* mp)
{
	PadLibData* p = &HSD_PadLibData;

	mp->nml_stickX    = (f32)mp->stickX / (f32)p->scale_stick;
	mp->nml_stickY    = (f32)mp->stickY / (f32)p->scale_stick;
	mp->nml_subStickX = (f32)mp->subStickX / (f32)p->scale_stick;
	mp->nml_subStickY = (f32)mp->subStickY / (f32)p->scale_stick;
	mp->nml_analogL   = (f32)mp->analogL / (f32)p->scale_analogLR;
	mp->nml_analogR   = (f32)mp->analogR / (f32)p->scale_analogLR;
	mp->nml_analogA   = (f32)mp->analogA / (f32)p->scale_analogAB;
	mp->nml_analogB   = (f32)mp->analogB / (f32)p->scale_analogAB;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
// this revision works on the dpad bits, 0xC is up and down, 0x3 is left and right,
// older hsd did the same thing on the stick direction bits
static inline void HSD_PadCrossDir(HSD_PadStatus* mp)
{
	PadLibData* p = &HSD_PadLibData;

	switch (p->cross_dir) {
	case 0:
		break;
	case 1:
		if ((mp->button & 0xC) == 0) {
			return;
		}
		mp->button = mp->button & ~0x3;
		return;
	case 2:
		if ((mp->button & 0x3) == 0) {
			return;
		}
		mp->button = mp->button & ~0xC;
		return;
	case 3:
		if ((mp->button & 0xC) != 0) {
			if ((mp->button & 0x3) != 0) {
				if (mp->cross_dir == 1) {
					mp->button = mp->button & ~0x3;
					return;
				}
				mp->button = mp->button & ~0xC;
				return;
			} else {
				mp->cross_dir = 1;
				return;
			}
		}
		if ((mp->button & 0x3) != 0) {
			mp->cross_dir = 2;
			return;
		}
	}
}

/**
 * @note Address: 0x801CD060
 * @note Size: 0x47C
 */
void HSD_PadRenewMasterStatus(void)
{
	PadLibData* p;
	HSD_PadStatus* mp;
	PADStatus* qread;
	u32 mask;
	s32 repeat_count;
	int i;
	u32 intr;

	p    = &HSD_PadLibData;
	mp   = &HSD_PadMasterStatus[0];
	intr = OSDisableInterrupts();
	if (p->qcount != 0) {
		qread = p->queue[p->qread].stat;
		mask  = p->queue[p->qread].connect_mask;
		HSD_PadRawQueueShift(p->qnum, &p->qread);
		p->qcount -= 1;

		for (i = 0; i < 4; i++, mp++, qread++) {
			mp->last_button = mp->button;
			mp->err         = qread->err;
			mp->_41         = (mask & pad_bit[i]) != 0;
			if (mp->err == 0) {
				mp->button    = qread->button;
				mp->stickX    = qread->stickX;
				mp->stickY    = qread->stickY;
				mp->subStickX = qread->substickX;
				mp->subStickY = qread->substickY;
				mp->analogL   = qread->triggerLeft;
				mp->analogR   = qread->triggerRight;
				mp->analogA   = qread->analogA;
				mp->analogB   = qread->analogB;
				HSD_PadClamp(mp);
				HSD_PadADConvert(mp);
				HSD_PadScale(mp);
				HSD_PadCrossDir(mp);
			} else if (mp->err == -3) {
				mp->err = 0;
			} else {
				mp->button        = 0;
				mp->subStickY     = 0;
				mp->subStickX     = 0;
				mp->stickY        = 0;
				mp->stickX        = 0;
				mp->analogB       = 0;
				mp->analogA       = 0;
				mp->analogR       = 0;
				mp->analogL       = 0;
				mp->nml_subStickY = 0.0F;
				mp->nml_subStickX = 0.0F;
				mp->nml_stickY    = 0.0F;
				mp->nml_stickX    = 0.0F;
				mp->nml_analogB   = 0.0F;
				mp->nml_analogA   = 0.0F;
				mp->nml_analogR   = 0.0F;
				mp->nml_analogL   = 0.0F;
			}
			mp->trigger = mp->button & (mp->last_button ^ mp->button);
			mp->release = mp->last_button & (mp->last_button ^ mp->button);
			if (mp->last_button ^ mp->button) {
				mp->repeat       = mp->trigger;
				mp->repeat_count = p->repeat_start;
			} else {
				repeat_count     = mp->repeat_count - 1;
				mp->repeat_count = repeat_count;
				if (repeat_count != 0) {
					mp->repeat = 0;
				} else {
					mp->repeat       = mp->button;
					mp->repeat_count = p->repeat_interval;
				}
			}
		}
	}
	OSRestoreInterrupts(intr);
}

/**
 * @note Address: 0x801CD4DC
 * @note Size: 0x1A0
 */
void HSD_PadRenewCopyStatus(void)
{
	PadLibData* p = &HSD_PadLibData;
	HSD_PadStatus* mp;
	HSD_PadStatus* cp;
	s32 repeat_count;
	int i;

	mp = HSD_PadMasterStatus;
	cp = HSD_PadCopyStatus;
	for (i = 0; i < 4; i++, mp++, cp++) {
		cp->last_button = cp->button;
		cp->err         = mp->err;
		cp->_41         = mp->_41;
		if (cp->err == 0) {
			cp->button        = mp->button;
			cp->stickX        = mp->stickX;
			cp->stickY        = mp->stickY;
			cp->subStickX     = mp->subStickX;
			cp->subStickY     = mp->subStickY;
			cp->analogL       = mp->analogL;
			cp->analogR       = mp->analogR;
			cp->analogA       = mp->analogA;
			cp->analogB       = mp->analogB;
			cp->nml_stickX    = mp->nml_stickX;
			cp->nml_stickY    = mp->nml_stickY;
			cp->nml_subStickX = mp->nml_subStickX;
			cp->nml_subStickY = mp->nml_subStickY;
			cp->nml_analogL   = mp->nml_analogL;
			cp->nml_analogR   = mp->nml_analogR;
			cp->nml_analogA   = mp->nml_analogA;
			cp->nml_analogB   = mp->nml_analogB;
		} else {
			cp->button        = 0;
			cp->subStickY     = 0;
			cp->subStickX     = 0;
			cp->stickY        = 0;
			cp->stickX        = 0;
			cp->analogB       = 0;
			cp->analogA       = 0;
			cp->analogR       = 0;
			cp->analogL       = 0;
			cp->nml_subStickY = 0.0F;
			cp->nml_subStickX = 0.0F;
			cp->nml_stickY    = 0.0F;
			cp->nml_stickX    = 0.0F;
			cp->nml_analogB   = 0.0F;
			cp->nml_analogA   = 0.0F;
			cp->nml_analogR   = 0.0F;
			cp->nml_analogL   = 0.0F;
		}
		cp->trigger = cp->button & (cp->last_button ^ cp->button);
		cp->release = cp->last_button & (cp->last_button ^ cp->button);
		if (cp->last_button ^ cp->button) {
			cp->repeat       = cp->trigger;
			cp->repeat_count = p->repeat_start;
		} else {
			repeat_count     = cp->repeat_count - 1;
			cp->repeat_count = repeat_count;
			if (repeat_count != 0) {
				cp->repeat = 0;
			} else {
				cp->repeat       = cp->button;
				cp->repeat_count = p->repeat_interval;
			}
		}
	}
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline void HSD_PadFlushQueueThrowaway(void)
{
	PadLibData* p = &HSD_PadLibData;
	u32 intr;

	intr      = OSDisableInterrupts();
	p->qread  = p->qwrite;
	p->qcount = 0;
	OSRestoreInterrupts(intr);
}

/**
 * @note Address: 0x801CD67C
 * @note Size: 0x98
 */
void HSD_PadReset(void)
{
	PadLibData* p;
	u32 intr;
	int i;

	p    = &HSD_PadLibData;
	intr = OSDisableInterrupts();

	HSD_PadRumbleRemoveAll();
	for (i = 0; i < 4; i++) {
		HSD_PadRumbleOffN(i);
	}
	HSD_PadFlushQueueThrowaway();
	PADRecalibrate(0xF0000000);
	p->reset_switch = 0;

	OSRestoreInterrupts(intr);
}

/**
 * @note Address: 0x801CD714
 * @note Size: 0x26C
 */
void HSD_PadInit(u8 qnum, HSD_PadData* queue, u16 nb_list, HSD_PadRumbleListData* listdatap)
{
	int i;
	PadLibData* p = &HSD_PadLibData;

	*p       = default_libinfo_data;
	p->qnum  = qnum;
	p->queue = queue;
	HSD_PadRumbleInit(nb_list, listdatap);
	for (i = 0; i < 4; i++) {
		HSD_PadMasterStatus[i] = default_status_data;
		HSD_PadCopyStatus[i]   = default_status_data;
	}
	PADInit();
}

/**
 * @note Address: 0x801CD980
 * @note Size: 0x4C
 */
u8 GetPadRumbleStatus(u8 no)
{
	HSD_RumbleData* rp = &gPadRumbleStates[no];
	u32 intr;
	u8 status;

	intr   = OSDisableInterrupts();
	status = rp->status;
	OSRestoreInterrupts(intr);
	return status;
}
