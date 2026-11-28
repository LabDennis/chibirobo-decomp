#include "revolution/vec.h"

/**
 * @note Address: 0x8018FC78
 * @note Size: 0xD8
 */
void C_VECHalfAngle(const Vec* a, const Vec* b, Vec* half)
{
	Vec aTmp;
	Vec bTmp;
	Vec hTmp;

	aTmp.x = -a->x;
	aTmp.y = -a->y;
	aTmp.z = -a->z;

	bTmp.x = -b->x;
	bTmp.y = -b->y;
	bTmp.z = -b->z;

	PSVECNormalize(&aTmp, &aTmp);
	PSVECNormalize(&bTmp, &bTmp);
	PSVECAdd(&aTmp, &bTmp, &hTmp);

	if (PSVECDotProduct(&hTmp, &hTmp) > 0.0f) {
		PSVECNormalize(&hTmp, half);
	} else {
		*half = hTmp;
	}
}

/**
 * @note Address: 0x8018FD50
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
