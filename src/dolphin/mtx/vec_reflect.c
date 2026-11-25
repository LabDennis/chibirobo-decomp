#include "Dolphin/vec.h"

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
