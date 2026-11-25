#include <global.h>
#include <sysdolphin/class.h>
#include <sysdolphin/texp_internal.h>
#include <string.h>

#define TEXPDAG_ASSERT(line, cond)        ((cond) ? ((void)0) : __assert(__FILE__, line, #cond))
#define TEXPDAG_ASSERT_S(line, cond, str) ((cond) ? ((void)0) : __assert(__FILE__, line, str))

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
				if (tev->c_in[i].sel == HSD_TE_RGB) {
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

	done_set |= 1U << idx;
	ready_set &= ~(1U << idx);
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
			if (blocked & (1U << i)) {
				full_bits |= full_dep[i];
			}
		}
		dag       = &list[idx];
		ready_set = blocked & ~full_bits;
		if (dag->nb_dep == 1 && (ready_set & dep[idx])) {
			order_dag(num, dep, full_dep, list, depth, dag->depend[0]->idx, done_set, ready_set, order, min, min_order);
		} else {
			for (i = 0; i < num; i++) {
				if (ready_set & (1U << i)) {
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

/**
 * @note Address: N/A
 * @note Size: 0x4B0
 */
int HSD_TExpMakeDag(HSD_TExp* root, HSD_TExpDag* list)
{
	HSD_TExp* tevs[HSD_TEXP_MAX_NUM];
	int dist[HSD_TEXP_MAX_NUM];
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

static inline void make_dependency_matrix(int num, HSD_TExpDag* list, u32* dep_mtx)
{
	HSD_TExpDag* dag;
	int i, j;

	for (i = 0; i < num; i++) {
		dep_mtx[i] = 0;
		dag        = &list[i];
		for (j = 0; j < dag->nb_dep; j++) {
			dep_mtx[i] |= 1U << dag->depend[j]->idx;
		}
	}
}

/* Expand each stage's mask to include indirect dependencies. */
static inline void make_full_dependency_matrix(int num, const u32* dep, u32* full)
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
			flag = 1U << j;
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
	u32 dep_mtx[HSD_TEXP_MAX_NUM];
	u32 full_dep_matrix[HSD_TEXP_MAX_NUM];
	int order[HSD_TEXP_MAX_NUM];
	int min_order[HSD_TEXP_MAX_NUM];

	int i, j;
	int min;

	min = 5;
	memset(min_order, 0, sizeof(min_order));
	make_dependency_matrix(num, list, dep_mtx);
	make_full_dependency_matrix(num, dep_mtx, full_dep_matrix);
	order_dag(num, dep_mtx, full_dep_matrix, list, 0, 0, 0, 0, order, &min, min_order);

	for (i = 0; i < num; i++) {
		result[i] = (HSD_TExp*)list[min_order[i]].tev;
		if (result[i]->tev.c_dst != HSD_TE_UNDEF) {
			resource->reg[result[i]->tev.c_dst + 4].color = 3;

			for (j = 0; j < 4; j++) {
				if (HSD_TExpGetType(result[i]->tev.c_in[j].exp) == HSD_TE_TEV) {
					if (result[i]->tev.c_in[j].sel == HSD_TE_RGB) {
						static GXTevColorArg color_register_args[4] = { GX_CC_C0, GX_CC_C1, GX_CC_C2, GX_CC_CPREV };
						result[i]->tev.c_in[j].arg = color_register_args[result[i]->tev.c_in[j].exp->tev.c_dst];
					} else {
						static GXTevColorArg color_from_alpha_args[4] = { GX_CC_A0, GX_CC_A1, GX_CC_A2, GX_CC_APREV };
						result[i]->tev.c_in[j].arg = color_from_alpha_args[result[i]->tev.c_in[j].exp->tev.a_dst];
					}
				}
			}
		}
		if (result[i]->tev.a_dst != HSD_TE_UNDEF) {
			resource->reg[result[i]->tev.a_dst + 4].alpha = 1;

			for (j = 0; j < 4; j++) {
				if (HSD_TExpGetType(result[i]->tev.a_in[j].exp) == HSD_TE_TEV) {
					static GXTevAlphaArg alpha_register_args[4] = { GX_CA_A0, GX_CA_A1, GX_CA_A2, GX_CA_APREV };
					result[i]->tev.a_in[j].arg = alpha_register_args[result[i]->tev.a_in[j].exp->tev.a_dst];
				}
			}
		}
	}
}
