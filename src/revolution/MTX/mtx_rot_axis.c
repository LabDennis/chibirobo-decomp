#include "Dolphin/mtx.h"
#include "math.h"

void __PSMTXRotAxisRadInternal(Mtx m, const Vec* axis, f32 sine, f32 cosine);

/**
 * @note Address: 0x8016BD6C
 * @note Size: 0x70
 */
void PSMTXRotAxisRad(Mtx m, const Vec* axis, f32 rad)
{
	f32 sinT, cosT;

	sinT = sin(rad);
	cosT = cos(rad);

	__PSMTXRotAxisRadInternal(m, axis, sinT, cosT);
}
