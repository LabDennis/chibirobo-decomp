#ifndef _texp_internal_h_
#define _texp_internal_h_

#include <dolphin/gx/gxtypes.h>
#include <sysdolphin/forward.h>
#include <sysdolphin/texp.h>

#define HSD_TEXP_MAX_NUM 32

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

int HSD_TExpMakeDag(HSD_TExp* root, HSD_TExpDag* list);
void HSD_TExpSchedule(int num, HSD_TExpDag* list, HSD_TExp** result, HSD_TExpRes* resource);
int HSD_TExpSimplify(HSD_TExp* texp);
int HSD_TExpSimplify2(HSD_TExp* texp);
void HSD_TExpRef(HSD_TExp* texp, u8 sel);
void HSD_TExpUnref(HSD_TExp* texp, u8 sel);

static inline bool IsThroughColor(HSD_TExp* texp)
{
	return texp->tev.c_op == GX_TEV_ADD && texp->tev.c_in[0].sel == HSD_TE_0 && texp->tev.c_in[1].sel == HSD_TE_0
	    && texp->tev.c_bias == GX_TB_ZERO && texp->tev.c_scale == GX_CS_SCALE_1;
}

static inline bool IsThroughAlpha(HSD_TExp* texp)
{
	return texp->tev.a_op == GX_TEV_ADD && texp->tev.a_in[0].sel == HSD_TE_0 && texp->tev.a_in[1].sel == HSD_TE_0
	    && texp->tev.a_bias == GX_TB_ZERO && texp->tev.a_scale == GX_CS_SCALE_1;
}

#endif
