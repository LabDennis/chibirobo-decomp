#ifndef GAME_EDGEPERPENDICULAR_H
#define GAME_EDGEPERPENDICULAR_H

extern "C" {
#include <dolphin/mtx/vec.h>
void C_VECCrossProduct(const Vec* a, const Vec* b, Vec* axb);
}

static inline void CalculateOutwardPerpendicular(Vec* a, Vec* b, Vec* c, Vec* out, Vec& bc, Vec& ba, Vec& n)
{
	PSVECSubtract(b, c, &bc);
	PSVECSubtract(b, a, &ba);
	C_VECCrossProduct(&ba, &bc, &n);
	C_VECCrossProduct(&bc, &n, out);
}

static inline void CalculateOutwardPerpendicular(Vec* a, Vec* b, Vec* c, Vec* out)
{
	Vec bc;
	Vec ba;
	Vec n;
	CalculateOutwardPerpendicular(a, b, c, out, bc, ba, n);
}

#endif
