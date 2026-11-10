#ifndef GAME_TRIANGLENORMAL_H
#define GAME_TRIANGLENORMAL_H

extern "C" {
#include <dolphin/mtx/vec.h>
void C_VECCrossProduct(const Vec* a, const Vec* b, Vec* axb);
}

static inline void CalculateTriangleNormal(Vec* a, Vec* b, Vec* c, Vec* normal, Vec& ab, Vec& bc)
{
	PSVECSubtract(a, b, &ab);
	PSVECSubtract(b, c, &bc);
	C_VECCrossProduct(&bc, &ab, normal);
}

#endif
