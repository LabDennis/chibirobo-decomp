#include "game/hsd_jobj.h"
#include "game/ut.h"
#include "game/trianglenormal.h"

/**
 * @note Address: 0x8018EE60
 * @note Size: 0x68
 */
void CalcTriangleNormal(Vec* a, Vec* b, Vec* c, Vec* normal)
{
	Vec ab, bc;
	CalculateTriangleNormal(a, b, c, normal, ab, bc);
}

/**
 * @note Address: 0x8018EE2C
 * @note Size: 0x34
 */
f32 SignedTriangleArea2XZ(Vec* a, Vec* b, Vec* c)
{
	return a->x * (b->z - c->z) + b->x * (c->z - a->z) + c->x * (a->z - b->z);
}
