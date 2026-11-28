#include "revolution/vec.h"

/**
 * @note Address: 0x8018FBDC
 * @note Size: 0x60
 */
void C_VECCrossProduct(const Vec* a, const Vec* b, Vec* axb)
{
	Vec cross;

	cross.x = (a->y * b->z) - (a->z * b->y);
	cross.y = (a->z * b->x) - (a->x * b->z);
	cross.z = (a->x * b->y) - (a->y * b->x);

	axb->x = cross.x;
	axb->y = cross.y;
	axb->z = cross.z;
}
