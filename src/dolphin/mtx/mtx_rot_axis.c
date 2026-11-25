#include "Dolphin/mtx.h"
#include "math.h"

void __PSMTXRotAxisRadInternal(Mtx m, const Vec* axis, f32 sine, f32 cosine);

void PSMTXRotAxisRad(Mtx m, const Vec* axis, f32 rad)
{
	f32 sine = sinf(rad);
	f32 cosine = cosf(rad);

	__PSMTXRotAxisRadInternal(m, axis, sine, cosine);
}
