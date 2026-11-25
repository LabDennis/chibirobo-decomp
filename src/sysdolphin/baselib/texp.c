#include <global.h>
#include <dolphin/gx/gx.h>
#include <sysdolphin/class.h>
#include <sysdolphin/class_alloc.h>
#include <sysdolphin/gobjproc.h>
#include <sysdolphin/tev.h>
#include <sysdolphin/texp_internal.h>
#include <sysdolphin/tobj.h>
#include <string.h>

#define TEXP_ASSERT(line, cond) ((cond) ? ((void)0) : __assert("texp.c", line, #cond))

/**
 * @note Address: 0x801D65E0
 * @note Size: 0x3C
 */
HSD_TExpType HSD_TExpGetType(HSD_TExp* texp)
{
	if (texp == NULL) {
		return HSD_TE_ZERO;
	}
	if (texp == HSD_TEXP_TEX) {
		return HSD_TE_TEX;
	}
	if (texp == HSD_TEXP_RAS) {
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
		if (sel == HSD_TE_RGB) {
			texp->tev.c_ref += 1;
		} else {
			texp->tev.a_ref += 1;
		}
		break;
	case HSD_TE_CNST:
		texp->cnst.ref += 1;
		break;
	}
}

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

/**
 * @note Address: 0x801D6C20
 * @note Size: 0x320
 */
HSD_TExp* HSD_TExpFreeList(HSD_TExp* texp_list, HSD_TExpType type, s32 all)
{
	HSD_TExp** link;
	HSD_TExp* node;

	if (!all && (type == HSD_TE_ALL || type == HSD_TE_TEV)) {
		for (node = texp_list; node != NULL; node = node->comm.next) {
			// Retail releases both references when the color reference count reaches zero.
			if (node->type == HSD_TE_TEV && node->tev.c_ref == 0) {
				HSD_TExpUnref(node, HSD_TE_RGB);
				HSD_TExpUnref(node, HSD_TE_A);
			}
		}
	}

	link = &texp_list;
	while ((node = *link) != NULL) {
		if (type == HSD_TE_ALL || type == node->type) {
			s32 size = 0;

			switch (node->type) {
			case HSD_TE_TEV:
				if (all || (node->tev.c_ref == 0 && node->tev.a_ref == 0)) {
					size = sizeof(HSD_TETev);
				}
				break;
			case HSD_TE_CNST:
				if (all || node->cnst.ref == 0) {
					size = sizeof(HSD_TECnst);
				}
				break;
			default:
				TEXP_ASSERT(all ? 220 : 268, 0);
				break;
			}

			if (size != 0) {
				HSD_TExp* next = node->comm.next;
				hsdFreeMemPiece(node, size);
				*link = next;
				continue;
			}
		}
		link = &node->comm.next;
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
	memset(texp, HSD_TE_UNDEF, sizeof(HSD_TETev));
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
	while (texp != NULL) {
		if (texp->type == HSD_TE_CNST && texp->cnst.val == val && texp->cnst.comp == comp) {
			TEXP_ASSERT(369, texp->cnst.ctype == type);
			return texp;
		}
		texp = texp->comm.next;
	}

	if (comp == HSD_TE_0) {
		return NULL;
	}
	texp             = CnstAlloc();
	texp->cnst.type  = HSD_TE_CNST;
	texp->cnst.next  = *texp_list;
	*texp_list       = texp;
	texp->cnst.ref   = 0;
	texp->cnst.val   = val;
	texp->cnst.comp  = comp;
	texp->cnst.ctype = type;
	texp->cnst.reg   = HSD_TE_UNDEF;
	texp->cnst.idx   = HSD_TE_UNDEF;
	texp->cnst.range = 0;
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
static void HSD_TExpColorInSub(HSD_TETev* tev, HSD_TEInput sel, HSD_TExp* exp, s32 idx)
{
	HSD_TEArg prev;
	u8 ksel;

	prev               = tev->c_in[idx];
	tev->c_in[idx].type = HSD_TExpGetType(exp);
	tev->c_in[idx].sel  = sel;
	tev->c_in[idx].exp  = exp;
	tev->c_in[idx].arg  = HSD_TE_UNDEF;

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

	default:
		switch (tev->c_in[idx].type) {
		case HSD_TE_ZERO:
			tev->c_in[idx].type = HSD_TE_ZERO;
			tev->c_in[idx].sel  = HSD_TE_0;
			tev->c_in[idx].arg  = GX_CC_ZERO;
			break;
		case HSD_TE_TEV:
			TEXP_ASSERT(521, sel == HSD_TE_RGB || sel == HSD_TE_A);
			TEXP_ASSERT(523, idx == 3 || sel != HSD_TE_RGB || exp->tev.c_clamp);
			TEXP_ASSERT(524, idx == 3 || sel != HSD_TE_A || exp->tev.a_clamp);
			HSD_TExpRef(tev->c_in[idx].exp, tev->c_in[idx].sel);
			break;
		case HSD_TE_CNST:
			tev->c_in[idx].sel = exp->cnst.comp;
			HSD_TExpRef(tev->c_in[idx].exp, tev->c_in[idx].sel);
			break;
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
		break;
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
static void HSD_TExpAlphaInSub(HSD_TETev* tev, HSD_TEInput sel, HSD_TExp* exp, s32 idx)
{
	HSD_TEArg prev;
	u8 ksel;

	prev               = tev->a_in[idx];
	tev->a_in[idx].sel  = sel;
	tev->a_in[idx].exp  = exp;
	tev->a_in[idx].type = HSD_TExpGetType(exp);
	tev->a_in[idx].arg  = HSD_TE_UNDEF;

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

	default:
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
		break;
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
static int AssignColorReg(HSD_TETev* tev, int idx, HSD_TExpRes* res)
{
	static GXTevColorArg color_input_alpha_regs[4] = { GX_CC_A0, GX_CC_A1, GX_CC_A2, GX_CC_APREV };
	static GXTevColorArg color_input_rgb_regs[4] = { GX_CC_C0, GX_CC_C1, GX_CC_C2, GX_CC_CPREV };

	HSD_TECnst* cnst;
	int j;

	cnst = &tev->c_in[idx].exp->cnst;
	if (cnst->reg != HSD_TE_UNDEF) {
		if (cnst->reg < 4) {
			return -1;
		}
		tev->c_in[idx].type = HSD_TE_IMM;
		if (cnst->comp == HSD_TE_X) {
			tev->c_in[idx].arg = color_input_alpha_regs[cnst->reg - 4];
		} else {
			tev->c_in[idx].arg = color_input_rgb_regs[cnst->reg - 4];
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
					tev->c_in[idx].arg  = color_input_alpha_regs[j - 4];
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
					tev->c_in[idx].arg  = color_input_rgb_regs[j - 4];
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
	static GXTevAlphaArg alpha_input_regs[4] = { GX_CA_A0, GX_CA_A1, GX_CA_A2, GX_CA_APREV };

	HSD_TECnst* cnst;
	int j;

	cnst = &tev->a_in[idx].exp->cnst;
	if (cnst->reg != HSD_TE_UNDEF) {
		if (cnst->reg < 4) {
			return -1;
		}
		tev->a_in[idx].type = HSD_TE_IMM;
		tev->a_in[idx].arg  = alpha_input_regs[cnst->reg - 4];
		return 0;
	} else {
		for (j = 4; j < 8; ++j) {
			if (res->reg[j].alpha == 0) {
				res->reg[j].alpha   = 1;
				cnst->reg           = j;
				cnst->idx           = 3;
				tev->a_in[idx].type = HSD_TE_IMM;
				tev->a_in[idx].arg  = alpha_input_regs[j - 4];
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
static int AssignColorKonst(HSD_TETev* tev, int idx, HSD_TExpRes* res)
{
	static GXTevKColorSel component_selectors[4][4] = {
		{ GX_TEV_KCSEL_K0_R, GX_TEV_KCSEL_K0_G, GX_TEV_KCSEL_K0_B, GX_TEV_KCSEL_K0_A },
		{ GX_TEV_KCSEL_K1_R, GX_TEV_KCSEL_K1_G, GX_TEV_KCSEL_K1_B, GX_TEV_KCSEL_K1_A },
		{ GX_TEV_KCSEL_K2_R, GX_TEV_KCSEL_K2_G, GX_TEV_KCSEL_K2_B, GX_TEV_KCSEL_K2_A },
		{ GX_TEV_KCSEL_K3_R, GX_TEV_KCSEL_K3_G, GX_TEV_KCSEL_K3_B, GX_TEV_KCSEL_K3_A },
	};

	static GXTevKColorSel rgb_selectors[4] = { GX_TEV_KCSEL_K0, GX_TEV_KCSEL_K1, GX_TEV_KCSEL_K2, GX_TEV_KCSEL_K3 };

	HSD_TECnst* cnst;
	int j;

	cnst = &tev->c_in[idx].exp->cnst;
	if (cnst->reg != HSD_TE_UNDEF) {
		if (cnst->reg >= 4) {
			return -1;
		}
		if (cnst->comp == HSD_TE_X) {
			tev->kcsel          = component_selectors[cnst->reg][cnst->idx];
			tev->c_in[idx].type = HSD_TE_KONST;
			tev->c_in[idx].arg  = GX_CC_KONST;
		} else {
			tev->kcsel          = rgb_selectors[cnst->reg];
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
					tev->kcsel          = component_selectors[cnst->reg][cnst->idx];
					tev->c_in[idx].type = HSD_TE_KONST;
					tev->c_in[idx].arg  = GX_CC_KONST;
					return 0;
				}
			}

			for (j = 0; j < 4; j++) {
				if (res->reg[j].color < 3) {
					cnst->reg           = j;
					cnst->idx           = res->reg[j].color++;
					tev->kcsel          = component_selectors[cnst->reg][cnst->idx];
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
					tev->kcsel          = rgb_selectors[cnst->reg];
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
static int AssignAlphaKonst(HSD_TETev* tev, int idx, HSD_TExpRes* res)
{
	static GXTevKAlphaSel component_selectors[4][4] = {
		{ GX_TEV_KASEL_K0_R, GX_TEV_KASEL_K0_G, GX_TEV_KASEL_K0_B, GX_TEV_KASEL_K0_A },
		{ GX_TEV_KASEL_K1_R, GX_TEV_KASEL_K1_G, GX_TEV_KASEL_K1_B, GX_TEV_KASEL_K1_A },
		{ GX_TEV_KASEL_K2_R, GX_TEV_KASEL_K2_G, GX_TEV_KASEL_K2_B, GX_TEV_KASEL_K2_A },
		{ GX_TEV_KASEL_K3_R, GX_TEV_KASEL_K3_G, GX_TEV_KASEL_K3_B, GX_TEV_KASEL_K3_A },
	};

	HSD_TECnst* cnst;
	int j;

	cnst = &tev->a_in[idx].exp->cnst;
	if (cnst->reg != HSD_TE_UNDEF) {
		if (cnst->reg >= 4) {
			return -1;
		}
		tev->kasel          = component_selectors[cnst->reg][cnst->idx];
		tev->a_in[idx].type = HSD_TE_KONST;
		tev->a_in[idx].arg  = GX_CA_KONST;
		return 0;
	} else {
		for (j = 1; j < 4; j++) {
			if (res->reg[j].alpha == 0) {
				res->reg[j].alpha   = 1;
				cnst->reg           = j;
				cnst->idx           = 3;
				tev->kasel          = component_selectors[cnst->reg][cnst->idx];
				tev->a_in[idx].type = HSD_TE_KONST;
				tev->a_in[idx].arg  = GX_CA_KONST;
				return 0;
			}
		}

		for (j = 0; j < 4; j++) {
			if (res->reg[j].color < 3) {
				cnst->reg           = j;
				cnst->idx           = res->reg[j].color++;
				tev->kasel          = component_selectors[cnst->reg][cnst->idx];
				tev->a_in[idx].type = HSD_TE_KONST;
				tev->a_in[idx].arg  = GX_CA_KONST;
				return 0;
			}
		}
	}
	return -1;
}

/**
 * @note Address: 0x801D847C
 * @note Size: 0x6CC
 */
static int TExpAssignReg(HSD_TExp* texp, HSD_TExpRes* res)
{
	HSD_TETev* tev = &texp->tev;
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
		} else if (IsThroughColor(texp) && tev->c_in[3].type == HSD_TE_CNST) {
			if (AssignColorReg(tev, 3, res) < 0) {
				val = AssignColorKonst(tev, 3, res);
				if (val < 0) {
					TEXP_ASSERT(1161, val >= 0);
					return val;
				}
			}
		} else {
			for (i = 0; i < 4; i++) {
				if (tev->c_in[i].type == HSD_TE_CNST && AssignColorKonst(tev, i, res) < 0) {
					val = AssignColorReg(tev, i, res);
					if (val < 0) {
						TEXP_ASSERT(1174, val >= 0);
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
		} else if (IsThroughAlpha(texp) && tev->a_in[3].type == HSD_TE_CNST) {
			if (AssignAlphaReg(tev, 3, res) < 0) {
				val = AssignAlphaKonst(tev, 3, res);
				if (val < 0) {
					TEXP_ASSERT(1211, val >= 0);
					return val;
				}
			}
		} else {
			for (i = 0; i < 4; i++) {
				if (tev->a_in[i].type == HSD_TE_CNST && AssignAlphaKonst(tev, i, res) < 0) {
					val = AssignAlphaReg(tev, i, res);
					if (val < 0) {
						TEXP_ASSERT(1224, val >= 0);
						return val;
					}
				}
			}
		}
	}
	return 0;
}

static u32 TExpDefault(u32 value, u32 fallback)
{
	if (value == HSD_TE_UNDEF) {
		value = fallback;
	}
	return value;
}

/**
 * @note Address: 0x801D8B48
 * @note Size: 0x400
 */
static void TExp2TevDesc(HSD_TExp* texp, HSD_TExpTevDesc* desc, BOOL* init_cprev, BOOL* init_aprev)
{
	static GXTevRegID dst[4] = { GX_TEVREG0, GX_TEVREG1, GX_TEVREG2, GX_TEVPREV };

	HSD_TETev* tev;
	HSD_TevDesc* tevdesc;
	HSD_TevConf* conf;

	TEXP_ASSERT(1289, texp);
	TEXP_ASSERT(1290, desc);
	TEXP_ASSERT(1291, HSD_TExpGetType(texp) == HSD_TE_TEV);

	tev     = &texp->tev;
	tevdesc = &desc->desc;
	conf    = &tevdesc->u.tevconf;

	tevdesc->next  = NULL;
	tevdesc->flags = 1;
	desc->tobj     = tev->tex;
	if (tev->tex == NULL) {
		tevdesc->coord = GX_TEXCOORD_NULL;
		tevdesc->map   = GX_TEXMAP_NULL;
	}
	tevdesc->color = TExpDefault(tev->chan, GX_COLOR_NULL);

	conf->ras_swap = TExpDefault(tev->ras_swap, GX_TEV_SWAP0);
	conf->tex_swap = TExpDefault(tev->tex_swap, GX_TEV_SWAP0);
	conf->kcsel    = TExpDefault(tev->kcsel, GX_TEV_KCSEL_1);
	conf->kasel    = TExpDefault(tev->kasel, GX_TEV_KASEL_1);

	// The R8, GR16 and BGR24 alpha comparisons also use the color inputs.
	if ((tev->c_op == HSD_TE_UNDEF)
	    || (tev->c_ref == 0 && tev->a_op != GX_TEV_COMP_R8_GT && tev->a_op != GX_TEV_COMP_R8_EQ
	        && tev->a_op != GX_TEV_COMP_GR16_GT && tev->a_op != GX_TEV_COMP_GR16_EQ
	        && tev->a_op != GX_TEV_COMP_BGR24_GT && tev->a_op != GX_TEV_COMP_BGR24_EQ)) {
		conf->clr_op = GX_TEV_ADD;
		conf->clr_a  = GX_CC_ZERO;
		conf->clr_b  = GX_CC_ZERO;
		conf->clr_c  = GX_CC_ZERO;
		if (*init_cprev) {
			*init_cprev = FALSE;
			conf->clr_d = GX_CC_ZERO;
		} else {
			conf->clr_d = GX_CC_CPREV;
		}
		conf->clr_scale   = GX_CS_SCALE_1;
		conf->clr_bias    = GX_TB_ZERO;
		conf->clr_clamp   = GX_FALSE;
		conf->clr_out_reg = GX_TEVPREV;
	} else {
		conf->clr_op    = tev->c_op;
		conf->clr_a     = TExpDefault(tev->c_in[0].arg, GX_CC_ZERO);
		conf->clr_b     = TExpDefault(tev->c_in[1].arg, GX_CC_ZERO);
		conf->clr_c     = TExpDefault(tev->c_in[2].arg, GX_CC_ZERO);
		conf->clr_d     = TExpDefault(tev->c_in[3].arg, GX_CC_ZERO);
		conf->clr_scale = TExpDefault(tev->c_scale, GX_CS_SCALE_1);
		conf->clr_bias  = TExpDefault(tev->c_bias, GX_TB_ZERO);
		conf->clr_clamp = tev->c_clamp != 0 ? GX_TRUE : GX_FALSE;

		TEXP_ASSERT(1354, tev->c_dst != HSD_TE_UNDEF);
		conf->clr_out_reg = dst[tev->c_dst];
		if ((GXTevRegID)conf->clr_out_reg == GX_TEVPREV) {
			*init_cprev = FALSE;
		}
	}

	if (tev->a_op == HSD_TE_UNDEF || tev->a_ref == 0) {
		conf->alpha_op = GX_TEV_ADD;
		conf->alpha_a  = GX_CA_ZERO;
		conf->alpha_b  = GX_CA_ZERO;
		conf->alpha_c  = GX_CA_ZERO;
		if (*init_aprev) {
			*init_aprev  = FALSE;
			conf->alpha_d = GX_CA_ZERO;
		} else {
			conf->alpha_d = GX_CA_APREV;
		}
		conf->alpha_scale   = GX_CS_SCALE_1;
		conf->alpha_bias    = GX_TB_ZERO;
		conf->alpha_clamp   = GX_FALSE;
		conf->alpha_out_reg = GX_TEVPREV;
	} else {
		conf->alpha_op    = tev->a_op;
		conf->alpha_a     = TExpDefault(tev->a_in[0].arg, GX_CA_ZERO);
		conf->alpha_b     = TExpDefault(tev->a_in[1].arg, GX_CA_ZERO);
		conf->alpha_c     = TExpDefault(tev->a_in[2].arg, GX_CA_ZERO);
		conf->alpha_d     = TExpDefault(tev->a_in[3].arg, GX_CA_ZERO);
		conf->alpha_scale = TExpDefault(tev->a_scale, GX_CS_SCALE_1);
		conf->alpha_bias  = TExpDefault(tev->a_bias, GX_TB_ZERO);
		conf->alpha_clamp = tev->a_clamp != 0 ? GX_TRUE : GX_FALSE;

		TEXP_ASSERT(1390, tev->a_dst != HSD_TE_UNDEF);
		conf->alpha_out_reg = dst[tev->a_dst];
		if ((GXTevRegID)conf->alpha_out_reg == GX_TEVPREV) {
			*init_aprev = FALSE;
		}
	}
	conf->mode = 0;
}

static GXTevKColorID kcolor_ids[4] = { GX_KCOLOR0, GX_KCOLOR1, GX_KCOLOR2, GX_KCOLOR3 };
static GXTevRegID tev_register_ids[3] = { GX_TEVREG0, GX_TEVREG1, GX_TEVREG2 };

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
					col                  = *(const GXColor*)clist->cnst.val;
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
				GXSetTevKColor(kcolor_ids[i], reg[i]);
			}
		}
		for (i = 4; i < 7; i++) {
			if (changed & (1 << i)) {
				GXSetTevColor(tev_register_ids[i - 4], reg[i]);
			}
		}
		HSD_StateInvalidate(HSD_STATE_TEV_REGISTER);
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
	HSD_TExp* order[HSD_TEXP_MAX_NUM];
	HSD_TExpDag list[HSD_TEXP_MAX_NUM];
	BOOL init_cprev = TRUE;
	BOOL init_aprev = TRUE;

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
		tdesc->desc.next = (HSD_TevDesc*)*tevdesc;
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
	while (tdesc != NULL) {
		HSD_TExpTevDesc* next = (HSD_TExpTevDesc*)tdesc->desc.next;
		hsdFreeMemPiece(tdesc, sizeof(*tdesc));
		tdesc = next;
	}
}
