#include <global.h>
#include <sysdolphin/texpdag.h>

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
