#include <global.h>
#include <dolphin/gx/gx.h>
#include <sysdolphin/class.h>
#include <sysdolphin/gobjproc.h>
#include <sysdolphin/texp.h>
#include <sysdolphin/tev.h>
#include <sysdolphin/tobj.h>

#define TEXP_ASSERT(line, cond) ((cond) ? ((void)0) : __assert("texp.c", line, #cond))

extern void* hsdAllocMemPiece(u32 size);
extern void memset(void* ptr, s32 value, u32 size);

#define GX_TEV_SWAP0 0
#define GX_TEV_SWAP1 1
#define GX_TEV_SWAP2 2
#define GX_TEV_SWAP3 3

#define GX_CC_CPREV 0
#define GX_CC_APREV 1
#define GX_CC_C0    2
#define GX_CC_A0    3
#define GX_CC_C1    4
#define GX_CC_A1    5
#define GX_CC_C2    6
#define GX_CC_A2    7
#define GX_CC_TEXC  8
#define GX_CC_TEXA  9
#define GX_CC_RASC  10
#define GX_CC_RASA  11
#define GX_CC_ONE   12
#define GX_CC_HALF  13
#define GX_CC_KONST 14
#define GX_CC_ZERO  15

#define GX_CA_APREV 0
#define GX_CA_A0    1
#define GX_CA_A1    2
#define GX_CA_A2    3
#define GX_CA_TEXA  4
#define GX_CA_RASA  5
#define GX_CA_KONST 6
#define GX_CA_ZERO  7

#define GX_TEV_ADD 0
#define GX_TEV_SUB 1

#define GX_TB_ZERO    0
#define GX_TB_ADDHALF 1
#define GX_TB_SUBHALF 2

#define GX_TC_LINEAR 0

#define GX_CS_SCALE_1  0
#define GX_CS_SCALE_2  1
#define GX_CS_SCALE_4  2
#define GX_CS_DIVIDE_2 3

#define GX_TEV_KCSEL_1    0x00
#define GX_TEV_KCSEL_7_8  0x01
#define GX_TEV_KCSEL_3_4  0x02
#define GX_TEV_KCSEL_5_8  0x03
#define GX_TEV_KCSEL_1_2  0x04
#define GX_TEV_KCSEL_3_8  0x05
#define GX_TEV_KCSEL_1_4  0x06
#define GX_TEV_KCSEL_1_8  0x07
#define GX_TEV_KCSEL_K0   0x0C
#define GX_TEV_KCSEL_K1   0x0D
#define GX_TEV_KCSEL_K2   0x0E
#define GX_TEV_KCSEL_K3   0x0F
#define GX_TEV_KCSEL_K0_R 0x10
#define GX_TEV_KCSEL_K1_R 0x11
#define GX_TEV_KCSEL_K2_R 0x12
#define GX_TEV_KCSEL_K3_R 0x13
#define GX_TEV_KCSEL_K0_G 0x14
#define GX_TEV_KCSEL_K1_G 0x15
#define GX_TEV_KCSEL_K2_G 0x16
#define GX_TEV_KCSEL_K3_G 0x17
#define GX_TEV_KCSEL_K0_B 0x18
#define GX_TEV_KCSEL_K1_B 0x19
#define GX_TEV_KCSEL_K2_B 0x1A
#define GX_TEV_KCSEL_K3_B 0x1B
#define GX_TEV_KCSEL_K0_A 0x1C
#define GX_TEV_KCSEL_K1_A 0x1D
#define GX_TEV_KCSEL_K2_A 0x1E
#define GX_TEV_KCSEL_K3_A 0x1F

#define GX_TEV_KASEL_1    0x00
#define GX_TEV_KASEL_7_8  0x01
#define GX_TEV_KASEL_3_4  0x02
#define GX_TEV_KASEL_5_8  0x03
#define GX_TEV_KASEL_1_2  0x04
#define GX_TEV_KASEL_3_8  0x05
#define GX_TEV_KASEL_1_4  0x06
#define GX_TEV_KASEL_1_8  0x07
#define GX_TEV_KASEL_K0_R 0x10
#define GX_TEV_KASEL_K1_R 0x11
#define GX_TEV_KASEL_K2_R 0x12
#define GX_TEV_KASEL_K3_R 0x13
#define GX_TEV_KASEL_K0_G 0x14
#define GX_TEV_KASEL_K1_G 0x15
#define GX_TEV_KASEL_K2_G 0x16
#define GX_TEV_KASEL_K3_G 0x17
#define GX_TEV_KASEL_K0_B 0x18
#define GX_TEV_KASEL_K1_B 0x19
#define GX_TEV_KASEL_K2_B 0x1A
#define GX_TEV_KASEL_K3_B 0x1B
#define GX_TEV_KASEL_K0_A 0x1C
#define GX_TEV_KASEL_K1_A 0x1D
#define GX_TEV_KASEL_K2_A 0x1E
#define GX_TEV_KASEL_K3_A 0x1F

#define GX_COLOR_NULL 0xFF

#define GX_KCOLOR0 0
#define GX_KCOLOR1 1
#define GX_KCOLOR2 2
#define GX_KCOLOR3 3

#define GX_TEVPREV 0
#define GX_TEVREG0 1
#define GX_TEVREG1 2
#define GX_TEVREG2 3

#define GX_ENABLE  1
#define GX_DISABLE 0
#define GX_TRUE    1
#define GX_FALSE   0

struct HSD_TExpDag {
	HSD_TETev* tev;
	u8 idx;
	u8 nb_dep;
	u8 nb_ref;
	u8 dist;
	struct HSD_TExpDag* depend[8];
};

struct HSD_TExpRes {
	s32 failed;
	s32 texmap;
	s32 cnst_remain;
	struct {
		u8 color;
		u8 alpha;
	} reg[8];
	u8 c_ref[4];
	u8 a_ref[4];
	u8 c_use[4];
	u8 a_use[4];
};

extern int HSD_Index2TevStage(int);

int HSD_TExpMakeDag(HSD_TExp* root, struct HSD_TExpDag* list);
void HSD_TExpSchedule(int num, struct HSD_TExpDag* list, HSD_TExp** result, struct HSD_TExpRes* resource);
int HSD_TExpSimplify(HSD_TExp* texp_);
int HSD_TExpSimplify2(HSD_TExp* texp_);

static inline u32 TExpDefault(u32 value, u32 fallback)
{
	if (value == HSD_TE_UNDEF) {
		value = fallback;
	}
	return value;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline bool IsThroughColor(HSD_TExp* texp)
{
	return texp->tev.c_op == GX_TEV_ADD && texp->tev.c_in[0].sel == HSD_TE_0 && texp->tev.c_in[1].sel == HSD_TE_0 && texp->tev.c_bias == 0
	    && texp->tev.c_scale == 0;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline bool IsThroughAlpha(HSD_TExp* texp)
{
	return texp->tev.a_op == GX_TEV_ADD && texp->tev.a_in[0].sel == HSD_TE_0 && texp->tev.a_in[1].sel == HSD_TE_0 && texp->tev.a_bias == 0
	    && texp->tev.a_scale == 0;
}

/**
 * @note Address: 0x801D65E0
 * @note Size: 0x3C
 */
HSD_TExpType HSD_TExpGetType(HSD_TExp* texp)
{
	if (texp == NULL) {
		return HSD_TE_ZERO;
	}
	if ((u32)texp == (u32)HSD_TEXP_TEX) {
		return HSD_TE_TEX;
	}
	if ((u32)texp == (u32)-2) {
		return HSD_TE_RAS;
	}
	return texp->type;
}

static HSD_TExp* TevAlloc(void)
{
	HSD_TExp* texp = hsdAllocMemPiece(sizeof(HSD_TETev));
	TEXP_ASSERT(63, texp);
	return texp;
}

static HSD_TExp* CnstAlloc(void)
{
	HSD_TExp* texp = hsdAllocMemPiece(sizeof(HSD_TECnst));
	TEXP_ASSERT(71, texp);
	return texp;
}

/**
 * @note Address: 0x801D661C
 * @note Size: 0x88
 */
void HSD_TExpRef(HSD_TExp* texp, u8 sel)
{
	HSD_TExpType type = HSD_TExpGetType(texp);
	switch (type) {
	case HSD_TE_TEV:
		if (sel == 1) {
			texp->tev.c_ref += 1;
			return;
		} else {
			texp->tev.a_ref += 1;
			return;
		}
		break;
	case HSD_TE_CNST:
		texp->cnst.ref += 1;
		break;
	}
}

#pragma push
#pragma inline_recursion on
#pragma inline_max_size(768)
/**
 * @note Address: 0x801D66A4
 * @note Size: 0x57C
 */
void HSD_TExpUnref(HSD_TExp* texp, u8 sel)
{
	s32 i;

	switch (HSD_TExpGetType(texp)) {
	case HSD_TE_TEV:
		if (sel == HSD_TE_RGB) {
			if (texp->tev.c_ref != 0) {
				texp->tev.c_ref -= 1;
			}
		} else if (texp->tev.a_ref != 0) {
			texp->tev.a_ref -= 1;
		}

		if (texp->tev.c_ref == 0 && texp->tev.a_ref == 0) {
			for (i = 0; i < 4; i++) {
				HSD_TExpUnref(texp->tev.c_in[i].exp, texp->tev.c_in[i].sel);
				HSD_TExpUnref(texp->tev.a_in[i].exp, texp->tev.a_in[i].sel);
			}
		}
		break;
	case HSD_TE_CNST:
		if (texp->cnst.ref != 0) {
			texp->cnst.ref -= 1;
		}
		break;
	}
}
#pragma pop

/**
 * @note Address: 0x801D6C20
 * @note Size: 0x320
 */
HSD_TExp* HSD_TExpFreeList(HSD_TExp* texp_list, HSD_TExpType type, s32 all)
{
	HSD_TExp** handle;
	HSD_TExp* ptr;
	HSD_TExp* next;

	handle = &texp_list;

	if (all != 0) {
		while (*handle != NULL) {
			if (type == HSD_TE_ALL || type == (*handle)->type) {
				switch ((*handle)->type) {
				case HSD_TE_TEV:
					next = (*handle)->tev.next;
					switch (HSD_TExpGetType(*handle)) {
					case HSD_TE_TEV:
						hsdFreeMemPiece(*handle, sizeof(HSD_TETev));
						break;
					case HSD_TE_CNST:
						hsdFreeMemPiece(*handle, sizeof(HSD_TECnst));
						break;
					}
					*handle = next;
					continue;
				case HSD_TE_CNST:
					next = (*handle)->tev.next;
					switch (HSD_TExpGetType(*handle)) {
					case HSD_TE_TEV:
						hsdFreeMemPiece(*handle, sizeof(HSD_TETev));
						break;
					case HSD_TE_CNST:
						hsdFreeMemPiece(*handle, sizeof(HSD_TECnst));
						break;
					}
					*handle = next;
					continue;
				default:
					TEXP_ASSERT(220, 0);
				}
			}
			handle = &(*handle)->tev.next;
		}
	} else {
		if (type == HSD_TE_ALL || type == HSD_TE_TEV) {
			for (ptr = texp_list; ptr != NULL; ptr = ptr->tev.next) {
				switch (ptr->type) {
				case HSD_TE_TEV:
					if (ptr->tev.c_ref == 0 && ptr->tev.c_ref == 0) {
						HSD_TExpUnref(ptr, 1);
						HSD_TExpUnref(ptr, 5);
					}
					break;
				}
			}
		}

		while (*handle != NULL) {
			if (type == HSD_TE_ALL || type == (*handle)->type) {
				switch ((*handle)->type) {
				case HSD_TE_TEV:
					if ((*handle)->tev.c_ref != 0 || (*handle)->tev.a_ref != 0) {
						break;
					}
					next = (*handle)->tev.next;
					switch (HSD_TExpGetType(*handle)) {
					case HSD_TE_TEV:
						hsdFreeMemPiece(*handle, sizeof(HSD_TETev));
						break;
					case HSD_TE_CNST:
						hsdFreeMemPiece(*handle, sizeof(HSD_TECnst));
						break;
					}
					*handle = next;
					continue;
				case HSD_TE_CNST:
					if ((*handle)->cnst.ref != 0) {
						break;
					}
					next = (*handle)->tev.next;
					switch (HSD_TExpGetType(*handle)) {
					case HSD_TE_TEV:
						hsdFreeMemPiece(*handle, sizeof(HSD_TETev));
						break;
					case HSD_TE_CNST:
						hsdFreeMemPiece(*handle, sizeof(HSD_TECnst));
						break;
					}
					*handle = next;
					continue;
				default:
					TEXP_ASSERT(268, 0);
				}
			}
			handle = &(*handle)->tev.next;
		}
	}

	return texp_list;
}

/**
 * @note Address: 0x801D6F40
 * @note Size: 0xC0
 */
HSD_TExp* HSD_TExpTev(HSD_TExp** texp_list)
{
	s32 i;
	HSD_TExp* texp;

	TEXP_ASSERT(295, texp_list);
	texp = TevAlloc();
	memset(texp, 0xFF, sizeof(HSD_TETev));
	texp->type      = HSD_TE_TEV;
	texp->tev.next  = *texp_list;
	*texp_list      = texp;
	texp->tev.c_ref = 0;
	texp->tev.a_ref = 0;
	texp->tev.tex   = NULL;
	for (i = 0; i < 4; i++) {
		texp->tev.c_in[i].exp = NULL;
		texp->tev.a_in[i].exp = NULL;
	}
	return texp;
}

/**
 * @note Address: 0x801D7000
 * @note Size: 0x120
 */
HSD_TExp* HSD_TExpCnst(void* val, HSD_TEInput comp, HSD_TEType type, HSD_TExp** texp_list)
{
	HSD_TExp* texp;

	TEXP_ASSERT(362, texp_list);

	texp = *texp_list;
	do {
		while (texp) {
			if (texp->type == HSD_TE_CNST && texp->cnst.val == val && texp->cnst.comp == comp) {
				TEXP_ASSERT(369, texp->cnst.ctype == type);
				return texp;
			}
			texp = texp->cnst.next;
		}

		if (comp == HSD_TE_0) {
			return NULL;
		}
		texp             = CnstAlloc();
		texp->cnst.type  = HSD_TE_CNST;
		texp->cnst.next  = (*texp_list);
		*texp_list       = texp;
		texp->cnst.ref   = 0;
		texp->cnst.val   = val;
		texp->cnst.comp  = comp;
		texp->cnst.ctype = type;
		texp->cnst.reg   = 0xFF;
		texp->cnst.idx   = 0xFF;
		texp->cnst.range = 0;
		return texp;
	} while (1);

	return texp;
}

/**
 * @note Address: 0x801D7120
 * @note Size: 0xE8
 */
void HSD_TExpColorOp(HSD_TExp* texp, s32 op, u32 bias, u32 scale, u8 clamp)
{
	TEXP_ASSERT(410, texp);
	TEXP_ASSERT(411, HSD_TExpGetType(texp) == HSD_TE_TEV);

	texp->tev.c_op    = op;
	texp->tev.c_clamp = (clamp ? GX_ENABLE : GX_DISABLE);
	if (op == GX_TEV_ADD || op == GX_TEV_SUB) {
		texp->tev.c_bias  = bias;
		texp->tev.c_scale = scale;
	} else {
		texp->tev.c_bias  = 0;
		texp->tev.c_scale = 0;
	}
}

/**
 * @note Address: 0x801D7208
 * @note Size: 0xE8
 */
void HSD_TExpAlphaOp(HSD_TExp* texp, s32 op, u32 bias, u32 scale, u8 clamp)
{
	TEXP_ASSERT(436, texp);
	TEXP_ASSERT(437, HSD_TExpGetType(texp) == HSD_TE_TEV);

	texp->tev.a_op    = op;
	texp->tev.a_clamp = (clamp ? GX_ENABLE : GX_DISABLE);
	if (op == GX_TEV_ADD || op == GX_TEV_SUB) {
		texp->tev.a_bias  = bias;
		texp->tev.a_scale = scale;
	} else {
		texp->tev.a_bias  = 0;
		texp->tev.a_scale = 0;
	}
}

/**
 * @note Address: 0x801D72F0
 * @note Size: 0x5B8
 */
void HSD_TExpColorInSub(HSD_TETev* tev, HSD_TEInput sel, HSD_TExp* exp, s32 idx)
{
	HSD_TEArg prev;
	u8 ksel;

	prev                = tev->c_in[idx];
	tev->c_in[idx].type = HSD_TExpGetType(exp);
	tev->c_in[idx].sel  = sel;
	tev->c_in[idx].exp  = exp;
	tev->c_in[idx].arg  = 0xFF;

	switch (sel) {
	case HSD_TE_0:
		tev->c_in[idx].arg  = GX_CC_ZERO;
		tev->c_in[idx].type = HSD_TE_ZERO;
		tev->c_in[idx].exp  = NULL;
		break;
	case HSD_TE_1:
		tev->c_in[idx].arg  = GX_CC_ONE;
		tev->c_in[idx].type = HSD_TE_IMM;
		tev->c_in[idx].exp  = NULL;
		break;
	case HSD_TE_4_8:
		tev->c_in[idx].arg  = GX_CC_HALF;
		tev->c_in[idx].type = HSD_TE_IMM;
		tev->c_in[idx].exp  = NULL;
		break;
	case HSD_TE_1_8:
	case HSD_TE_2_8:
	case HSD_TE_3_8:
	case HSD_TE_5_8:
	case HSD_TE_6_8:
	case HSD_TE_7_8:
		tev->c_in[idx].arg = GX_CC_KONST;
		switch (sel) {
		case HSD_TE_1_8:
			ksel = GX_TEV_KCSEL_1_8;
			break;
		case HSD_TE_2_8:
			ksel = GX_TEV_KCSEL_1_4;
			break;
		case HSD_TE_3_8:
			ksel = GX_TEV_KCSEL_3_8;
			break;
		case HSD_TE_5_8:
			ksel = GX_TEV_KCSEL_5_8;
			break;
		case HSD_TE_6_8:
			ksel = GX_TEV_KCSEL_3_4;
			break;
		case HSD_TE_7_8:
			ksel = GX_TEV_KCSEL_7_8;
			break;
		default:
			TEXP_ASSERT(500, 0);
			break;
		}

		if (tev->kcsel == HSD_TE_UNDEF) {
			tev->kcsel = ksel;
		} else if (tev->kcsel == ksel) {
			HSD_Panic("texp.c", 505, "tev can't select multiple konst input.\n");
		}
		tev->c_in[idx].type = HSD_TE_KONST;
		break;

	default: {
		switch (tev->c_in[idx].type) {
		case HSD_TE_ZERO:
			tev->c_in[idx].type = HSD_TE_ZERO;
			tev->c_in[idx].sel  = HSD_TE_0;
			tev->c_in[idx].arg  = GX_CC_ZERO;
			break;
		case HSD_TE_TEV: {
			u8 swap;
			TEXP_ASSERT(521, sel == HSD_TE_RGB || sel == HSD_TE_A);
			TEXP_ASSERT(523, idx == 3 || sel != HSD_TE_RGB || exp->tev.c_clamp);
			TEXP_ASSERT(524, idx == 3 || sel != HSD_TE_A || exp->tev.a_clamp);
			swap = tev->c_in[idx].sel;
			switch (HSD_TExpGetType(tev->c_in[idx].exp)) {
			case HSD_TE_TEV:
				if (swap == 1) {
					tev->c_in[idx].exp->tev.c_ref += 1;
				} else {
					tev->c_in[idx].exp->tev.a_ref += 1;
				}
				break;
			case HSD_TE_CNST:
				tev->c_in[idx].exp->cnst.ref += 1;
				break;
			}
		} break;
		case HSD_TE_CNST: {
			u8 swap;
			tev->c_in[idx].sel = exp->cnst.comp;
			swap               = tev->c_in[idx].sel;
			switch (HSD_TExpGetType(tev->c_in[idx].exp)) {
			case HSD_TE_TEV:
				if (swap == 1) {
					tev->c_in[idx].exp->tev.c_ref += 1;
				} else {
					tev->c_in[idx].exp->tev.a_ref += 1;
				}
				break;
			case HSD_TE_CNST:
				tev->c_in[idx].exp->cnst.ref += 1;
				break;
			}
		} break;
		case HSD_TE_TEX: {
			u8 swap = HSD_TE_UNDEF;
			switch (sel) {
			case HSD_TE_RGB:
				tev->c_in[idx].arg = GX_CC_TEXC;
				swap               = GX_TEV_SWAP0;
				break;
			case HSD_TE_R:
				tev->c_in[idx].arg = GX_CC_TEXC;
				swap               = GX_TEV_SWAP1;
				break;
			case HSD_TE_G:
				tev->c_in[idx].arg = GX_CC_TEXC;
				swap               = GX_TEV_SWAP2;
				break;
			case HSD_TE_B:
				tev->c_in[idx].arg = GX_CC_TEXC;
				swap               = GX_TEV_SWAP3;
				break;
			case HSD_TE_A:
				tev->c_in[idx].arg = GX_CC_TEXA;
				break;
			default:
				TEXP_ASSERT(559, 0);
				break;
			}
			if (tev->tex_swap == HSD_TE_UNDEF) {
				tev->tex_swap = swap;
			} else {
				TEXP_ASSERT(564, swap == HSD_TE_UNDEF || tev->tex_swap == swap);
			}
		} break;
		case HSD_TE_RAS: {
			u8 swap = HSD_TE_UNDEF;
			switch (sel) {
			case HSD_TE_RGB:
				tev->c_in[idx].arg = GX_CC_RASC;
				swap               = GX_TEV_SWAP0;
				break;
			case HSD_TE_R:
				tev->c_in[idx].arg = GX_CC_RASC;
				swap               = GX_TEV_SWAP1;
				break;
			case HSD_TE_G:
				tev->c_in[idx].arg = GX_CC_RASC;
				swap               = GX_TEV_SWAP2;
				break;
			case HSD_TE_B:
				tev->c_in[idx].arg = GX_CC_RASC;
				swap               = GX_TEV_SWAP3;
				break;
			case HSD_TE_A:
				tev->c_in[idx].arg = GX_CC_RASA;
				break;
			default:
				TEXP_ASSERT(607, 0);
				break;
			}
			if (tev->ras_swap == HSD_TE_UNDEF) {
				tev->ras_swap = swap;
			} else {
				TEXP_ASSERT(612, swap == HSD_TE_UNDEF || tev->ras_swap == swap);
			}
		} break;
		default:
			TEXP_ASSERT(630, 0);
			break;
		}
	}
	}
	HSD_TExpUnref(prev.exp, prev.sel);
}

/**
 * @note Address: 0x801D78A8
 * @note Size: 0x108
 */
void HSD_TExpColorIn(HSD_TExp* texp, HSD_TEInput sel_a, HSD_TExp* exp_a, HSD_TEInput sel_b, HSD_TExp* exp_b, HSD_TEInput sel_c,
                     HSD_TExp* exp_c, HSD_TEInput sel_d, HSD_TExp* exp_d)
{
	HSD_TETev* tev;

	TEXP_ASSERT(693, texp);
	TEXP_ASSERT(694, HSD_TExpGetType(texp) == HSD_TE_TEV);

	tev = &texp->tev;
	HSD_TExpColorInSub(tev, sel_a, exp_a, 0);
	HSD_TExpColorInSub(tev, sel_b, exp_b, 1);
	HSD_TExpColorInSub(tev, sel_c, exp_c, 2);
	HSD_TExpColorInSub(tev, sel_d, exp_d, 3);
}

/**
 * @note Address: 0x801D79B0
 * @note Size: 0x3F8
 */
void HSD_TExpAlphaInSub(HSD_TETev* tev, HSD_TEInput sel, HSD_TExp* exp, s32 idx)
{
	HSD_TEArg prev;
	u8 ksel;

	prev                = tev->a_in[idx];
	tev->a_in[idx].sel  = sel;
	tev->a_in[idx].exp  = exp;
	tev->a_in[idx].type = HSD_TExpGetType(exp);
	tev->a_in[idx].arg  = 0xFF;

	switch (sel) {
	case HSD_TE_0:
		tev->a_in[idx].type = HSD_TE_ZERO;
		tev->a_in[idx].arg  = GX_CA_ZERO;
		tev->a_in[idx].exp  = NULL;
		break;

	case HSD_TE_1:
	case HSD_TE_1_8:
	case HSD_TE_2_8:
	case HSD_TE_3_8:
	case HSD_TE_4_8:
	case HSD_TE_5_8:
	case HSD_TE_6_8:
	case HSD_TE_7_8:
		tev->a_in[idx].exp = NULL;
		tev->a_in[idx].arg = GX_CA_KONST;
		switch (sel) {
		case HSD_TE_1_8:
			ksel = GX_TEV_KASEL_1_8;
			break;
		case HSD_TE_2_8:
			ksel = GX_TEV_KASEL_1_4;
			break;
		case HSD_TE_3_8:
			ksel = GX_TEV_KASEL_3_8;
			break;
		case HSD_TE_4_8:
			ksel = GX_TEV_KASEL_1_2;
			break;
		case HSD_TE_5_8:
			ksel = GX_TEV_KASEL_5_8;
			break;
		case HSD_TE_6_8:
			ksel = GX_TEV_KASEL_3_4;
			break;
		case HSD_TE_7_8:
			ksel = GX_TEV_KASEL_7_8;
			break;
		default:
			ksel = GX_TEV_KASEL_1;
			break;
		}

		if (tev->kasel == HSD_TE_UNDEF) {
			tev->kasel = ksel;
		} else {
			TEXP_ASSERT(750, tev->kasel == ksel);
		}
		tev->a_in[idx].type = HSD_TE_KONST;
		break;

	default: {
		switch (tev->a_in[idx].type) {
		case HSD_TE_ZERO:
			tev->a_in[idx].exp  = NULL;
			tev->a_in[idx].type = HSD_TE_ZERO;
			tev->a_in[idx].sel  = HSD_TE_0;
			tev->a_in[idx].arg  = GX_CA_ZERO;
			break;
		case HSD_TE_TEV:
			TEXP_ASSERT(773, sel == HSD_TE_A);
			TEXP_ASSERT(774, idx == 3 || exp->tev.a_clamp);
			HSD_TExpRef(tev->a_in[idx].exp, tev->a_in[idx].sel);
			break;
		case HSD_TE_CNST:
			TEXP_ASSERT(778, sel == HSD_TE_A || sel == HSD_TE_X);
			TEXP_ASSERT(779, exp->cnst.comp == HSD_TE_X);
			tev->a_in[idx].sel = HSD_TE_X;
			HSD_TExpRef(tev->a_in[idx].exp, tev->a_in[idx].sel);
			break;
		case HSD_TE_TEX:
			TEXP_ASSERT(784, sel == HSD_TE_A);
			tev->a_in[idx].arg = GX_CA_TEXA;
			break;
		case HSD_TE_RAS:
			TEXP_ASSERT(788, sel == HSD_TE_A);
			tev->a_in[idx].arg = GX_CA_RASA;
			break;
		default:
			TEXP_ASSERT(792, 0);
			break;
		}
	}
	}
	HSD_TExpUnref(prev.exp, prev.sel);
}

/**
 * @note Address: 0x801D7DA8
 * @note Size: 0x108
 */
void HSD_TExpAlphaIn(HSD_TExp* texp, HSD_TEInput sel_a, HSD_TExp* exp_a, HSD_TEInput sel_b, HSD_TExp* exp_b, HSD_TEInput sel_c,
                     HSD_TExp* exp_c, HSD_TEInput sel_d, HSD_TExp* exp_d)
{
	HSD_TETev* tev;

	TEXP_ASSERT(823, texp);
	TEXP_ASSERT(824, HSD_TExpGetType(texp) == HSD_TE_TEV);

	tev = &texp->tev;
	HSD_TExpAlphaInSub(tev, sel_a, exp_a, 0);
	HSD_TExpAlphaInSub(tev, sel_b, exp_b, 1);
	HSD_TExpAlphaInSub(tev, sel_c, exp_c, 2);
	HSD_TExpAlphaInSub(tev, sel_d, exp_d, 3);
}

/**
 * @note Address: 0x801D7EB0
 * @note Size: 0xC4
 */
void HSD_TExpOrder(HSD_TExp* texp, HSD_TObj* tex, s32 chan)
{
	TEXP_ASSERT(839, texp);
	TEXP_ASSERT(840, HSD_TExpGetType(texp) == HSD_TE_TEV);

	texp->tev.tex = tex;
	if (chan == GX_COLOR_NULL) {
		texp->tev.chan = GX_COLOR_NULL;
		return;
	}
	texp->tev.chan = chan;
}

/**
 * @note Address: 0x801D7F74
 * @note Size: 0x178
 */
int AssignColorReg(HSD_TETev* tev, int idx, HSD_TExpRes* res)
{
	static u32 acr_a_in[4] = { GX_CC_A0, GX_CC_A1, GX_CC_A2, GX_CC_APREV };
	static u32 acr_c_in[4] = { GX_CC_C0, GX_CC_C1, GX_CC_C2, GX_CC_CPREV };

	HSD_TECnst* cnst;
	int j;

	cnst = &tev->c_in[idx].exp->cnst;
	if (cnst->reg != HSD_TE_UNDEF) {
		if (cnst->reg < 4) {
			return -1;
		}
		tev->c_in[idx].type = HSD_TE_IMM;
		if (cnst->comp == HSD_TE_X) {
			tev->c_in[idx].arg = acr_a_in[cnst->reg - 4];
		} else {
			tev->c_in[idx].arg = acr_c_in[cnst->reg - 4];
		}
		return 0;
	} else {
		if (cnst->comp == HSD_TE_X) {
			for (j = 4; j < 8; ++j) {
				if (res->reg[j].alpha == 0) {
					res->reg[j].alpha   = 1;
					cnst->reg           = j;
					cnst->idx           = 3;
					tev->c_in[idx].type = HSD_TE_IMM;
					tev->c_in[idx].arg  = acr_a_in[j - 4];
					return 0;
				}
			}
		} else {
			for (j = 4; j < 8; ++j) {
				if (res->reg[j].color == 0) {
					res->reg[j].color   = 3;
					cnst->reg           = j;
					cnst->idx           = 0;
					tev->c_in[idx].type = HSD_TE_IMM;
					tev->c_in[idx].arg  = acr_c_in[j - 4];
					return 0;
				}
			}
		}
	}
	return -1;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static int AssignAlphaReg(HSD_TETev* tev, int idx, HSD_TExpRes* res)
{
	static u32 in[4] = { GX_CA_A0, GX_CA_A1, GX_CA_A2, GX_CA_APREV };

	HSD_TECnst* cnst;
	int j;

	cnst = &tev->a_in[idx].exp->cnst;
	if (cnst->reg != HSD_TE_UNDEF) {
		if (cnst->reg < 4) {
			return -1;
		}
		tev->a_in[idx].type = HSD_TE_IMM;
		tev->a_in[idx].arg  = in[cnst->reg - 4];
		return 0;
	} else {
		for (j = 4; j < 8; ++j) {
			if (res->reg[j].alpha == 0) {
				res->reg[j].alpha   = 1;
				cnst->reg           = j;
				cnst->idx           = 3;
				tev->a_in[idx].type = HSD_TE_IMM;
				tev->a_in[idx].arg  = in[j - 4];
				return 0;
			}
		}
	}
	return -1;
}

/**
 * @note Address: 0x801D80EC
 * @note Size: 0x22C
 */
int AssignColorKonst(HSD_TETev* tev, int idx, HSD_TExpRes* res)
{
	static u32 xsel[4][4] = { { GX_TEV_KCSEL_K0_R, GX_TEV_KCSEL_K0_G, GX_TEV_KCSEL_K0_B, GX_TEV_KCSEL_K0_A },
		                      { GX_TEV_KCSEL_K1_R, GX_TEV_KCSEL_K1_G, GX_TEV_KCSEL_K1_B, GX_TEV_KCSEL_K1_A },
		                      { GX_TEV_KCSEL_K2_R, GX_TEV_KCSEL_K2_G, GX_TEV_KCSEL_K2_B, GX_TEV_KCSEL_K2_A },
		                      { GX_TEV_KCSEL_K3_R, GX_TEV_KCSEL_K3_G, GX_TEV_KCSEL_K3_B, GX_TEV_KCSEL_K3_A } };

	static u32 csel[4] = { GX_TEV_KCSEL_K0, GX_TEV_KCSEL_K1, GX_TEV_KCSEL_K2, GX_TEV_KCSEL_K3 };

	HSD_TECnst* cnst;
	int j;

	cnst = &tev->c_in[idx].exp->cnst;
	if (cnst->reg != HSD_TE_UNDEF) {
		if (cnst->reg >= 4) {
			return -1;
		}
		if (cnst->comp == HSD_TE_X) {
			tev->kcsel          = xsel[cnst->reg][cnst->idx];
			tev->c_in[idx].type = HSD_TE_KONST;
			tev->c_in[idx].arg  = GX_CC_KONST;
		} else {
			tev->kcsel          = csel[cnst->reg];
			tev->c_in[idx].type = HSD_TE_KONST;
			tev->c_in[idx].arg  = GX_CC_KONST;
		}
		return 0;
	} else {
		if (cnst->comp == HSD_TE_X) {
			for (j = 1; j < 4; j++) {
				if (res->reg[j].alpha == 0) {
					res->reg[j].alpha   = 1;
					cnst->reg           = j;
					cnst->idx           = 3;
					tev->kcsel          = xsel[cnst->reg][cnst->idx];
					tev->c_in[idx].type = HSD_TE_KONST;
					tev->c_in[idx].arg  = GX_CC_KONST;
					return 0;
				}
			}

			for (j = 0; j < 4; j++) {
				if (res->reg[j].color < 3) {
					cnst->reg           = j;
					cnst->idx           = res->reg[j].color++;
					tev->kcsel          = xsel[cnst->reg][cnst->idx];
					tev->c_in[idx].type = HSD_TE_KONST;
					tev->c_in[idx].arg  = GX_CC_KONST;
					return 0;
				}
			}
		} else {
			for (j = 0; j < 4; j++) {
				if (res->reg[j].color == 0) {
					res->reg[j].color   = 3;
					cnst->reg           = j;
					cnst->idx           = 0;
					tev->kcsel          = csel[cnst->reg];
					tev->c_in[idx].type = HSD_TE_KONST;
					tev->c_in[idx].arg  = GX_CC_KONST;
					return 0;
				}
			}
		}
	}
	return -1;
}

/**
 * @note Address: 0x801D8318
 * @note Size: 0x164
 */
int AssignAlphaKonst(HSD_TETev* tev, int idx, HSD_TExpRes* res)
{
	static u32 sel[4][4] = { { GX_TEV_KASEL_K0_R, GX_TEV_KASEL_K0_G, GX_TEV_KASEL_K0_B, GX_TEV_KASEL_K0_A },
		                     { GX_TEV_KASEL_K1_R, GX_TEV_KASEL_K1_G, GX_TEV_KASEL_K1_B, GX_TEV_KASEL_K1_A },
		                     { GX_TEV_KASEL_K2_R, GX_TEV_KASEL_K2_G, GX_TEV_KASEL_K2_B, GX_TEV_KASEL_K2_A },
		                     { GX_TEV_KASEL_K3_R, GX_TEV_KASEL_K3_G, GX_TEV_KASEL_K3_B, GX_TEV_KASEL_K3_A } };

	HSD_TECnst* cnst;
	int j;

	cnst = &tev->a_in[idx].exp->cnst;
	if (cnst->reg != HSD_TE_UNDEF) {
		if (cnst->reg >= 4) {
			return -1;
		}
		tev->kasel          = sel[cnst->reg][cnst->idx];
		tev->a_in[idx].type = HSD_TE_KONST;
		tev->a_in[idx].arg  = GX_CA_KONST;
		return 0;
	} else {
		for (j = 1; j < 4; j++) {
			if (res->reg[j].alpha == 0) {
				res->reg[j].alpha   = 1;
				cnst->reg           = j;
				cnst->idx           = 3;
				tev->kasel          = sel[cnst->reg][cnst->idx];
				tev->a_in[idx].type = HSD_TE_KONST;
				tev->a_in[idx].arg  = GX_CA_KONST;
				return 0;
			}
		}

		for (j = 0; j < 4; j++) {
			if (res->reg[j].color < 3) {
				cnst->reg           = j;
				cnst->idx           = res->reg[j].color++;
				tev->kasel          = sel[cnst->reg][cnst->idx];
				tev->a_in[idx].type = HSD_TE_KONST;
				tev->a_in[idx].arg  = GX_CA_KONST;
				return 0;
			}
		}
	}
	return -1;
}

/**
 * @note Address: N/A
 * @note Size: 0x6CC
 */
int TExpAssignReg(HSD_TExp* texp, HSD_TExpRes* res)
{
	HSD_TETev* tev = (HSD_TETev*)texp;
	int i, val;

	if (tev->c_ref > 0) {
		if (tev->kcsel != HSD_TE_UNDEF) {
			for (i = 0; i < 4; i++) {
				if (tev->c_in[i].type == HSD_TE_CNST) {
					val = AssignColorReg(tev, i, res);
					if (val < 0) {
						TEXP_ASSERT(1149, val >= 0);
						return val;
					}
				}
			}
		} else if (IsThroughColor((HSD_TExp*)tev) && tev->c_in[3].type == HSD_TE_CNST) {
			if (AssignColorReg(tev, 3, res) < 0) {
				val = AssignColorKonst(tev, 3, res);
				if (val < 0) {
					TEXP_ASSERT(1161, val >= 0);
					return val;
				}
			}
		} else {
			for (i = 0; i < 4; i++) {
				if (tev->c_in[i].type == HSD_TE_CNST) {
					if (AssignColorKonst(tev, i, res) < 0) {
						val = AssignColorReg(tev, i, res);
						if (val < 0) {
							TEXP_ASSERT(1174, val >= 0);
							return val;
						}
					}
				}
			}
			for (; i < 4; i++) {
				if (tev->c_in[i].type == HSD_TE_CNST) {
					val = AssignColorReg(tev, i, res);
					if (val < 0) {
						TEXP_ASSERT(1183, val >= 0);
						return val;
					}
				}
			}
		}
	}

	if (tev->a_ref > 0) {
		if (tev->kasel != HSD_TE_UNDEF) {
			for (i = 0; i < 4; i++) {
				if (tev->a_in[i].type == HSD_TE_CNST) {
					val = AssignAlphaReg(tev, i, res);
					if (val < 0) {
						TEXP_ASSERT(1199, val >= 0);
						return val;
					}
				}
			}
		} else {
			if (IsThroughAlpha((HSD_TExp*)tev) && tev->a_in[3].type == HSD_TE_CNST) {
				if (AssignAlphaReg(tev, 3, res) < 0) {
					val = AssignAlphaKonst(tev, 3, res);
					if (val < 0) {
						TEXP_ASSERT(1211, val >= 0);
						return val;
					}
				}
			} else {
				for (i = 0; i < 4; i++) {
					if (tev->a_in[i].type == HSD_TE_CNST) {
						if (AssignAlphaKonst(tev, i, res) < 0) {
							val = AssignAlphaReg(tev, i, res);
							if (val < 0) {
								TEXP_ASSERT(1224, val >= 0);
								return val;
							}
						}
					}
				}
				for (; i < 4; i++) {
					if (tev->a_in[i].type == HSD_TE_CNST) {
						val = AssignAlphaReg(tev, i, res);
						if (val < 0) {
							TEXP_ASSERT(1233, val >= 0);
							return val;
						}
					}
				}
			}
		}
	}
	return 0;
}

/**
 * @note Address: N/A
 * @note Size: 0x400
 */
void TExp2TevDesc(HSD_TExp* texp, HSD_TExpTevDesc* desc, int* init_cprev, int* init_aprev)
{
	static u32 dst[4] = { GX_TEVREG0, GX_TEVREG1, GX_TEVREG2, GX_TEVPREV };

	HSD_TETev* tev;
	HSD_TevDesc* tevdesc;

	TEXP_ASSERT(1289, texp);
	TEXP_ASSERT(1290, desc);
	TEXP_ASSERT(1291, HSD_TExpGetType(texp) == HSD_TE_TEV);

	tev     = &texp->tev;
	tevdesc = &desc->desc;

	tevdesc->next  = NULL;
	tevdesc->flags = 1;
	desc->tobj     = tev->tex;
	if (tev->tex == NULL) {
		tevdesc->coord = HSD_TE_UNDEF;
		tevdesc->map   = HSD_TE_UNDEF;
	}
	tevdesc->color              = TExpDefault(tev->chan, HSD_TE_UNDEF);
	tevdesc->u.tevconf.ras_swap = TExpDefault(tev->ras_swap, 0);
	tevdesc->u.tevconf.tex_swap = TExpDefault(tev->tex_swap, 0);
	tevdesc->u.tevconf.kcsel    = TExpDefault(tev->kcsel, 0);
	tevdesc->u.tevconf.kasel    = TExpDefault(tev->kasel, 0);

	if ((tev->c_op == HSD_TE_UNDEF)
	    || (tev->c_ref == 0 && tev->a_op != 8 && tev->a_op != 9 && tev->a_op != 0xA && tev->a_op != 0xB && tev->a_op != 0xC
	        && tev->a_op != 0xD)) {
		tevdesc->u.tevconf.clr_op = GX_TEV_ADD;
		tevdesc->u.tevconf.clr_a  = GX_CC_ZERO;
		tevdesc->u.tevconf.clr_b  = GX_CC_ZERO;
		tevdesc->u.tevconf.clr_c  = GX_CC_ZERO;
		if (*init_cprev != 0) {
			*init_cprev              = 0;
			tevdesc->u.tevconf.clr_d = GX_CC_ZERO;
		} else {
			tevdesc->u.tevconf.clr_d = GX_CC_CPREV;
		}
		tevdesc->u.tevconf.clr_scale   = 0;
		tevdesc->u.tevconf.clr_bias    = 0;
		tevdesc->u.tevconf.clr_clamp   = 0;
		tevdesc->u.tevconf.clr_out_reg = 0;
	} else {
		tevdesc->u.tevconf.clr_op    = tev->c_op;
		tevdesc->u.tevconf.clr_a     = TExpDefault(tev->c_in[0].arg, GX_CC_ZERO);
		tevdesc->u.tevconf.clr_b     = TExpDefault(tev->c_in[1].arg, GX_CC_ZERO);
		tevdesc->u.tevconf.clr_c     = TExpDefault(tev->c_in[2].arg, GX_CC_ZERO);
		tevdesc->u.tevconf.clr_d     = TExpDefault(tev->c_in[3].arg, GX_CC_ZERO);
		tevdesc->u.tevconf.clr_scale = TExpDefault(tev->c_scale, 0);
		tevdesc->u.tevconf.clr_bias  = TExpDefault(tev->c_bias, 0);
		tevdesc->u.tevconf.clr_clamp = tev->c_clamp != 0 ? GX_TRUE : GX_FALSE;

		TEXP_ASSERT(1354, tev->c_dst != HSD_TE_UNDEF);
		tevdesc->u.tevconf.clr_out_reg = dst[tev->c_dst];
		if ((s32)tevdesc->u.tevconf.clr_out_reg == 0) {
			*init_cprev = 0;
		}
	}

	if (tev->a_op == HSD_TE_UNDEF || tev->a_ref == 0) {
		tevdesc->u.tevconf.alpha_op = GX_TEV_ADD;
		tevdesc->u.tevconf.alpha_a  = GX_CA_ZERO;
		tevdesc->u.tevconf.alpha_b  = GX_CA_ZERO;
		tevdesc->u.tevconf.alpha_c  = GX_CA_ZERO;
		if (*init_aprev != 0) {
			*init_aprev                = 0;
			tevdesc->u.tevconf.alpha_d = GX_CA_ZERO;
		} else {
			tevdesc->u.tevconf.alpha_d = GX_CA_APREV;
		}
		tevdesc->u.tevconf.alpha_scale   = 0;
		tevdesc->u.tevconf.alpha_bias    = 0;
		tevdesc->u.tevconf.alpha_clamp   = 0;
		tevdesc->u.tevconf.alpha_out_reg = 0;
	} else {
		tevdesc->u.tevconf.alpha_op    = tev->a_op;
		tevdesc->u.tevconf.alpha_a     = TExpDefault(tev->a_in[0].arg, GX_CA_ZERO);
		tevdesc->u.tevconf.alpha_b     = TExpDefault(tev->a_in[1].arg, GX_CA_ZERO);
		tevdesc->u.tevconf.alpha_c     = TExpDefault(tev->a_in[2].arg, GX_CA_ZERO);
		tevdesc->u.tevconf.alpha_d     = TExpDefault(tev->a_in[3].arg, GX_CA_ZERO);
		tevdesc->u.tevconf.alpha_scale = TExpDefault(tev->a_scale, 0);
		tevdesc->u.tevconf.alpha_bias  = TExpDefault(tev->a_bias, 0);
		tevdesc->u.tevconf.alpha_clamp = tev->a_clamp != 0 ? GX_TRUE : GX_FALSE;

		TEXP_ASSERT(1390, tev->a_dst != HSD_TE_UNDEF);
		tevdesc->u.tevconf.alpha_out_reg = dst[tev->a_dst];
		if ((s32)tevdesc->u.tevconf.alpha_out_reg == 0) {
			*init_aprev = 0;
		}
	}
	tevdesc->u.tevconf.mode = GX_TC_LINEAR;
}

static u32 id1[4] = { GX_KCOLOR0, GX_KCOLOR1, GX_KCOLOR2, GX_KCOLOR3 };
static u32 id2[3] = { GX_TEVREG0, GX_TEVREG1, GX_TEVREG2 };

/**
 * @note Address: 0x801D847C
 * @note Size: 0x6CC
 */
// TExpAssignReg

/**
 * @note Address: 0x801D8B48
 * @note Size: 0x400
 */
// TExp2TevDesc
/*
    stwu     r1, -0x20(r1)
    mflr     r0
    stw      r0, 0x24(r1)
    addi     r11, r1, 0x20
    bl       _savegpr_27
    mr.      r30, r3
    lis      r3, lbl_8025ACA8@ha
    mr       r31, r4
    mr       r27, r5
    mr       r28, r6
    addi     r29, r3, lbl_8025ACA8@l
    bne      .L_801D8B88
    li       r3, lbl_8065B038@sda21
    li       r4, 0x509
    li       r5, lbl_8065B040@sda21
    bl       __assert
.L_801D8B88:
    cmplwi   r31, 0x0
    bne      .L_801D8BA0
    li       r3, lbl_8065B038@sda21
    li       r4, 0x50a
    li       r5, lbl_8065B04C@sda21
    bl       __assert
.L_801D8BA0:
    cmplwi   r30, 0x0
    bne      .L_801D8BB0
    li       r0, 0x0
    b        .L_801D8BD8
.L_801D8BB0:
    addis    r0, r30, 0x1
    cmplwi   r0, 0xffff
    bne      .L_801D8BC4
    li       r0, 0x2
    b        .L_801D8BD8
.L_801D8BC4:
    cmplwi   r0, 0xfffe
    bne      .L_801D8BD4
    li       r0, 0x3
    b        .L_801D8BD8
.L_801D8BD4:
    lwz      r0, 0x0(r30)
.L_801D8BD8:
    cmpwi    r0, 0x1
    beq      .L_801D8BF0
    addi     r5, r29, 0x28
    li       r3, lbl_8065B038@sda21
    li       r4, 0x50b
    bl       __assert
.L_801D8BF0:
    li       r3, 0x0
    li       r0, 0x1
    stw      r3, 0x0(r31)
    stw      r0, 0x4(r31)
    lwz      r0, 0x64(r30)
    stw      r0, 0x74(r31)
    lwz      r0, 0x64(r30)
    cmplwi   r0, 0x0
    bne      .L_801D8C20
    li       r0, 0xff
    stw      r0, 0xc(r31)
    stw      r0, 0x10(r31)
.L_801D8C20:
    lbz      r0, 0x68(r30)
    cmplwi   r0, 0xff
    bne      .L_801D8C30
    li       r0, 0xff
.L_801D8C30:
    stw      r0, 0x14(r31)
    lbz      r0, 0x1f(r30)
    cmplwi   r0, 0xff
    bne      .L_801D8C44
    li       r0, 0x0
.L_801D8C44:
    stw      r0, 0x64(r31)
    lbz      r0, 0x1e(r30)
    cmplwi   r0, 0xff
    bne      .L_801D8C58
    li       r0, 0x0
.L_801D8C58:
    stw      r0, 0x68(r31)
    lbz      r0, 0x20(r30)
    cmplwi   r0, 0xff
    bne      .L_801D8C6C
    li       r0, 0x0
.L_801D8C6C:
    stw      r0, 0x6c(r31)
    lbz      r0, 0x21(r30)
    cmplwi   r0, 0xff
    bne      .L_801D8C80
    li       r0, 0x0
.L_801D8C80:
    stw      r0, 0x70(r31)
    lbz      r3, 0xd(r30)
    cmplwi   r3, 0xff
    beq      .L_801D8CD0
    lwz      r0, 0x8(r30)
    cmpwi    r0, 0x0
    bne      .L_801D8D1C
    lbz      r0, 0x19(r30)
    cmplwi   r0, 0x8
    beq      .L_801D8D1C
    cmplwi   r0, 0x9
    beq      .L_801D8D1C
    cmplwi   r0, 0xa
    beq      .L_801D8D1C
    cmplwi   r0, 0xb
    beq      .L_801D8D1C
    cmplwi   r0, 0xc
    beq      .L_801D8D1C
    cmplwi   r0, 0xd
    beq      .L_801D8D1C
.L_801D8CD0:
    li       r4, 0x0
    li       r3, 0xf
    stw      r4, 0x18(r31)
    stw      r3, 0x1c(r31)
    stw      r3, 0x20(r31)
    stw      r3, 0x24(r31)
    lwz      r0, 0x0(r27)
    cmpwi    r0, 0x0
    beq      .L_801D8D00
    stw      r4, 0x0(r27)
    stw      r3, 0x28(r31)
    b        .L_801D8D04
.L_801D8D00:
    stw      r4, 0x28(r31)
.L_801D8D04:
    li       r0, 0x0
    stw      r0, 0x2c(r31)
    stw      r0, 0x30(r31)
    stb      r0, 0x34(r31)
    stw      r0, 0x38(r31)
    b        .L_801D8DF0
.L_801D8D1C:
    stw      r3, 0x18(r31)
    lbz      r0, 0x26(r30)
    cmplwi   r0, 0xff
    bne      .L_801D8D30
    li       r0, 0xf
.L_801D8D30:
    stw      r0, 0x1c(r31)
    lbz      r0, 0x2e(r30)
    cmplwi   r0, 0xff
    bne      .L_801D8D44
    li       r0, 0xf
.L_801D8D44:
    stw      r0, 0x20(r31)
    lbz      r0, 0x36(r30)
    cmplwi   r0, 0xff
    bne      .L_801D8D58
    li       r0, 0xf
.L_801D8D58:
    stw      r0, 0x24(r31)
    lbz      r0, 0x3e(r30)
    cmplwi   r0, 0xff
    bne      .L_801D8D6C
    li       r0, 0xf
.L_801D8D6C:
    stw      r0, 0x28(r31)
    lbz      r0, 0x10(r30)
    cmplwi   r0, 0xff
    bne      .L_801D8D80
    li       r0, 0x0
.L_801D8D80:
    stw      r0, 0x2c(r31)
    lbz      r0, 0xf(r30)
    cmplwi   r0, 0xff
    bne      .L_801D8D94
    li       r0, 0x0
.L_801D8D94:
    stw      r0, 0x30(r31)
    lbz      r3, 0xe(r30)
    neg      r0, r3
    or       r0, r0, r3
    srwi     r0, r0, 31
    stb      r0, 0x34(r31)
    lbz      r0, 0xc(r30)
    cmplwi   r0, 0xff
    bne      .L_801D8DC8
    addi     r5, r29, 0x2f8
    li       r3, lbl_8065B038@sda21
    li       r4, 0x54a
    bl       __assert
.L_801D8DC8:
    lbz      r0, 0xc(r30)
    addi     r3, r29, 0x2e8
    slwi     r0, r0, 2
    lwzx     r0, r3, r0
    stw      r0, 0x38(r31)
    lwz      r0, 0x38(r31)
    cmpwi    r0, 0x0
    bne      .L_801D8DF0
    li       r0, 0x0
    stw      r0, 0x0(r27)
.L_801D8DF0:
    lbz      r3, 0x19(r30)
    cmplwi   r3, 0xff
    beq      .L_801D8E08
    lwz      r0, 0x14(r30)
    cmpwi    r0, 0x0
    bne      .L_801D8E54
.L_801D8E08:
    li       r4, 0x0
    li       r3, 0x7
    stw      r4, 0x3c(r31)
    stw      r3, 0x40(r31)
    stw      r3, 0x44(r31)
    stw      r3, 0x48(r31)
    lwz      r0, 0x0(r28)
    cmpwi    r0, 0x0
    beq      .L_801D8E38
    stw      r4, 0x0(r28)
    stw      r3, 0x4c(r31)
    b        .L_801D8E3C
.L_801D8E38:
    stw      r4, 0x4c(r31)
.L_801D8E3C:
    li       r0, 0x0
    stw      r0, 0x50(r31)
    stw      r0, 0x54(r31)
    stb      r0, 0x58(r31)
    stw      r0, 0x5c(r31)
    b        .L_801D8F28
.L_801D8E54:
    stw      r3, 0x3c(r31)
    lbz      r0, 0x46(r30)
    cmplwi   r0, 0xff
    bne      .L_801D8E68
    li       r0, 0x7
.L_801D8E68:
    stw      r0, 0x40(r31)
    lbz      r0, 0x4e(r30)
    cmplwi   r0, 0xff
    bne      .L_801D8E7C
    li       r0, 0x7
.L_801D8E7C:
    stw      r0, 0x44(r31)
    lbz      r0, 0x56(r30)
    cmplwi   r0, 0xff
    bne      .L_801D8E90
    li       r0, 0x7
.L_801D8E90:
    stw      r0, 0x48(r31)
    lbz      r0, 0x5e(r30)
    cmplwi   r0, 0xff
    bne      .L_801D8EA4
    li       r0, 0x7
.L_801D8EA4:
    stw      r0, 0x4c(r31)
    lbz      r0, 0x1c(r30)
    cmplwi   r0, 0xff
    bne      .L_801D8EB8
    li       r0, 0x0
.L_801D8EB8:
    stw      r0, 0x50(r31)
    lbz      r0, 0x1b(r30)
    cmplwi   r0, 0xff
    bne      .L_801D8ECC
    li       r0, 0x0
.L_801D8ECC:
    stw      r0, 0x54(r31)
    lbz      r3, 0x1a(r30)
    neg      r0, r3
    or       r0, r0, r3
    srwi     r0, r0, 31
    stb      r0, 0x58(r31)
    lbz      r0, 0x18(r30)
    cmplwi   r0, 0xff
    bne      .L_801D8F00
    addi     r5, r29, 0x314
    li       r3, lbl_8065B038@sda21
    li       r4, 0x56e
    bl       __assert
.L_801D8F00:
    lbz      r0, 0x18(r30)
    addi     r3, r29, 0x2e8
    slwi     r0, r0, 2
    lwzx     r0, r3, r0
    stw      r0, 0x5c(r31)
    lwz      r0, 0x5c(r31)
    cmpwi    r0, 0x0
    bne      .L_801D8F28
    li       r0, 0x0
    stw      r0, 0x0(r28)
.L_801D8F28:
    li       r0, 0x0
    addi     r11, r1, 0x20
    stw      r0, 0x60(r31)
    bl       _restgpr_27
    lwz      r0, 0x24(r1)
    mtlr     r0
    addi     r1, r1, 0x20
    blr
*/

/**
 * @note Address: 0x801D8F48
 * @note Size: 0x4FC
 */
void HSD_TExpSetReg(HSD_TExp* clist)
{
	int i;
	GXColor reg[8];
	u32 changed;

	changed = 0;

	while (clist != NULL) {
		TEXP_ASSERT(1414, clist->type == HSD_TE_CNST);
		if (clist->cnst.reg < 8) {
			changed |= 1 << clist->cnst.reg;
			if (clist->cnst.comp == HSD_TE_RGB) {
				switch (clist->cnst.ctype) {
				case HSD_TE_U8: {
					GXColor col;
					col                  = *(GXColor*)clist->cnst.val;
					col.a                = reg[clist->cnst.reg].a;
					reg[clist->cnst.reg] = col;
					break;
				}
				case HSD_TE_U32: {
					u32* ptr               = clist->cnst.val;
					reg[clist->cnst.reg].r = ptr[0] < 0x100 ? ptr[0] : 0xFF;
					reg[clist->cnst.reg].g = ptr[1] < 0x100 ? ptr[1] : 0xFF;
					reg[clist->cnst.reg].b = ptr[2] < 0x100 ? ptr[2] : 0xFF;
					break;
				}
				default: {
					int r;
					int g;
					int b;
					switch (clist->cnst.ctype) {
					case HSD_TE_U16: {
						u16* ptr = clist->cnst.val;
						r        = ptr[0];
						g        = ptr[1];
						b        = ptr[2];
						break;
					}
					case HSD_TE_F32: {
						f32* ptr = clist->cnst.val;
						r        = 0xFF * ptr[0];
						g        = 0xFF * ptr[1];
						b        = 0xFF * ptr[2];
						break;
					}
					default: {
						f64* ptr = clist->cnst.val;
						r        = 0xFF * ptr[0];
						g        = 0xFF * ptr[1];
						b        = 0xFF * ptr[2];
						break;
					}
					}
					reg[clist->cnst.reg].r = r > 0xFF ? 0xFF : r < 0 ? 0 : r;
					reg[clist->cnst.reg].g = g > 0xFF ? 0xFF : g < 0 ? 0 : g;
					reg[clist->cnst.reg].b = b > 0xFF ? 0xFF : b < 0 ? 0 : b;
				}
				}
			} else {
				int x;
				u8 val;
				switch (clist->cnst.ctype) {
				case HSD_TE_U8:
					val = *(u8*)clist->cnst.val;
					break;
				case HSD_TE_U16: {
					x   = *(u16*)clist->cnst.val;
					val = x > 0xFF ? 0xFF : x < 0 ? 0 : x;
					break;
				}
				case HSD_TE_U32:
					x   = *(u32*)clist->cnst.val;
					val = x > 0xFF ? 0xFF : x < 0 ? 0 : x;
					break;
				case HSD_TE_F32:
					x   = 0xFF * *(f32*)clist->cnst.val;
					val = x > 0xFF ? 0xFF : x < 0 ? 0 : x;
					break;
				default:
					x   = 0xFF * *(f64*)clist->cnst.val;
					val = x > 0xFF ? 0xFF : x < 0 ? 0 : x;
					break;
				}
				if (clist->cnst.reg < 4) {
					switch (clist->cnst.idx) {
					case 0:
						reg[clist->cnst.reg].r = val;
						break;
					case 1:
						reg[clist->cnst.reg].g = val;
						break;
					case 2:
						reg[clist->cnst.reg].b = val;
						break;
					default:
						reg[clist->cnst.reg].a = val;
						break;
					}
				} else if (clist->cnst.idx == 3) {
					reg[clist->cnst.reg].a = val;
				} else {
					reg[clist->cnst.reg].r = val;
					reg[clist->cnst.reg].g = val;
					reg[clist->cnst.reg].b = val;
				}
			}
		}
		clist = clist->cnst.next;
	}
	if (changed != 0) {
		GXPixModeSync();
		for (i = 0; i < 4; i++) {
			if (changed & (1 << i)) {
				GXSetTevKColor(id1[i], reg[i]);
			}
		}
		for (i = 4; i < 7; i++) {
			if (changed & (1 << i)) {
				GXSetTevColor(id2[i - 4], reg[i]);
			}
		}
		HSD_StateInvalidate(0x10);
		GXPixModeSync();
	}
}

/**
 * @note Address: 0x801D9444
 * @note Size: 0x6C
 */
void HSD_TExpSetupTev(HSD_TExpTevDesc* tevdesc, HSD_TExp* texp)
{
	HSD_TExpSetReg(texp);
	for (; tevdesc != NULL; tevdesc = (HSD_TExpTevDesc*)tevdesc->desc.next) {
		if (tevdesc->tobj != NULL) {
			tevdesc->desc.map   = tevdesc->tobj->map_id;
			tevdesc->desc.coord = tevdesc->tobj->coord;
		}
		HSD_StateAssignTev();
		HSD_SetupTevStage(&tevdesc->desc);
	}
}

/**
 * @note Address: 0x801D94B0
 * @note Size: 0x2A8
 */
int HSD_TExpCompile(HSD_TExp* texp, HSD_TExpTevDesc** tevdesc, HSD_TExp** texp_list)
{
	int num, i, val;
	HSD_TExpRes res;
	HSD_TExp* order[32];
	HSD_TExpDag list[32];
	int init_cprev = 1;
	int init_aprev = 1;

	TEXP_ASSERT(1616, tevdesc);
	TEXP_ASSERT(1617, texp_list);

	memset(&res, 0, sizeof(HSD_TExpRes));

	HSD_TExpRef(texp, HSD_TE_RGB);
	HSD_TExpRef(texp, HSD_TE_A);
	HSD_TExpSimplify(texp);

	num = HSD_TExpMakeDag(texp, list);
	HSD_TExpSchedule(num, list, order, &res);
	for (i = 0; i < num; ++i) {
		val = TExpAssignReg(order[i], &res);
		TEXP_ASSERT(1649, val >= 0);
	}

	for (i = num - 1; i >= 0; --i) {
		HSD_TExpSimplify2(order[i]);
	}

	num = HSD_TExpMakeDag(texp, list);
	HSD_TExpSchedule(num, list, order, &res);
	*tevdesc = NULL;
	for (i = 0; i < num; ++i) {
		HSD_TExpTevDesc* tdesc = hsdAllocMemPiece(sizeof(HSD_TExpTevDesc));
		tdesc->desc.stage      = HSD_Index2TevStage(i);
		TExp2TevDesc(order[(num - i) - 1], tdesc, &init_cprev, &init_aprev);
		tdesc->desc.next = &(*tevdesc)->desc;
		*tevdesc         = tdesc;
	}

	*texp_list = HSD_TExpFreeList(*texp_list, HSD_TE_TEV, 1);
	*texp_list = HSD_TExpFreeList(*texp_list, HSD_TE_CNST, 0);
	return num;
}

/**
 * @note Address: 0x801D9758
 * @note Size: 0x40
 */
void HSD_TExpFreeTevDesc(HSD_TExpTevDesc* tdesc)
{
	HSD_TExpTevDesc* next = tdesc;
	while (next != NULL) {
		HSD_TExpTevDesc* temp = (HSD_TExpTevDesc*)next->desc.next;
		hsdFreeMemPiece(next, sizeof(HSD_TExpTevDesc));
		next = temp;
	}
}
