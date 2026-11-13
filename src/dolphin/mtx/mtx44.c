#include "Dolphin/mtx.h"
#include "math.h"

/**
 * @note Address: 0x8016C3F4
 * @note Size: 0x9C
 */
void C_MTXFrustum(Mtx44 m, f32 top, f32 bottom, f32 left, f32 right, f32 nearZ, f32 farZ)
{
	f32 tmp = 1.0f / (right - left);
	m[0][0] = (2 * nearZ) * tmp;
	m[0][1] = 0.0f;
	m[0][2] = (right + left) * tmp;
	m[0][3] = 0.0f;
	tmp     = 1.0f / (top - bottom);
	m[1][0] = 0.0f;
	m[1][1] = (2 * nearZ) * tmp;
	m[1][2] = (top + bottom) * tmp;
	m[1][3] = 0.0f;
	m[2][0] = 0.0f;
	m[2][1] = 0.0f;
	tmp     = 1.0f / (farZ - nearZ);
	m[2][2] = -nearZ * tmp;
	m[2][3] = -(farZ * nearZ) * tmp;
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
#if defined(VERSION_R24J01)
	cot     = 1.0f / (f32)tan(angle);
#else
	cot     = 1.0f / tanf(angle);
#endif
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

/**
 * @note Wii JP address: 0x8018F604
 * @note Size: 0x34
 */
void PSMTX44Identity(Mtx44 m)
{
	int row, col;
	for (row = 0; row < 4; ++row) {
		for (col = 0; col < 4; ++col) {
			m[row][col] = row == col ? 1.0f : 0.0f;
		}
	}
}

/**
 * @note Wii JP address: 0x8018F638
 * @note Size: 0x44
 */
ASM void PSMTX44Copy(const register Mtx44 src, register Mtx44 dest)
{
#ifdef __MWERKS__ // clang-format off
	nofralloc;
	psq_l fp1, 0(src), 0, 0;
	psq_st fp1, 0(dest), 0, 0;
	psq_l fp1, 8(src), 0, 0;
	psq_st fp1, 8(dest), 0, 0;
	psq_l fp1, 0x10(src), 0, 0;
	psq_st fp1, 0x10(dest), 0, 0;
	psq_l fp1, 0x18(src), 0, 0;
	psq_st fp1, 0x18(dest), 0, 0;
	psq_l fp1, 0x20(src), 0, 0;
	psq_st fp1, 0x20(dest), 0, 0;
	psq_l fp1, 0x28(src), 0, 0;
	psq_st fp1, 0x28(dest), 0, 0;
	psq_l fp1, 0x30(src), 0, 0;
	psq_st fp1, 0x30(dest), 0, 0;
	psq_l fp1, 0x38(src), 0, 0;
	psq_st fp1, 0x38(dest), 0, 0;
	blr;
#endif // clang-format on
}

/**
 * @note Wii JP address: 0x8018F67C
 * @note Size: 0x104
 */
void PSMTX44Concat(const Mtx44 a, const Mtx44 b, Mtx44 out)
{
	// Whole matrices can alias. Retail stores some columns earlier for partially overlapping buffers.
	Mtx44 product;
	int row, col, k;
	for (row = 0; row < 4; ++row) {
		for (col = 0; col < 4; ++col) {
			f32 sum = b[0][col] * a[row][0];
			for (k = 1; k < 4; ++k) {
				sum = __fmadds(b[k][col], a[row][k], sum);
			}
			product[row][col] = sum;
		}
	}
	PSMTX44Copy(product, out);
}

/**
 * @note Wii JP address: 0x8018F780
 * @note Size: 0x2C0
 */
u32 C_MTX44Inverse(const Mtx44 src, Mtx44 inverse)
{
	Mtx44 work;
	int col, row, k;
	PSMTX44Copy(src, work);
	PSMTX44Identity(inverse);

	for (col = 0; col < 4; ++col) {
		int pivot = col;
		f32 largest = 0.0f;
		f32 scale;
		for (row = col; row < 4; ++row) {
			f32 magnitude = __fabsf(work[row][col]);
			if (magnitude > largest) {
				largest = magnitude;
				pivot = row;
			}
		}
		if (largest == 0.0f) {
			return 0;
		}
		if (pivot != col) {
			for (k = 0; k < 4; ++k) {
				f32 tmp = work[col][k];
				work[col][k] = work[pivot][k];
				work[pivot][k] = tmp;
				tmp = inverse[col][k];
				inverse[col][k] = inverse[pivot][k];
				inverse[pivot][k] = tmp;
			}
		}
		scale = 1.0f / work[col][col];
		for (k = 0; k < 4; ++k) {
			work[col][k] *= scale;
			inverse[col][k] *= scale;
		}
		for (row = 0; row < 4; ++row) {
			if (row != col) {
				f32 factor = work[row][col];
				for (k = 0; k < 4; ++k) {
					work[row][k] -= factor * work[col][k];
					inverse[row][k] -= factor * inverse[col][k];
				}
			}
		}
	}
	return 1;
}
