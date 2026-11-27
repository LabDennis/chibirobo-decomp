#include "Dolphin/vec.h"

// NONMATCHING: same vector operations; retail uses mr for the normal/dst copies in the prologue,
// while GC/1.2.5n emits addi ...,0 and interleaves those copies with the register saves.
// The SDK source form is kept by bfbb:
// https://github.com/bfbbdecomp/bfbb/blob/main/src/dolphin/src/mtx/vec.c
/**
 * @note Address: 0x8016C7A4
 * @note Size: 0xD4
 */
void C_VECReflect(const Vec* src, const Vec* normal, Vec* dst)
{
	Vec oppositeIncident;
	Vec unitNormal;
	f32 cosine;

	oppositeIncident.x = -src->x;
	oppositeIncident.y = -src->y;
	oppositeIncident.z = -src->z;

	PSVECNormalize(&oppositeIncident, &oppositeIncident);
	PSVECNormalize(normal, &unitNormal);

	cosine = PSVECDotProduct(&oppositeIncident, &unitNormal);

	dst->x = 2.0f * unitNormal.x * cosine - oppositeIncident.x;
	dst->y = 2.0f * unitNormal.y * cosine - oppositeIncident.y;
	dst->z = 2.0f * unitNormal.z * cosine - oppositeIncident.z;

	PSVECNormalize(dst, dst);
}
