#include <global.h>
#include <dolphin/gx/gx.h>
#include <sysdolphin/class.h>
#include <sysdolphin/gobjproc.h>
#include <sysdolphin/texp.h>
#include <sysdolphin/tev.h>
#include <sysdolphin/tobj.h>

#define TEXPDAG_ASSERT(line, cond)        ((cond) ? ((void)0) : __assert(__FILE__, line, #cond))
#define TEXPDAG_ASSERT_S(line, cond, str) ((cond) ? ((void)0) : __assert(__FILE__, line, str))

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

int HSD_TExpMakeDag(HSD_TExp* root, struct HSD_TExpDag* list);
void HSD_TExpSchedule(int num, struct HSD_TExpDag* list, HSD_TExp** result, struct HSD_TExpRes* resource);
int HSD_TExpSimplify(HSD_TExp* texp_);
int HSD_TExpSimplify2(HSD_TExp* texp_);

static inline bool IsThroughColor(HSD_TExp* texp)
{
	return texp->tev.c_op == GX_TEV_ADD && texp->tev.c_in[0].sel == HSD_TE_0 && texp->tev.c_in[1].sel == HSD_TE_0 && texp->tev.c_bias == 0
	    && texp->tev.c_scale == 0;
}

static inline bool IsThroughAlpha(HSD_TExp* texp)
{
	return texp->tev.a_op == GX_TEV_ADD && texp->tev.a_in[0].sel == HSD_TE_0 && texp->tev.a_in[1].sel == HSD_TE_0 && texp->tev.a_bias == 0
	    && texp->tev.a_scale == 0;
}

HSD_TExpType HSD_TExpGetType(HSD_TExp* texp);
void HSD_TExpRef(HSD_TExp* texp, u8 sel);
void HSD_TExpUnref(HSD_TExp* texp, u8 sel);

/**
 * @note Address: 0x801D9798
 * @note Size: 0x1B0
 */
int assign_reg(int num, u32* dep, HSD_TExpDag* list, int* order)
{
	int ref;
	u8 color_refs[4] = { 0 };
	HSD_TETev* tev;
	HSD_TExpDag* dag_entry;
	int idx;
	int i;
	int j;
	int min_color_reg = 4;
	int min_alpha_reg = 4;
	u8 alpha_refs[4] = { 0 };

	for (idx = num - 1; idx >= 0; idx--) {
		dag_entry = &list[order[idx]];
		tev       = dag_entry->tev;

		for (i = 0; i < 4; i++) {
			if (HSD_TExpGetType(tev->c_in[i].exp) == HSD_TE_TEV) {
				if (tev->c_in[i].sel == 1) {
					color_refs[tev->c_in[i].exp->tev.c_dst] -= 1;
				} else {
					alpha_refs[tev->c_in[i].exp->tev.a_dst] -= 1;
				}
			}
			if (HSD_TExpGetType(tev->a_in[i].exp) == HSD_TE_TEV) {
				alpha_refs[tev->a_in[i].exp->tev.a_dst] -= 1;
			}
		}

		tev = dag_entry->tev;
		ref = tev->c_ref;
		if (ref > 0) {
			for (j = 3; j >= 0; j--) {
				if (color_refs[j] == 0) {
					color_refs[j] = ref;
					tev->c_dst    = j;
					if (min_color_reg > j) {
						min_color_reg = j;
					}
					break;
				}
			}
		}
		ref = tev->a_ref;
		if (ref > 0) {
			for (j = 3; j >= 0; j--) {
				if (alpha_refs[j] == 0) {
					alpha_refs[j] = ref;
					tev->a_dst    = j;
					if (min_alpha_reg > j) {
						min_alpha_reg = j;
					}
					break;
				}
			}
		}
	}
	return (4 - min_color_reg) + (4 - min_alpha_reg);
}

/**
 * @note Address: 0x801D9948
 * @note Size: 0x238
 */
void order_dag(int num, u32* dep, u32* full_dep, HSD_TExpDag* list, int depth, int idx, u32 done_set, u32 ready_set, int* order, int* min,
               int* min_order)
{
	HSD_TExpDag* dag;
	u32 blocked;
	u32 full_bits;
	int score;
	int i;

	done_set |= 1 << idx;
	ready_set &= ~(1 << idx);
	order[depth++] = (u8)idx;

	if (depth == num) {
		score = assign_reg(num, dep, list, order);
		if (score < *min) {
			*min = score;
			for (i = 0; i < num; i++) {
				min_order[i] = order[i];
			}
		}
	} else {
		blocked   = ready_set | dep[idx];
		full_bits = 0;
		for (i = 0; i < num; i++) {
			if (blocked & (1 << i)) {
				full_bits |= full_dep[i];
			}
		}
		dag       = &list[idx];
		ready_set = blocked & ~full_bits;
		if (dag->nb_dep == 1 && (ready_set & dep[idx])) {
			order_dag(num, dep, full_dep, list, depth, dag->depend[0]->idx, done_set, ready_set, order, min, min_order);
		} else {
			for (i = 0; i < num; i++) {
				if (ready_set & (1 << i)) {
					order_dag(num, dep, full_dep, list, depth, i, done_set, ready_set, order, min, min_order);
				}
			}
		}
	}
}

/**
 * @note Address: N/A
 * @note Size: 0x438
 */
void CalcDistance(HSD_TExp** tevs, int* dist, HSD_TExp* tev, int num, int depth)
{
	int i;

	for (i = 0; i < num; i++) {
		if (tevs[i] == tev) {
			if (dist[i] < depth) {
				dist[i] = depth;
				for (i = 0; i < 4; i++) {
					if (tev->tev.c_in[i].type == HSD_TE_TEV) {
						CalcDistance(tevs, dist, tev->tev.c_in[i].exp, num, depth + 1);
					}
					if (tev->tev.a_in[i].type == HSD_TE_TEV) {
						CalcDistance(tevs, dist, tev->tev.a_in[i].exp, num, depth + 1);
					}
				}
			}
			return;
		}
	}
}

#define HSD_TEXP_MAX_NUM 32

/**
 * @note Address: N/A
 * @note Size: 0x4B0
 */
int HSD_TExpMakeDag(HSD_TExp* root, HSD_TExpDag* list)
{
	HSD_TExp* tevs[HSD_TEXP_MAX_NUM];
	int dist[32];
	int n;
	int i;
	int j;
	int k;
	int l;
	int num;
	int idx;
	HSD_TExpDag* dag;

	TEXPDAG_ASSERT(0xEE, HSD_TExpGetType(root) == HSD_TE_TEV);

	n         = 0;
	tevs[n++] = root;
	for (j = 0; j < n; j++) {
		HSD_TExp* tmp;
		TEXPDAG_ASSERT_S(0xF6, j < HSD_TEXP_MAX_NUM, "j<HSD_TEXP_MAX_NUM");
		tmp = tevs[j];
		for (i = 0; i < 4; i++) {
			if (tmp->tev.c_in[i].type == HSD_TE_TEV) {
				for (k = 0; k < n; k++) {
					if (tevs[k] == tmp->tev.c_in[i].exp) {
						break;
					}
				}
				if (k >= n) {
					tevs[n++] = tmp->tev.c_in[i].exp;
				}
			}
		}

		for (i = 0; i < 4; i++) {
			if (tmp->tev.a_in[i].type == HSD_TE_TEV) {
				for (k = 0; k < n; k++) {
					if (tevs[k] == tmp->tev.a_in[i].exp) {
						break;
					}
				}
				if (k >= n) {
					tevs[n++] = tmp->tev.a_in[i].exp;
				}
			}
		}
	}

	num = n;
	for (n = 0; n < num; n++) {
		dist[n] = -1;
	}

	CalcDistance(&tevs[0], &dist[0], tevs[0], num, 0);
	for (i = 0; i < num; i++) {
		for (idx = i + 1; idx < num; idx++) {
			if (dist[idx - 1] > dist[idx]) {
				{
					HSD_TExp* tmp = tevs[idx - 1];
					tevs[idx - 1] = tevs[idx];
					tevs[idx]     = tmp;
				}

				{
					int tmp       = dist[idx - 1];
					dist[idx - 1] = dist[idx];
					dist[idx]     = tmp;
				}
			}
		}
	}

	for (j = num - 1; j >= 0; j--) {
		HSD_TExp* tmp;
		dag         = &list[j];
		tmp         = tevs[j];
		dag->idx    = (u8)j;
		dag->nb_ref = 0;
		dag->nb_dep = 0;
		dag->tev    = &tmp->tev;
		for (idx = 0; idx < 4; idx++) {
			if (tmp->tev.c_in[idx].type == HSD_TE_TEV) {
				for (l = j; l < num; l++) {
					if (tmp->tev.c_in[idx].exp == tevs[l]) {
						HSD_TExpDag* dep_entry = &list[l];
						for (l = 0; l < dag->nb_dep; l++) {
							if (dag->depend[l] == dep_entry) {
								break;
							}
						}
						if (l >= dag->nb_dep) {
							dag->depend[dag->nb_dep++] = dep_entry;
							dep_entry->nb_ref++;
						}
						break;
					}
				}
				TEXPDAG_ASSERT(0x145, l < num);
			}
		}

		for (idx = 0; idx < 4; idx++) {
			if (tmp->tev.a_in[idx].type == HSD_TE_TEV) {
				for (l = j; l < num; l++) {
					if (tmp->tev.a_in[idx].exp == tevs[l]) {
						HSD_TExpDag* dep_entry = &list[l];
						for (l = 0; l < dag->nb_dep; l++) {
							if (dag->depend[l] == dep_entry) {
								break;
							}
						}
						if (l >= dag->nb_dep) {
							dag->depend[dag->nb_dep++] = dep_entry;
							dep_entry->nb_ref++;
						}
						break;
					}
				}
				TEXPDAG_ASSERT(0x15B, l < num);
			}
		}
	}
	return num;
}

static inline void make_dependancy_mtx(int num, HSD_TExpDag* list, u32* dep_mtx)
{
	HSD_TExpDag* dag;
	int i, j;

	for (i = 0; i < num; i++) {
		dep_mtx[i] = 0;
		dag        = &list[i];
		for (j = 0; j < dag->nb_dep; j++) {
			dep_mtx[i] |= 1 << dag->depend[j]->idx;
		}
	}
}

static inline void make_full_dependancy_mtx(int num, const u32* dep, u32* full)
{
	int i, j, k;
	int changed;
	u32 bits;
	u32 flag;
	u32 old;

	for (i = 0; i < num; i++) {
		full[i] = dep[i];
	}
	do {
		changed = 0;
		for (j = 0; j < num; j++) {
			flag = 1 << j;
			old  = full[j];
			bits = old;
			for (k = 0; k < num; k++) {
				if ((flag & full[k]) != 0) {
					u32 old = full[k];
					full[k] |= bits;
					if (old != full[k]) {
						changed = 1;
					}
				}
			}
		}
	} while (changed != 0);
}

/**
 * @note Address: N/A
 * @note Size: 0x368
 */
void HSD_TExpSchedule(int num, HSD_TExpDag* list, HSD_TExp** result, HSD_TExpRes* resource)
{

	u32 dep_mtx[32];
	u32 full_dep_matrix[32];
	u32 order[32];
	u32 min_order[32];

	int i, j;
	int min;

	min = 5;
	memset(min_order, 0, sizeof(min_order));
	make_dependancy_mtx(num, list, dep_mtx);
	make_full_dependancy_mtx(num, dep_mtx, full_dep_matrix);
	order_dag(num, dep_mtx, full_dep_matrix, list, 0, 0, 0, 0, (int*)order, &min, (int*)min_order);

	for (i = 0; i < num; i++) {
		result[i] = (HSD_TExp*)list[min_order[i]].tev;
		if (result[i]->tev.c_dst != 0xFF) {
			resource->reg[result[i]->tev.c_dst + 4].color = 3;

			for (j = 0; j < 4; j++) {
				if (HSD_TExpGetType(result[i]->tev.c_in[j].exp) == HSD_TE_TEV) {
					if (result[i]->tev.c_in[j].sel == 1) {
						static u32 in[4]           = { GX_CC_C0, GX_CC_C1, GX_CC_C2, GX_CC_CPREV };
						result[i]->tev.c_in[j].arg = in[result[i]->tev.c_in[j].exp->tev.c_dst];
					} else {
						static u32 in[4]           = { GX_CC_A0, GX_CC_A1, GX_CC_A2, GX_CC_APREV };
						result[i]->tev.c_in[j].arg = in[result[i]->tev.c_in[j].exp->tev.a_dst];
					}
				}
			}
		}
		if (result[i]->tev.a_dst != 0xFF) {
			resource->reg[result[i]->tev.a_dst + 4].alpha = 1;

			for (j = 0; j < 4; j++) {
				if (HSD_TExpGetType(result[i]->tev.a_in[j].exp) == HSD_TE_TEV) {
					static u32 in[4]           = { GX_CA_A0, GX_CA_A1, GX_CA_A2, GX_CA_APREV };
					result[i]->tev.a_in[j].arg = in[result[i]->tev.a_in[j].exp->tev.a_dst];
				}
			}
		}
	}
}
