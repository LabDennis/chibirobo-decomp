#include "Dolphin/mtx.h"
#include "math.h"

/**
 * @note Address: 0x8016C3F4
 * @note Size: 0x9C
 */
void C_MTXFrustum(Mtx44 m, f32 top, f32 bottom, f32 left, f32 right, f32 near, f32 far)
{
	f32 tmp = 1.0f / (right - left);
	m[0][0] = (2 * near) * tmp;
	m[0][1] = 0.0f;
	m[0][2] = (right + left) * tmp;
	m[0][3] = 0.0f;
	tmp     = 1.0f / (top - bottom);
	m[1][0] = 0.0f;
	m[1][1] = (2 * near) * tmp;
	m[1][2] = (top + bottom) * tmp;
	m[1][3] = 0.0f;
	m[2][0] = 0.0f;
	m[2][1] = 0.0f;
	tmp     = 1.0f / (far - near);
	m[2][2] = -near * tmp;
	m[2][3] = -(far * near) * tmp;
	m[3][0] = 0.0f;
	m[3][1] = 0.0f;
	m[3][2] = -1.0f;
	m[3][3] = 0.0f;
}

/**
 * @note Address: 0x8016C490
 * @note Size: 0xD0
 */
void C_MTXPerspective(Mtx44 m, f32 fovY, f32 aspect, f32 n, f32 f)
{
	f32 angle = fovY * 0.5f;
	f32 cot;
	f32 tmp;
	angle   = MTXDegToRad(angle);
	cot     = 1.0f / tanf(angle);
	m[0][0] = cot / aspect;
	m[0][1] = 0.0f;
	m[0][2] = 0.0f;
	m[0][3] = 0.0f;
	m[1][0] = 0.0f;
	m[1][1] = cot;
	m[1][2] = 0.0f;
	m[1][3] = 0.0f;
	m[2][0] = 0.0f;
	m[2][1] = 0.0f;
	tmp     = 1.0f / (f - n);
	m[2][2] = -(n)*tmp;
	m[2][3] = -(f * n) * tmp;
	m[3][0] = 0.0f;
	m[3][1] = 0.0f;
	m[3][2] = -1.0f;
	m[3][3] = 0.0f;
}

/**
 * @note Address: 0x8016C560
 * @note Size: 0x98
 */
void C_MTXOrtho(Mtx44 m, f32 t, f32 b, f32 l, f32 r, f32 n, f32 f)
{
	f32 tmp = 1.0f / (r - l);
	m[0][0] = 2.0f * tmp;
	m[0][1] = 0.0f;
	m[0][2] = 0.0f;
	m[0][3] = -(r + l) * tmp;
	tmp     = 1.0f / (t - b);
	m[1][0] = 0.0f;
	m[1][1] = 2.0f * tmp;
	m[1][2] = 0.0f;
	m[1][3] = -(t + b) * tmp;
	m[2][0] = 0.0f;
	m[2][1] = 0.0f;
	tmp     = 1.0f / (f - n);
	m[2][2] = -(1.0f) * tmp;
	m[2][3] = -(f)*tmp;
	m[3][0] = 0.0f;
	m[3][1] = 0.0f;
	m[3][2] = 0.0f;
	m[3][3] = 1.0f;
}

static inline f32 PairedStoreValue(f32 value)
{
	union {
		f32 value;
		u32 bits;
	} word;

	// Unquantized paired-single stores flush subnormals to signed zero.
	word.value = value;
	if ((word.bits & 0x7f800000u) == 0) {
		word.bits &= 0x80000000u;
	}
	return word.value;
}

/**
 * @note Address: N/A
 * @note Size: 0x44
 */
void PSMTX44Copy(const Mtx44 src, Mtx44 dst)
{
	u32 row;
	u32 column;

	for (row = 0; row < 4; row++) {
		for (column = 0; column < 4; column += 2) {
			f32 first = PairedStoreValue(src[row][column]);
			f32 second = PairedStoreValue(src[row][column + 1]);

			dst[row][column] = first;
			dst[row][column + 1] = second;
		}
	}
}
