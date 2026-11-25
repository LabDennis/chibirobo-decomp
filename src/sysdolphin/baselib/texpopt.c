#include <global.h>
#include <sysdolphin/texp_internal.h>

static HSD_TEArg zero_arg = { HSD_TE_ZERO, HSD_TE_0, HSD_TE_UNDEF, NULL };

/**
 * @note Address: 0x801DA7D0
 * @note Size: 0x498
 */
int SimplifySrc(HSD_TExp* texp)
{
	int i;
	bool result;

	result = 0;
	i      = 0;

	for (; i < 4; i++) {
		if (texp->tev.c_in[i].type == HSD_TE_TEV) {
			HSD_TExp* src = texp->tev.c_in[i].exp;
			u8 sel        = texp->tev.c_in[i].sel;
			if (HSD_TExpSimplify(src) != 0) {
				result = 1;
			}
			if (sel == HSD_TE_RGB) {
				switch (src->tev.c_op) {
				case HSD_TE_UNDEF:
					HSD_TExpUnref(src, sel);
					result            = 1;
					texp->tev.c_in[i] = zero_arg;
					break;
				case GX_TEV_ADD:
					if (src->tev.c_in[0].sel == HSD_TE_0 && src->tev.c_in[1].sel == HSD_TE_0 && src->tev.c_bias == GX_TB_ZERO
					    && src->tev.c_scale == GX_CS_SCALE_1) {
						switch (src->tev.c_in[3].type) {
						case HSD_TE_IMM:
							texp->tev.c_in[i] = src->tev.c_in[3];
							HSD_TExpRef(texp->tev.c_in[i].exp, texp->tev.c_in[i].sel);
							HSD_TExpUnref(src, sel);
							result = 1;
							break;
						case HSD_TE_TEV:
							if (i == 3 && src->tev.c_clamp != GX_FALSE && src->tev.c_range == 1)
								break;
							texp->tev.c_in[i] = src->tev.c_in[3];
							HSD_TExpRef(texp->tev.c_in[i].exp, texp->tev.c_in[i].sel);
							HSD_TExpUnref(src, sel);
							result = 1;
							break;
						case HSD_TE_TEX:
							if ((texp->tev.tex == NULL || texp->tev.tex == src->tev.tex)
							    && (texp->tev.tex_swap == HSD_TE_UNDEF || src->tev.tex_swap == HSD_TE_UNDEF
							        || texp->tev.tex_swap == src->tev.tex_swap)) {
								texp->tev.c_in[i] = src->tev.c_in[3];
								texp->tev.tex     = src->tev.tex;
								if (texp->tev.tex_swap == HSD_TE_UNDEF) {
									texp->tev.tex_swap = src->tev.tex_swap;
								}
								HSD_TExpUnref(src, sel);
								result = 1;
							}
							break;
						case HSD_TE_RAS:
							if ((texp->tev.chan == HSD_TE_UNDEF || texp->tev.chan == src->tev.chan)
							    && (texp->tev.ras_swap == HSD_TE_UNDEF || src->tev.ras_swap == HSD_TE_UNDEF
							        || texp->tev.ras_swap == src->tev.ras_swap)) {
								texp->tev.c_in[i] = src->tev.c_in[3];
								texp->tev.chan    = src->tev.chan;
								if (texp->tev.ras_swap == HSD_TE_UNDEF) {
									texp->tev.ras_swap = src->tev.ras_swap;
								}
								HSD_TExpUnref(src, sel);
								result = 1;
							}
							break;
						}
					}
					break;
				}
			} else {
				switch (src->tev.a_op) {
				case GX_TEV_ADD:
					break;
				case HSD_TE_UNDEF:
					HSD_TExpUnref(src, sel);
					result            = 1;
					texp->tev.c_in[i] = zero_arg;
					break;
				}
			}
		}
	}

	for (i = 0; i < 4; i++) {
		if (texp->tev.a_in[i].type == HSD_TE_TEV) {
			HSD_TExp* src = texp->tev.a_in[i].exp;
			u8 sel        = texp->tev.a_in[i].sel;
			HSD_TExpSimplify(src);
			switch (src->tev.a_op) {
			case HSD_TE_UNDEF:
				HSD_TExpUnref(src, sel);
				result            = 1;
				texp->tev.a_in[i] = zero_arg;
				break;
			case GX_TEV_ADD:
				if (src->tev.a_in[0].sel == HSD_TE_0 && src->tev.a_in[1].sel == HSD_TE_0 && src->tev.a_bias == GX_TB_ZERO
				    && src->tev.a_scale == GX_CS_SCALE_1) {
					switch (src->tev.a_in[3].type) {
					case HSD_TE_IMM:
						texp->tev.a_in[i] = src->tev.a_in[3];
						HSD_TExpRef(texp->tev.a_in[i].exp, texp->tev.a_in[i].sel);
						HSD_TExpUnref(src, sel);
						result = 1;
						/* fall through */
					case HSD_TE_TEV:
						if (i == 3 && src->tev.a_clamp != GX_FALSE && src->tev.a_range == 1)
							break;
						texp->tev.a_in[i] = src->tev.a_in[3];
						HSD_TExpRef(texp->tev.a_in[i].exp, texp->tev.a_in[i].sel);
						HSD_TExpUnref(src, sel);
						result = 1;
						break;
					case HSD_TE_TEX:
						if (texp->tev.tex == NULL || texp->tev.tex == src->tev.tex) {
							texp->tev.a_in[i] = src->tev.a_in[3];
							texp->tev.tex     = src->tev.tex;
							HSD_TExpUnref(src, sel);
							result = 1;
						}
						break;
					case HSD_TE_RAS:
						if (texp->tev.chan == HSD_TE_UNDEF || texp->tev.chan == src->tev.chan) {
							texp->tev.a_in[i] = src->tev.a_in[3];
							texp->tev.chan    = src->tev.chan;
							HSD_TExpUnref(src, sel);
							result = 1;
						}
						break;
					}
				}
				break;
			}
		}
	}
	return result;
}

/**
 * @note Address: 0x801DAC68
 * @note Size: 0x840
 */
int SimplifyThis(HSD_TExp* texp)
{
	int i;
	int color_tex;
	int alpha_tex;
	int color_ras;
	int alpha_ras;
	int result;
	int changed;
	int op;

	result = 0;
	do {
		color_tex = -1;
		alpha_tex = -1;
		color_ras = -1;
		alpha_ras = -1;

		for (i = 0; i < 4; i++) {
			switch (texp->tev.c_in[i].type) {
			case HSD_TE_TEX:
				color_tex = i;
				break;
			case HSD_TE_RAS:
				color_ras = i;
				break;
			}
			switch (texp->tev.a_in[i].type) {
			case HSD_TE_TEX:
				alpha_tex = i;
				break;
			case HSD_TE_RAS:
				alpha_ras = i;
				break;
			}
		}

		if (color_tex == -1 && alpha_tex == -1) {
			texp->tev.tex      = NULL;
			texp->tev.tex_swap = HSD_TE_UNDEF;
		}
		if (color_ras == -1 && alpha_ras == -1) {
			texp->tev.chan     = HSD_TE_UNDEF;
			texp->tev.ras_swap = HSD_TE_UNDEF;
		}

		changed = 0;
		if (texp->tev.a_op == HSD_TE_UNDEF || texp->tev.a_op == GX_TEV_COMP_A8_GT || texp->tev.a_op == GX_TEV_COMP_A8_EQ
		    || texp->tev.a_op == GX_TEV_ADD || texp->tev.a_op == GX_TEV_SUB) {
			if (texp->tev.c_op != HSD_TE_UNDEF && texp->tev.c_ref == 0) {
				texp->tev.c_op = HSD_TE_UNDEF;
				for (i = 0; i < 4; i++) {
					HSD_TExpUnref(texp->tev.c_in[i].exp, texp->tev.c_in[i].sel);
					texp->tev.c_in[i] = zero_arg;
				}
				changed = 1;
			}

			switch (texp->tev.c_op) {
			case GX_TEV_ADD:
			case GX_TEV_SUB:
				if (texp->tev.c_in[2].sel == HSD_TE_0) {
					if (texp->tev.c_in[1].sel != HSD_TE_0) {
						HSD_TExpUnref(texp->tev.c_in[1].exp, texp->tev.c_in[1].sel);
						changed = 1;
						texp->tev.c_in[1] = zero_arg;
					}
					if (texp->tev.c_op == GX_TEV_ADD && texp->tev.c_in[3].sel == HSD_TE_0) {
						changed           = 1;
						texp->tev.c_in[3] = texp->tev.c_in[0];
						texp->tev.c_in[0] = zero_arg;
						texp->tev.c_clamp = GX_TRUE;
					}
				}
				if (texp->tev.c_in[2].sel == HSD_TE_1) {
					if (texp->tev.c_in[0].sel != HSD_TE_0) {
						HSD_TExpUnref(texp->tev.c_in[0].exp, texp->tev.c_in[0].sel);
						changed = 1;
						texp->tev.c_in[0] = zero_arg;
					}
					if (texp->tev.c_op == GX_TEV_ADD && texp->tev.c_in[3].sel == HSD_TE_0) {
						changed           = 1;
						texp->tev.c_in[3] = texp->tev.c_in[1];
						texp->tev.c_in[1] = zero_arg;
						texp->tev.c_in[2] = zero_arg;
					}
				}
				if (texp->tev.c_in[0].sel == HSD_TE_0 && texp->tev.c_in[1].sel == HSD_TE_1) {
					texp->tev.c_in[0] = texp->tev.c_in[2];
					texp->tev.c_in[1] = zero_arg;
					texp->tev.c_in[2] = zero_arg;
					changed = 1;
				}
				if (texp->tev.c_in[0].sel == HSD_TE_0 && texp->tev.c_in[1].sel == HSD_TE_0 && texp->tev.c_in[3].sel == HSD_TE_0
				    && texp->tev.c_bias == GX_TB_ZERO) {
					texp->tev.c_op = HSD_TE_UNDEF;
					HSD_TExpUnref(texp->tev.c_in[2].exp, texp->tev.c_in[2].sel);
					changed = 1;
					texp->tev.c_in[2] = zero_arg;
				}
				break;
			case GX_TEV_COMP_R8_GT:
			case GX_TEV_COMP_GR16_GT:
			case GX_TEV_COMP_BGR24_GT:
			case GX_TEV_COMP_RGB8_GT:
				if (texp->tev.c_in[2].sel == HSD_TE_0) {
					texp->tev.c_op = GX_TEV_ADD;
					HSD_TExpUnref(texp->tev.c_in[0].exp, texp->tev.c_in[0].sel);
					texp->tev.c_in[0] = zero_arg;
					HSD_TExpUnref(texp->tev.c_in[1].exp, texp->tev.c_in[1].sel);
					changed = 1;
					texp->tev.c_in[1] = zero_arg;
				} else if (texp->tev.c_in[0].sel == HSD_TE_0) {
					texp->tev.c_op = GX_TEV_ADD;
					HSD_TExpUnref(texp->tev.c_in[1].exp, texp->tev.c_in[1].sel);
					texp->tev.c_in[1] = zero_arg;
					HSD_TExpUnref(texp->tev.c_in[2].exp, texp->tev.c_in[2].sel);
					changed = 1;
					texp->tev.c_in[2] = zero_arg;
				}
				break;
			case GX_TEV_COMP_R8_EQ:
			case GX_TEV_COMP_GR16_EQ:
			case GX_TEV_COMP_BGR24_EQ:
			case GX_TEV_COMP_RGB8_EQ:
				if (texp->tev.c_in[2].sel == HSD_TE_0) {
					texp->tev.c_op = GX_TEV_ADD;
					HSD_TExpUnref(texp->tev.c_in[0].exp, texp->tev.c_in[0].sel);
					texp->tev.c_in[0] = zero_arg;
					HSD_TExpUnref(texp->tev.c_in[1].exp, texp->tev.c_in[1].sel);
					changed = 1;
					texp->tev.c_in[1] = zero_arg;
				} else if (texp->tev.c_in[0].sel == HSD_TE_0 && texp->tev.c_in[1].sel == HSD_TE_0) {
					texp->tev.c_op    = GX_TEV_ADD;
					changed           = 1;
					texp->tev.c_in[0] = texp->tev.c_in[2];
					texp->tev.c_in[2] = zero_arg;
				}
				break;
			}
		}

		if (texp->tev.a_op != HSD_TE_UNDEF && texp->tev.a_ref == 0) {
			texp->tev.a_op = HSD_TE_UNDEF;
			for (i = 0; i < 4; i++) {
				HSD_TExpUnref(texp->tev.a_in[i].exp, texp->tev.a_in[i].sel);
				texp->tev.a_in[i] = zero_arg;
			}
			changed = 1;
		}

		switch (texp->tev.a_op) {
		case GX_TEV_ADD:
		case GX_TEV_SUB:
			if (texp->tev.a_in[2].sel == HSD_TE_0) {
				if (texp->tev.a_in[1].sel != HSD_TE_0) {
					HSD_TExpUnref(texp->tev.a_in[1].exp, texp->tev.a_in[1].sel);
					changed = 1;
					texp->tev.a_in[1] = zero_arg;
				}
				if (texp->tev.a_op == GX_TEV_ADD && texp->tev.a_in[3].sel == HSD_TE_0) {
					changed           = 1;
					texp->tev.a_in[3] = texp->tev.a_in[0];
					texp->tev.a_in[0] = zero_arg;
				}
			}
			if (texp->tev.a_in[2].sel == HSD_TE_1) {
				if (texp->tev.a_in[0].sel != HSD_TE_0) {
					HSD_TExpUnref(texp->tev.a_in[0].exp, texp->tev.a_in[0].sel);
					changed = 1;
					texp->tev.a_in[0] = zero_arg;
				}
				if (texp->tev.a_op == GX_TEV_ADD && texp->tev.a_in[3].sel == HSD_TE_0) {
					changed           = 1;
					texp->tev.a_in[3] = texp->tev.a_in[1];
					texp->tev.a_in[1] = zero_arg;
					texp->tev.a_in[2] = zero_arg;
				}
			}
			if (texp->tev.a_in[0].sel == HSD_TE_0 && texp->tev.a_in[1].sel == HSD_TE_0 && texp->tev.a_in[3].sel == HSD_TE_0) {
				texp->tev.a_op = HSD_TE_UNDEF;
				changed        = 1;
			}
			break;

		case GX_TEV_COMP_R8_GT:
		case GX_TEV_COMP_R8_EQ:
		case GX_TEV_COMP_GR16_GT:
		case GX_TEV_COMP_GR16_EQ:
		case GX_TEV_COMP_BGR24_GT:
		case GX_TEV_COMP_BGR24_EQ:
			if (texp->tev.a_in[2].sel == HSD_TE_0) {
				texp->tev.a_op = GX_TEV_ADD;
				HSD_TExpUnref(texp->tev.a_in[0].exp, texp->tev.a_in[0].sel);
				texp->tev.a_in[0] = zero_arg;
				HSD_TExpUnref(texp->tev.a_in[1].exp, texp->tev.a_in[1].sel);
				changed = 1;
				texp->tev.a_in[1] = zero_arg;
			}
			break;

		case GX_TEV_COMP_A8_GT:
			if (texp->tev.a_in[2].sel == HSD_TE_0) {
				texp->tev.a_op = GX_TEV_ADD;
				HSD_TExpUnref(texp->tev.a_in[0].exp, texp->tev.a_in[0].sel);
				texp->tev.a_in[0] = zero_arg;
				HSD_TExpUnref(texp->tev.a_in[1].exp, texp->tev.a_in[1].sel);
				changed = 1;
				texp->tev.a_in[1] = zero_arg;
			} else if (texp->tev.a_in[0].sel == HSD_TE_0) {
				texp->tev.a_op = GX_TEV_ADD;
				HSD_TExpUnref(texp->tev.a_in[1].exp, texp->tev.a_in[1].sel);
				texp->tev.a_in[1] = zero_arg;
				HSD_TExpUnref(texp->tev.a_in[2].exp, texp->tev.a_in[2].sel);
				changed = 1;
				texp->tev.a_in[2] = zero_arg;
			}
			break;

		case GX_TEV_COMP_A8_EQ:
			if (texp->tev.a_in[2].sel == HSD_TE_0) {
				texp->tev.a_op = GX_TEV_ADD;
				HSD_TExpUnref(texp->tev.a_in[0].exp, texp->tev.a_in[0].sel);
				texp->tev.a_in[0] = zero_arg;
				HSD_TExpUnref(texp->tev.a_in[1].exp, texp->tev.a_in[1].sel);
				changed = 1;
				texp->tev.a_in[1] = zero_arg;
			} else if (texp->tev.a_in[0].sel == HSD_TE_0 && texp->tev.a_in[1].sel == HSD_TE_0) {
				texp->tev.a_op    = GX_TEV_ADD;
				changed           = 1;
				texp->tev.a_in[0] = texp->tev.a_in[2];
				texp->tev.a_in[2] = zero_arg;
			}
			break;
		}

		if (changed != 0) {
			result = 1;
		}
	} while (changed != 0);

	return result;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline int TExpMergeConflict(HSD_TExp* texp, HSD_TExp* child)
{
	if (texp->tev.tex != NULL && child->tev.tex != NULL && texp->tev.tex != child->tev.tex) {
		return 1;
	}
	if (texp->tev.chan != HSD_TE_UNDEF && child->tev.chan != HSD_TE_UNDEF && texp->tev.chan != child->tev.chan) {
		return 1;
	}
	return 0;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline int TExpColorHasCnst(HSD_TExp* texp)
{
	int has = 0;
	if (texp->tev.c_in[0].type == HSD_TE_CNST || texp->tev.c_in[1].type == HSD_TE_CNST || texp->tev.c_in[2].type == HSD_TE_CNST
	    || texp->tev.c_in[3].type == HSD_TE_CNST) {
		has = 1;
	}
	return has;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline int TExpAlphaHasCnst(HSD_TExp* texp)
{
	int has = 0;
	if (texp->tev.a_in[0].type == HSD_TE_CNST || texp->tev.a_in[1].type == HSD_TE_CNST || texp->tev.a_in[2].type == HSD_TE_CNST
	    || texp->tev.a_in[3].type == HSD_TE_CNST) {
		has = 1;
	}
	return has;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline int TExpBiasEnum(int value, u8* bias)
{
	switch (value) {
	case 0:
		*bias = GX_TB_ZERO;
		return 1;
	case 1:
		*bias = GX_TB_ADDHALF;
		return 1;
	case -1:
		*bias = GX_TB_SUBHALF;
		return 1;
	default:
		return 0;
	}
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline int TExpAddBias(u8 src_bias, int op, u8 dst_bias, u8* bias)
{
	int value;
	switch (src_bias) {
	case GX_TB_ADDHALF:
		value = 1;
		break;
	case GX_TB_SUBHALF:
		value = -1;
		break;
	default:
		value = 0;
		break;
	}
	if (op == GX_TEV_SUB) {
		value = -value;
	}
	switch (dst_bias) {
	case GX_TB_ADDHALF:
		value++;
		break;
	case GX_TB_SUBHALF:
		value--;
		break;
	}
	return TExpBiasEnum(value, bias);
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline int TExpScaleValue(u8 scale)
{
	// twice the real factor so divide by 2 stays an integer
	switch (scale) {
	case GX_CS_SCALE_1:
		return 2;
	case GX_CS_SCALE_2:
		return 4;
	case GX_CS_SCALE_4:
		return 8;
	case GX_CS_DIVIDE_2:
		return 1;
	default:
		return 2;
	}
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline int TExpScaleEnum(int value, u8* scale)
{
	switch (value) {
	case 1:
		*scale = GX_CS_DIVIDE_2;
		break;
	case 2:
		*scale = GX_CS_SCALE_1;
		break;
	case 4:
		*scale = GX_CS_SCALE_2;
		break;
	case 8:
		*scale = GX_CS_SCALE_4;
		break;
	default:
		return 0;
	}
	return 1;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline int TExpMulScale(u8 a, u8 b, u8* scale)
{
	return TExpScaleEnum(TExpScaleValue(a) * TExpScaleValue(b) / 2, scale);
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline void TExpInheritSrc(HSD_TExp* texp, HSD_TExp* child)
{
	if (texp->tev.tex == NULL) {
		texp->tev.tex = child->tev.tex;
	}
	if (texp->tev.chan == HSD_TE_UNDEF) {
		texp->tev.chan = child->tev.chan;
	}
	if (texp->tev.tex_swap == HSD_TE_UNDEF) {
		texp->tev.tex_swap = child->tev.tex_swap;
	}
	if (texp->tev.ras_swap == HSD_TE_UNDEF) {
		texp->tev.ras_swap = child->tev.ras_swap;
	}
}

/**
 * @note Address: 0x801DB4A8
 * @note Size: 0x17C8
 */
int SimplifyByMerge(HSD_TExp* texp)
{
	HSD_TExp* child;
	HSD_TExp* old_exp;
	HSD_TEArg tmp_arg;
	int result;
	int merged;
	int i;
	u8 child_sel;
	u8 old_sel;
	u8 bias;
	u8 scale;

	result = 0;
	do {
		merged = 0;

		// color: a tev with zero d feeding a or d, fold it into this stage
		if ((texp->tev.c_op != GX_TEV_ADD && texp->tev.c_op != GX_TEV_SUB) || texp->tev.c_in[2].sel != HSD_TE_0) {
			goto color_d;
		}
		child     = texp->tev.c_in[0].exp;
		child_sel = texp->tev.c_in[0].sel;
		if (!(texp->tev.c_in[0].type == HSD_TE_TEV && texp->tev.c_in[0].sel == HSD_TE_RGB && child->tev.c_range == 0
		      && child->tev.c_in[3].sel == HSD_TE_0 && !TExpMergeConflict(texp, child)
		      && !(TExpColorHasCnst(texp) && TExpColorHasCnst(child)))) {
			child     = texp->tev.c_in[3].exp;
			child_sel = texp->tev.c_in[3].sel;
			if (!(texp->tev.c_op == GX_TEV_ADD && texp->tev.c_in[3].type == HSD_TE_TEV && texp->tev.c_in[3].sel == HSD_TE_RGB
			      && child->tev.c_range == 0 && child->tev.c_in[3].sel == HSD_TE_0 && !TExpMergeConflict(texp, child)
			      && !(TExpColorHasCnst(texp) && TExpColorHasCnst(child)))) {
				goto color_d;
			}
			// only swap when a can move into d without losing a clamp
			if (texp->tev.c_in[0].type == HSD_TE_TEV) {
				if (texp->tev.c_in[0].sel == HSD_TE_RGB) {
					if (texp->tev.c_in[0].exp->tev.c_range != 0 && texp->tev.c_in[0].exp->tev.c_clamp != GX_TRUE) {
						goto color_d;
					}
				} else if (texp->tev.c_in[0].exp->tev.a_range != 0 && texp->tev.c_in[0].exp->tev.a_clamp != GX_TRUE) {
					goto color_d;
				}
			}
			tmp_arg           = texp->tev.c_in[0];
			texp->tev.c_in[0] = texp->tev.c_in[3];
			texp->tev.c_in[3] = tmp_arg;
		}

		if (child->tev.c_scale == GX_CS_SCALE_1) {
			if (!TExpAddBias(child->tev.c_bias, texp->tev.c_op, texp->tev.c_bias, &bias)) {
				goto color_d;
			}
			scale = texp->tev.c_scale;
		} else if (texp->tev.c_in[3].sel == HSD_TE_0 && texp->tev.c_bias == GX_TB_ZERO) {
			if (!TExpMulScale(texp->tev.c_scale, child->tev.c_scale, &scale)) {
				goto color_d;
			}
			bias = child->tev.c_bias;
		} else {
			goto color_d;
		}
		if (texp->tev.c_op == GX_TEV_SUB) {
			texp->tev.c_op = child->tev.c_op == GX_TEV_ADD;
		} else {
			texp->tev.c_op = child->tev.c_op;
		}
		for (i = 0; i < 3; i++) {
			texp->tev.c_in[i] = child->tev.c_in[i];
			HSD_TExpRef(texp->tev.c_in[i].exp, texp->tev.c_in[i].sel);
		}
		texp->tev.c_bias  = bias;
		texp->tev.c_scale = scale;
		HSD_TExpUnref(child, child_sel);
		TExpInheritSrc(texp, child);
		merged = 1;

	color_d:
		// color: a tev on d, pull its inputs up into this stage
		child     = texp->tev.c_in[3].exp;
		child_sel = texp->tev.c_in[3].sel;
		if (texp->tev.c_in[3].type == HSD_TE_TEV && child_sel == HSD_TE_RGB && !TExpMergeConflict(texp, child)
		    && !(TExpColorHasCnst(texp) && TExpColorHasCnst(child))) {
			if (child->tev.c_range != 0 && child->tev.c_clamp == GX_TRUE) {
				if (texp->tev.c_in[0].sel == HSD_TE_0 && texp->tev.c_in[2].sel == HSD_TE_0 && texp->tev.c_bias == GX_TB_ZERO
				    && texp->tev.c_scale != GX_CS_DIVIDE_2) {
					if (TExpMulScale(texp->tev.c_scale, child->tev.c_scale, &scale)) {
						merged            = 1;
						texp->tev.c_bias  = child->tev.c_bias;
						texp->tev.c_scale = scale;
						if (texp->tev.c_clamp != GX_TRUE) {
							texp->tev.c_clamp = child->tev.c_clamp;
						}
						TExpInheritSrc(texp, child);
						for (i = 0; i < 4; i++) {
							old_exp           = texp->tev.c_in[i].exp;
							old_sel           = texp->tev.c_in[i].sel;
							texp->tev.c_in[i] = child->tev.c_in[i];
							HSD_TExpRef(texp->tev.c_in[i].exp, texp->tev.c_in[i].sel);
							HSD_TExpUnref(old_exp, old_sel);
						}
					}
				}
			} else if (texp->tev.a_op == HSD_TE_UNDEF || texp->tev.a_op == GX_TEV_COMP_A8_GT || texp->tev.a_op == GX_TEV_COMP_A8_EQ
			           || texp->tev.a_op <= GX_TEV_SUB
			           || (child->tev.c_in[0].sel == HSD_TE_0 && child->tev.c_in[1].sel == HSD_TE_0
			               && child->tev.c_in[2].sel == HSD_TE_0)) {
				if (child->tev.c_scale == GX_CS_SCALE_1) {
					if (!((texp->tev.c_in[0].sel == HSD_TE_0 && texp->tev.c_in[2].sel == HSD_TE_0)
					      || (child->tev.c_in[0].sel == HSD_TE_0 && child->tev.c_in[2].sel == HSD_TE_0))) {
						goto alpha_ad;
					}
					if (!TExpAddBias(child->tev.c_bias, GX_TEV_ADD, texp->tev.c_bias, &bias)) {
						goto alpha_ad;
					}
					scale = texp->tev.c_scale;
				} else if (texp->tev.c_in[0].sel == HSD_TE_0 && texp->tev.c_in[1].sel == HSD_TE_0 && texp->tev.c_in[2].sel == HSD_TE_0
				           && texp->tev.c_bias == GX_TB_ZERO) {
					if (!TExpMulScale(texp->tev.c_scale, child->tev.c_scale, &scale)) {
						goto alpha_ad;
					}
					bias = child->tev.c_bias;
				} else {
					goto alpha_ad;
				}
				merged            = 1;
				texp->tev.c_in[3] = child->tev.c_in[3];
				HSD_TExpRef(texp->tev.c_in[3].exp, texp->tev.c_in[3].sel);
				texp->tev.c_bias  = bias;
				texp->tev.c_scale = scale;
				TExpInheritSrc(texp, child);
				if (child->tev.c_in[0].sel != HSD_TE_0 || child->tev.c_in[1].sel != HSD_TE_0 || child->tev.c_in[2].sel != HSD_TE_0) {
					for (i = 0; i < 3; i++) {
						HSD_TExp* old_exp = texp->tev.c_in[i].exp;
						u8 old_sel        = texp->tev.c_in[i].sel;
						texp->tev.c_in[i] = child->tev.c_in[i];
						HSD_TExpRef(texp->tev.c_in[i].exp, texp->tev.c_in[i].sel);
						HSD_TExpUnref(old_exp, old_sel);
					}
				}
				HSD_TExpUnref(child, child_sel);
			}
		}

	alpha_ad:
		// alpha: same as the color a or d fold
		if ((texp->tev.a_op != GX_TEV_ADD && texp->tev.a_op != GX_TEV_SUB) || texp->tev.a_in[2].sel != HSD_TE_0) {
			goto alpha_d;
		}
		child     = texp->tev.a_in[0].exp;
		child_sel = texp->tev.a_in[0].sel;
		if (!(texp->tev.a_in[0].type == HSD_TE_TEV && child->tev.a_range == 0 && child->tev.a_in[3].sel == HSD_TE_0
		      && !TExpMergeConflict(texp, child) && !(TExpAlphaHasCnst(texp) && TExpAlphaHasCnst(child)))) {
			child     = texp->tev.a_in[3].exp;
			child_sel = texp->tev.a_in[3].sel;
			// The alpha fallback requires a child without alpha constants.
			if (!(texp->tev.a_op == GX_TEV_ADD && texp->tev.a_in[3].type == HSD_TE_TEV && child->tev.a_range == 0
			      && child->tev.a_in[3].sel == HSD_TE_0 && !TExpMergeConflict(texp, child)
			      && !TExpAlphaHasCnst(child))) {
				goto alpha_d;
			}
			if (texp->tev.a_in[0].type == HSD_TE_TEV && texp->tev.a_in[0].exp->tev.a_range != 0
			    && texp->tev.a_in[0].exp->tev.a_clamp != GX_TRUE) {
				goto alpha_d;
			}
			tmp_arg           = texp->tev.a_in[0];
			texp->tev.a_in[0] = texp->tev.a_in[3];
			texp->tev.a_in[3] = tmp_arg;
		}

		if (child->tev.a_scale == GX_CS_SCALE_1) {
			if (!TExpAddBias(child->tev.a_bias, texp->tev.a_op, texp->tev.a_bias, &bias)) {
				goto alpha_d;
			}
			scale = texp->tev.a_scale;
		} else if (texp->tev.a_in[3].sel == HSD_TE_0 && texp->tev.a_bias == GX_TB_ZERO) {
			if (!TExpMulScale(texp->tev.a_scale, child->tev.a_scale, &scale)) {
				goto alpha_d;
			}
			bias = child->tev.a_bias;
		} else {
			goto alpha_d;
		}
		if (texp->tev.a_op == GX_TEV_SUB) {
			texp->tev.a_op = child->tev.a_op == GX_TEV_ADD;
		} else {
			texp->tev.a_op = child->tev.a_op;
		}
		for (i = 0; i < 3; i++) {
			texp->tev.a_in[i] = child->tev.a_in[i];
			HSD_TExpRef(texp->tev.a_in[i].exp, texp->tev.a_in[i].sel);
		}
		texp->tev.a_bias  = bias;
		texp->tev.a_scale = scale;
		HSD_TExpUnref(child, child_sel);
		TExpInheritSrc(texp, child);
		merged = 1;

	alpha_d:
		// alpha: a tev on d
		child     = texp->tev.a_in[3].exp;
		child_sel = texp->tev.a_in[3].sel;
		if (texp->tev.a_in[3].type == HSD_TE_TEV && !TExpMergeConflict(texp, child)
		    && !(TExpAlphaHasCnst(texp) && TExpAlphaHasCnst(child))) {
			if (child->tev.a_range != 0 && child->tev.a_clamp == GX_TRUE) {
				if (texp->tev.a_in[0].sel == HSD_TE_0 && texp->tev.a_in[2].sel == HSD_TE_0 && texp->tev.a_bias == GX_TB_ZERO
				    && texp->tev.a_scale != GX_CS_DIVIDE_2) {
					if (TExpMulScale(texp->tev.a_scale, child->tev.a_scale, &scale)) {
						merged            = 1;
						texp->tev.a_bias  = child->tev.a_bias;
						texp->tev.a_scale = scale;
						if (texp->tev.a_clamp != GX_TRUE) {
							texp->tev.a_clamp = child->tev.a_clamp;
						}
						TExpInheritSrc(texp, child);
						for (i = 0; i < 4; i++) {
							HSD_TExp* old_exp = texp->tev.a_in[i].exp;
							u8 old_sel        = texp->tev.a_in[i].sel;
							texp->tev.a_in[i] = child->tev.a_in[i];
							HSD_TExpRef(texp->tev.a_in[i].exp, texp->tev.a_in[i].sel);
							HSD_TExpUnref(old_exp, old_sel);
						}
					}
				}
			} else {
				if (child->tev.a_scale == GX_CS_SCALE_1) {
					if (!((texp->tev.a_in[0].sel == HSD_TE_0 && texp->tev.a_in[2].sel == HSD_TE_0)
					      || (child->tev.a_in[0].sel == HSD_TE_0 && child->tev.a_in[2].sel == HSD_TE_0))) {
						goto next;
					}
					if (!TExpAddBias(child->tev.a_bias, GX_TEV_ADD, texp->tev.a_bias, &bias)) {
						goto next;
					}
					scale = texp->tev.a_scale;
				} else if (texp->tev.a_in[0].sel == HSD_TE_0 && texp->tev.a_in[1].sel == HSD_TE_0 && texp->tev.a_in[2].sel == HSD_TE_0
				           && texp->tev.a_bias == GX_TB_ZERO) {
					if (!TExpMulScale(texp->tev.a_scale, child->tev.a_scale, &scale)) {
						goto next;
					}
					bias = child->tev.a_bias;
				} else {
					goto next;
				}
				merged            = 1;
				texp->tev.a_in[3] = child->tev.a_in[3];
				HSD_TExpRef(texp->tev.a_in[3].exp, texp->tev.a_in[3].sel);
				texp->tev.a_bias  = bias;
				texp->tev.a_scale = scale;
				TExpInheritSrc(texp, child);
				if (child->tev.a_in[0].sel != HSD_TE_0 || child->tev.a_in[1].sel != HSD_TE_0 || child->tev.a_in[2].sel != HSD_TE_0) {
					for (i = 0; i < 3; i++) {
						HSD_TExp* old_exp = texp->tev.a_in[i].exp;
						u8 old_sel        = texp->tev.a_in[i].sel;
						texp->tev.a_in[i] = child->tev.a_in[i];
						HSD_TExpRef(texp->tev.a_in[i].exp, texp->tev.a_in[i].sel);
						HSD_TExpUnref(old_exp, old_sel);
					}
				}
				HSD_TExpUnref(child, child_sel);
			}
		}

	next:
		if (merged != 0) {
			result = 1;
		}
	} while (merged != 0);

	return result;
}

/**
 * @note Address: 0x801DCC70
 * @note Size: 0x2B4
 */
void CalcTevRange(HSD_TExp* texp)
{
	/* Use 256 as the normalized unit for the range estimate. */
	const int one = 256;
	int range = 0;
	int i;
	HSD_TExp* src;
	if (texp->tev.c_in[3].sel != HSD_TE_0) {
		switch (HSD_TExpGetType(texp->tev.c_in[3].exp)) {
		case HSD_TE_TEV:
			if (texp->tev.c_in[3].sel == HSD_TE_RGB) {
				src = texp->tev.c_in[3].exp;
				if (src->tev.c_clamp != GX_TRUE && src->tev.c_range == 1) {
					texp->tev.c_range = 1;
					goto alpha;
				}
				range = one;
				break;
			} else {
				src = texp->tev.c_in[3].exp;
				if (src->tev.a_clamp != GX_TRUE && src->tev.a_range == 1) {
					texp->tev.c_range = 1;
					goto alpha;
				}
			}
			range = one;
			break;
		default:
			range = one;
			break;
		}
	}
	for (i = 0; i < 3; i++) {
		if (texp->tev.c_in[i].sel != HSD_TE_0) {
			if (texp->tev.c_op == GX_TEV_SUB) {
				texp->tev.c_range = 1;
				goto alpha;
			}
			range += one;
			break;
		}
	}
	switch (texp->tev.c_bias) {
	case GX_TB_ADDHALF:
		range += one / 2;
		break;
	case GX_TB_SUBHALF:
		texp->tev.c_range = 1;
		goto alpha;
	}
	switch (texp->tev.c_scale) {
	case GX_CS_SCALE_2:
		range *= 2;
		break;
	case GX_CS_SCALE_4:
		range *= 4;
		break;
	case GX_CS_DIVIDE_2:
		range /= 2;
		break;
	}
	texp->tev.c_range = range > one;
alpha:
	range = 0;
	if (texp->tev.a_in[3].sel != HSD_TE_0) {
		switch (HSD_TExpGetType(texp->tev.a_in[3].exp)) {
		case HSD_TE_TEV:
			src = texp->tev.a_in[3].exp;
			if (src->tev.a_clamp != GX_TRUE && src->tev.a_range == 1) {
				texp->tev.a_range = 1;
				return;
			}
			range = one;
			break;
		default:
			range = one;
			break;
		}
	}
	for (i = 0; i < 3; i++) {
		if (texp->tev.a_in[i].sel != HSD_TE_0) {
			if (texp->tev.a_op == GX_TEV_SUB) {
				texp->tev.a_range = 1;
				return;
			}
			range += one;
			break;
		}
	}
	switch (texp->tev.a_bias) {
	case GX_TB_ADDHALF:
		range += one / 2;
		break;
	case GX_TB_SUBHALF:
		texp->tev.a_range = 1;
		return;
	}
	switch (texp->tev.a_scale) {
	case GX_CS_SCALE_2:
		range *= 2;
		break;
	case GX_CS_SCALE_4:
		range *= 4;
		break;
	case GX_CS_DIVIDE_2:
		range /= 2;
		break;
	}
	texp->tev.a_range = range > one;
}

/**
 * @note Address: 0x801DCF24
 * @note Size: 0x90
 */
int HSD_TExpSimplify(HSD_TExp* texp_)
{
	HSD_TExp* texp = (HSD_TExp*)texp_;
	int res        = 0;
	if (HSD_TExpGetType(texp) != HSD_TE_TEV) {
		return 0;
	}
	if (SimplifySrc(texp) != 0) {
		res = 1;
	}
	if (SimplifyThis(texp) != 0) {
		res = 1;
	}
	if (SimplifyByMerge(texp) != 0) {
		res = 1;
	}
	CalcTevRange(texp);
	return res;
}

/**
 * @note Address: 0x801DCFB4
 * @note Size: 0x1FC
 */
int HSD_TExpSimplify2(HSD_TExp* texp_)
{
	HSD_TExp* texp = (HSD_TExp*)texp_;
	HSD_TExp* src_exp;
	u8 src_sel;
	int i;

	for (i = 0; i < 4; i++) {
		src_exp = texp->tev.c_in[i].exp;
		src_sel = texp->tev.c_in[i].sel;
		if (texp->tev.c_in[i].type == HSD_TE_TEV && src_sel == HSD_TE_RGB && IsThroughColor(src_exp)) {
			switch (src_exp->tev.c_in[3].type) {
			case HSD_TE_KONST:
				if (texp->tev.kcsel == HSD_TE_UNDEF) {
					texp->tev.kcsel = src_exp->tev.kcsel;
				} else if (texp->tev.kcsel != src_exp->tev.kcsel) {
					break;
				}
				/* fallthrough */
			case HSD_TE_IMM:
				texp->tev.c_in[i] = src_exp->tev.c_in[3];
				HSD_TExpRef(texp->tev.c_in[i].exp, texp->tev.c_in[i].sel);
				HSD_TExpUnref(src_exp, src_sel);
				break;
			}
		}
	}
	for (i = 0; i < 4; i++) {
		src_exp = texp->tev.a_in[i].exp;
		src_sel = texp->tev.a_in[i].sel;
		if (texp->tev.a_in[i].type == HSD_TE_TEV && IsThroughAlpha(src_exp)) {
			switch (src_exp->tev.a_in[3].type) {
			case HSD_TE_KONST:
				if (texp->tev.kasel == HSD_TE_UNDEF) {
					texp->tev.kasel = src_exp->tev.kasel;
				} else if (texp->tev.kasel != src_exp->tev.kasel) {
					break;
				}
				/* fallthrough */
			case HSD_TE_IMM:
				texp->tev.a_in[i] = src_exp->tev.a_in[3];
				HSD_TExpRef(texp->tev.a_in[i].exp, texp->tev.a_in[i].sel);
				HSD_TExpUnref(src_exp, src_sel);
				break;
			}
		}
	}
	return 0;
}
