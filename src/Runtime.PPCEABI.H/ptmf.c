#include "types.h"

typedef struct PTMF {
	s32 thisDelta;
	s32 vtableOffset;
	u32 target; // Function address if vtableOffset is negative, otherwise the vtable pointer offset.
} PTMF;

STATIC_ASSERT(sizeof(PTMF) == 12);

const PTMF __ptmf_null = { 0, 0, 0 };

s32 __ptmf_test(const PTMF* ptmf)
{
	return ptmf->thisDelta != 0 || ptmf->vtableOffset != 0 || ptmf->target != 0;
}

s32 __ptmf_cmpr(const PTMF* lhs, const PTMF* rhs)
{
	return lhs->thisDelta != rhs->thisDelta || lhs->vtableOffset != rhs->vtableOffset || lhs->target != rhs->target;
}
