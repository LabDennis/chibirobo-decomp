#ifndef HSD_TEXPDAG_H
#define HSD_TEXPDAG_H

#include <sysdolphin/forward.h>
#include <sysdolphin/texp.h>

struct HSD_TExpDag {
	HSD_TETev* tev;
	u8 idx;
	u8 nb_dep;
	u8 nb_ref;
	u8 dist;
	struct HSD_TExpDag* depend[8];
};

void order_dag(int num, u32* dep, u32* full_dep, HSD_TExpDag* list, int depth, int idx, u32 done_set, u32 ready_set, int* order, int* min,
               int* min_order);

#endif
