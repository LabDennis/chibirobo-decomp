#include <global.h>
#include <sysdolphin/fobj.h>
#include <sysdolphin/spline.h>

#include <string.h>

enum {
	FOBJ_STOPPED = 0,
	FOBJ_INTERPOLATE = 4,
	FOBJ_RESUME = 5,
	FOBJ_END = 6,
};

enum {
	FOBJ_STATE_MASK = 0x0F,
	FOBJ_CONTROL_MASK = 0xF0,
	FOBJ_LINEAR_SLOPE_DIRTY = 0x20,
	FOBJ_KEY_PENDING = 0x40,
	FOBJ_KEY_READY = 0x80,
};

enum {
	FOBJ_OPCODE_MASK = 0x0F,
	FOBJ_FRACTION_BITS_MASK = 0x1F,
	FOBJ_FRACTION_TYPE_MASK = 0xE0,
	FOBJ_CONTINUATION_BIT = 0x80,
	FOBJ_CONTINUATION_MASK = 0x7F,
};

static HSD_ObjAllocData fobj_alloc_data;

/**
 * @note Address: 0x801C1DD0
 * @note Size: 0x30
 */
void HSD_FObjInitAllocData(void)
{
	HSD_ObjAllocInit(&fobj_alloc_data, sizeof(HSD_FObj), 4);
}

/**
 * @note Address: 0x801C1E00
 * @note Size: 0x28
 */
void HSD_FObjRemove(HSD_FObj* fobj)
{
	if (!fobj) {
		return;
	}

	HSD_FObjFree(fobj);
}

/**
 * @note Address: 0x801C1E28
 * @note Size: 0x124
 */
void HSD_FObjRemoveAll(HSD_FObj* fobj)
{
	if (fobj == NULL) {
		return;
	}
	HSD_FObjRemoveAll(fobj->next);
	HSD_FObjRemove(fobj);
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline u32 HSD_FObjSetState(HSD_FObj* fobj, u32 state)
{
	if (fobj) {
		fobj->flags = (fobj->flags & FOBJ_CONTROL_MASK) | (state & FOBJ_STATE_MASK);
	}
	return state;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline u32 HSD_FObjGetState(HSD_FObj* fobj)
{
	return fobj == NULL ? FOBJ_STOPPED : fobj->flags & FOBJ_STATE_MASK;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline void HSD_FObjReqAnim(HSD_FObj* fobj, f32 startframe)
{
	if (fobj == NULL) {
		return;
	}

	fobj->ad       = fobj->ad_head;
	fobj->time     = (f32)fobj->startframe + startframe;
	fobj->op       = HSD_A_OP_NONE;
	fobj->op_intrp = HSD_A_OP_NONE;
	fobj->flags &= ~FOBJ_KEY_PENDING;
	fobj->nb_pack = 0;
	fobj->fterm   = 0;
	fobj->p0      = 0.f;
	fobj->p1      = 0.f;
	fobj->d0      = 0.f;
	fobj->d1      = 0.f;
	HSD_FObjSetState(fobj, FOBJ_LOAD_DATA0);
}

/**
 * @note Address: 0x801C1F4C
 * @note Size: 0xA4
 */
void HSD_FObjReqAnimAll(HSD_FObj* fobj, f32 startframe)
{
	HSD_FObj* fp;

	if (fobj == NULL) {
		return;
	}

	for (fp = fobj; fp != NULL; fp = fp->next) {
		HSD_FObjReqAnim(fp, startframe);
	}
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline void FObj_FlushKeyData(HSD_FObj* fobj, void* obj, HSD_ObjUpdateFunc obj_update, f32 rate)
{
	if (fobj->op_intrp == HSD_A_OP_KEY) {
		HSD_FObjInterpretAnim(fobj, obj, obj_update, rate);
	}
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline void HSD_FObjStopAnim(HSD_FObj* fobj, void* obj, HSD_ObjUpdateFunc obj_update, f32 rate)
{
	if (fobj == NULL) {
		return;
	}

	FObj_FlushKeyData(fobj, obj, obj_update, rate);
	HSD_FObjSetState(fobj, FOBJ_STOPPED);
}

/**
 * @note Address: 0x801C1FF0
 * @note Size: 0x98
 */
void HSD_FObjStopAnimAll(HSD_FObj* fobj, void* obj, HSD_ObjUpdateFunc obj_update, f32 rate)
{
	for (; fobj != NULL; fobj = fobj->next) {
		HSD_FObjStopAnim(fobj, obj, obj_update, rate);
	}
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline f32 parseFloat(u8** pos, u8 frac)
{
	union {
		f32 f;
		u32 bits;
	} u;
	f32 numer;
	s32 denom;

	if (frac == HSD_A_FRAC_FLOAT) {
		u.bits = *(*pos)++;
		u.bits |= (u32)*(*pos)++ << 8;
		u.bits |= (u32)*(*pos)++ << 16;
		u.bits |= (u32)*(*pos)++ << 24;
		return u.f;
	}

	denom = (s32)(1u << (frac & FOBJ_FRACTION_BITS_MASK));
	switch (frac & FOBJ_FRACTION_TYPE_MASK) {
	case HSD_A_FRAC_S8:
		numer = (s8)(*pos)[0];
		*pos += 1;
		break;
	case HSD_A_FRAC_U8:
		numer = (*pos)[0];
		*pos += 1;
		break;
	case HSD_A_FRAC_S16:
		numer = (s32)(((u32)(s8)(*pos)[1] << 8) | (*pos)[0]);
		*pos += 2;
		break;
	case HSD_A_FRAC_U16:
		numer = ((*pos)[1] << 8) | (*pos)[0];
		*pos += 2;
		break;
	default:
		return 0.0f;
	}
	return numer / denom;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline u8 parseOpCode(const u8* pos)
{
	return *pos & FOBJ_OPCODE_MASK;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline u32 parsePackInfo(u8** adp)
{
	u8 d;
	u32 nb_pack;
	s32 shift;

	// The first byte carries the opcode and the low three bits of the biased count.
	d       = *(*adp)++;
	nb_pack = ((d >> 4) & 7) + 1;
	shift   = 3;
	if (!(d & FOBJ_CONTINUATION_BIT)) {
		return nb_pack;
	}
	do {
		d = *(*adp)++;
		nb_pack += (u32)(d & FOBJ_CONTINUATION_MASK) << shift;
		shift += 7;
	} while (d & FOBJ_CONTINUATION_BIT);
	return nb_pack;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline void FObjLaunchKeyData(HSD_FObj* fobj)
{
	if ((fobj->flags & FOBJ_KEY_PENDING) != 0) {
		fobj->op_intrp = fobj->op;
		fobj->flags &= ~FOBJ_KEY_PENDING;
		fobj->flags |= FOBJ_KEY_READY;
		fobj->p0 = fobj->p1;
	}
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline u32 parseWait(u8** adp)
{
	u8 d;
	u32 wait  = 0;
	s32 shift = 0;

	do {
		d = *(*adp)++;
		wait |= (u32)(d & FOBJ_CONTINUATION_MASK) << shift;
		shift += 7;
	} while (d & FOBJ_CONTINUATION_BIT);

	return wait;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static u32 FObjLoadWait(HSD_FObj* fobj)
{
	u32 st = HSD_FObjGetState(fobj);
	assert_line(0x16C, st == FOBJ_LOAD_WAIT);

	if ((unsigned)(fobj->ad - fobj->ad_head) >= fobj->length) {
		return FOBJ_END;
	}

	fobj->fterm = parseWait(&fobj->ad);
	fobj->flags |= FOBJ_LINEAR_SLOPE_DIRTY;
	return HSD_FObjSetState(fobj, FOBJ_LOAD_DATA);
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline u32 FObjLoadValue(HSD_FObj* fobj, u32 line)
{
	u32 st = HSD_FObjGetState(fobj);
	assert_line(line, st == FOBJ_LOAD_DATA0 || st == FOBJ_LOAD_DATA);

	fobj->p0 = fobj->p1;
	fobj->p1 = parseFloat(&fobj->ad, fobj->frac_value);
	if (fobj->op_intrp != HSD_A_OP_SLP) {
		fobj->d0 = fobj->d1;
		fobj->d1 = 0.0F;
	}

	return HSD_FObjSetState(fobj, st == FOBJ_LOAD_DATA0 ? FOBJ_LOAD_WAIT : FOBJ_INTERPOLATE);
}


/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline u32 FObjAnimSPL0(HSD_FObj* fobj)
{
	u32 st = HSD_FObjGetState(fobj);
	assert_line(0x1A7, st == FOBJ_LOAD_DATA0 || st == FOBJ_LOAD_DATA);

	fobj->p0 = fobj->p1;
	fobj->d0 = fobj->d1;
	fobj->p1 = parseFloat(&fobj->ad, fobj->frac_value);
	fobj->d1 = 0.0F;

	return HSD_FObjSetState(fobj, st == FOBJ_LOAD_DATA0 ? FOBJ_LOAD_WAIT : FOBJ_INTERPOLATE);
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline u32 FObjAnimSPL(HSD_FObj* fobj)
{
	u32 st = HSD_FObjGetState(fobj);
	assert_line(0x1B9, st == FOBJ_LOAD_DATA0 || st == FOBJ_LOAD_DATA);

	fobj->p0 = fobj->p1;
	fobj->p1 = parseFloat(&fobj->ad, fobj->frac_value);
	fobj->d0 = fobj->d1;
	fobj->d1 = parseFloat(&fobj->ad, fobj->frac_slope);

	return HSD_FObjSetState(fobj, st == FOBJ_LOAD_DATA0 ? FOBJ_LOAD_WAIT : FOBJ_INTERPOLATE);
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline u32 FObjAnimSLP(HSD_FObj* fobj)
{
	u32 st = HSD_FObjGetState(fobj);
	assert_line(0x1CC, st == FOBJ_LOAD_DATA0 || st == FOBJ_LOAD_DATA);

	fobj->d0 = fobj->d1;
	fobj->d1 = parseFloat(&fobj->ad, fobj->frac_slope);

	return HSD_FObjGetState(fobj);
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline u32 FObjAnimKey(HSD_FObj* fobj)
{
	u32 st = HSD_FObjGetState(fobj);
	assert_line(0x1E9, st == FOBJ_LOAD_DATA0 || st == FOBJ_LOAD_DATA);

	FObjLaunchKeyData(fobj);
	fobj->p1 = parseFloat(&fobj->ad, fobj->frac_value);
	fobj->flags |= FOBJ_KEY_PENDING;

	return HSD_FObjSetState(fobj, st == FOBJ_LOAD_DATA0 ? FOBJ_LOAD_WAIT : FOBJ_INTERPOLATE);
}

/**
 * @note Address: 0x801C2088
 * @note Size: 0x10D0
 */
u32 FObjLoadData(HSD_FObj* fobj)
{
	if ((unsigned)(fobj->ad - fobj->ad_head) >= fobj->length) {
		return FOBJ_END;
	}

	fobj->op_intrp = fobj->op;
	if (fobj->nb_pack == 0) {
		fobj->op      = parseOpCode(fobj->ad);
		fobj->nb_pack = parsePackInfo(&fobj->ad);
	}

	fobj->nb_pack -= 1;

	switch (fobj->op) {
	case HSD_A_OP_CON:
		return FObjLoadValue(fobj, 0x17F);

	case HSD_A_OP_LIN:
		return FObjLoadValue(fobj, 0x193);

	case HSD_A_OP_SPL0:
		return FObjAnimSPL0(fobj);

	case HSD_A_OP_SPL:
		return FObjAnimSPL(fobj);

	case HSD_A_OP_SLP:
		return FObjAnimSLP(fobj);

	case HSD_A_OP_KEY:
		return FObjAnimKey(fobj);

	default:
		return FOBJ_STOPPED;
	}
}

static f32 FObjInterpolateSpline(HSD_FObj* fobj)
{
	return splGetHelmite(1.0 / fobj->fterm, fobj->time, fobj->p0, fobj->p1, fobj->d0, fobj->d1);
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline void FObjUpdateAnim(HSD_FObj* fobj, void* obj, HSD_ObjUpdateFunc obj_update)
{
	HSD_ObjData fobjdata;

	if (obj_update == NULL) {
		return;
	}
	switch (fobj->op_intrp) {
	case HSD_A_OP_KEY:
		if (fobj->flags & FOBJ_KEY_READY) {
			fobjdata.fv = fobj->p0;
			fobj->flags &= ~FOBJ_KEY_READY;
		} else {
			return;
		}
		break;
	case HSD_A_OP_CON:
		fobjdata.fv = fobj->time >= fobj->fterm ? fobj->p1 : fobj->p0;
		break;
	case HSD_A_OP_LIN:
		if (fobj->flags & FOBJ_LINEAR_SLOPE_DIRTY) {
			fobj->flags &= ~FOBJ_LINEAR_SLOPE_DIRTY;
			if (fobj->fterm != 0) {
				fobj->d0 = (fobj->p1 - fobj->p0) / fobj->fterm;
			} else {
				fobj->d0 = 0;
				fobj->p0 = fobj->p1;
			}
		}
		fobjdata.fv = fobj->d0 * fobj->time + fobj->p0;
		break;
	case HSD_A_OP_SPL0:
	case HSD_A_OP_SPL:
	case HSD_A_OP_SLP:
		if (fobj->fterm != 0) {
			fobjdata.fv = FObjInterpolateSpline(fobj);
		} else {
			fobjdata.fv = fobj->p1;
		}
		break;
	default:
		break;
	}
	obj_update(obj, fobj->obj_type, &fobjdata);
}

/**
 * @note Address: 0x801C3158
 * @note Size: 0x6D4
 */
void HSD_FObjInterpretAnim(HSD_FObj* fobj, void* obj, HSD_ObjUpdateFunc obj_update, f32 rate)
{
	f32 fterm;
	u32 state;

	fterm = 0.0F;
	state = fobj != NULL ? HSD_FObjGetState(fobj) : FOBJ_STOPPED;
	if (state == FOBJ_STOPPED) {
		return;
	}
	fobj->time += rate;
	if (fobj->time < 0.0) {
		return;
	}

	for (;;) {
		switch (state) {
		case FOBJ_END: {
			fobj->time += fterm;
			FObjLaunchKeyData(fobj);
			FObjUpdateAnim(fobj, obj, obj_update);
			return;
		}
		case FOBJ_LOAD_DATA0:
		case FOBJ_LOAD_DATA: {
			state = FObjLoadData(fobj);
			break;
		}
		case FOBJ_LOAD_WAIT: {
			if ((fobj->flags & FOBJ_KEY_READY) != 0) {
				FObjUpdateAnim(fobj, obj, obj_update);
			}
			state = FObjLoadWait(fobj);
			break;
		}
		case FOBJ_INTERPOLATE: {
			if (fobj->fterm <= fobj->time) {
				fterm = fobj->fterm;
				fobj->time -= fobj->fterm;
				HSD_FObjSetState(fobj, state = FOBJ_LOAD_WAIT);
				break;
			}
			FObjUpdateAnim(fobj, obj, obj_update);
			HSD_FObjSetState(fobj, FOBJ_RESUME);
			return;
		}
		case FOBJ_RESUME: {
			HSD_FObjSetState(fobj, state = FOBJ_INTERPOLATE);
			break;
		}
		case FOBJ_STOPPED:
			return;
		}
	}
}

/**
 * @note Address: 0x801C382C
 * @note Size: 0x70
 */
void HSD_FObjInterpretAnimAll(HSD_FObj* fobj, void* obj, HSD_ObjUpdateFunc obj_update, f32 rate)
{
	HSD_FObj* fobjNew = fobj;
	while (fobjNew != NULL) {
		HSD_FObjInterpretAnim(fobjNew, obj, obj_update, rate);
		fobjNew = fobjNew->next;
	}
}

/**
 * @note Address: 0x801C389C
 * @note Size: 0x228
 */
HSD_FObj* HSD_FObjLoadDesc(HSD_FObjDesc* desc)
{
	if (desc != NULL) {
		HSD_FObj* fobj   = HSD_FObjAlloc();
		fobj->next       = HSD_FObjLoadDesc(desc->next);
		fobj->startframe = desc->startframe;
		fobj->obj_type   = desc->type;
		fobj->frac_value = desc->frac_value;
		fobj->frac_slope = desc->frac_slope;
		fobj->ad_head    = desc->ad;
		fobj->length     = desc->length;
		fobj->flags      = 0;
		return fobj;
	}
	return NULL;
}

/**
 * @note Address: 0x801C3AC4
 * @note Size: 0x5C
 */
HSD_FObj* HSD_FObjAlloc(void)
{
	HSD_FObj* new = HSD_ObjAlloc(&fobj_alloc_data);
	assert_line(0x2F3, new);
	memset(new, 0, sizeof(HSD_FObj));
	return new;
}

/**
 * @note Address: 0x801C3B20
 * @note Size: 0x2C
 */
void HSD_FObjFree(HSD_FObj* fobj)
{
	HSD_ObjFree(&fobj_alloc_data, fobj);
}

/**
 * @note Address: N/A
 * @note Size: 0x18
 */
u16 HSD_FObjGetFlags(HSD_FObj* fobj)
{
	if (fobj != NULL) {
		return fobj->length >> 16;
	}
	return 0;
}

/**
 * @note Address: N/A
 * @note Size: 0xC
 */
HSD_ObjAllocData* HSD_FObjGetAllocData(void)
{
	return &fobj_alloc_data;
}
