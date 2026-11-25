#include "Dolphin/mtx.h"
#include "math.h"

void PSMTXRotRad(Mtx m, char axis, f32 rad)
{
	f32 sine = sinf(rad);
	f32 cosine = cosf(rad);

	PSMTXRotTrig(m, axis, sine, cosine);
}

void C_MTXIdentity(Mtx m)
{
	PSMTXIdentity(m);
}

void PSMTXTransApply(const Mtx src, Mtx dst, f32 x, f32 y, f32 z)
{
	PSMTXCopy(src, dst);
	dst[0][3] += x;
	dst[1][3] += y;
	dst[2][3] += z;
}

void PSMTXScaleApply(const Mtx src, Mtx dst, f32 x, f32 y, f32 z)
{
	int column;

	for (column = 0; column < 4; ++column) {
		dst[0][column] = x * src[0][column];
		dst[1][column] = y * src[1][column];
		dst[2][column] = z * src[2][column];
	}
}
