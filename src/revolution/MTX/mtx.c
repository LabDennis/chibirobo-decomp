#include "Dolphin/mtx.h"
#include "math.h"

/**
 * @note Address: 0x8016BB9C
 * @note Size: 0x70
 */
void PSMTXRotRad(Mtx m, char axis, f32 rad)
{
	f32 sinA, cosA;
	sinA = sin(rad);
	cosA = cos(rad);
	PSMTXRotTrig(m, axis, sinA, cosA);
}
