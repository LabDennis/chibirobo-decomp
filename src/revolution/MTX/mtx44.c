#include "revolution/mtx.h"
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
	cot     = 1.0f / (f32)tan(angle);
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
void PSMTX44Identity(register Mtx44 m)
{
	register f32 one;
	register f32 zero;
	zero = 0.0f;
	one  = 1.0f;
#ifdef __MWERKS__ // clang-format off
	asm {
		psq_st zero, 0x4(m), 0, 0
		stfs one, 0x0(m)
		psq_st zero, 0xc(m), 0, 0
		stfs one, 0x14(m)
		psq_st zero, 0x18(m), 0, 0
		psq_st zero, 0x20(m), 0, 0
		stfs one, 0x28(m)
		psq_st zero, 0x2c(m), 0, 0
		psq_st zero, 0x34(m), 0, 0
		stfs one, 0x3c(m)
	}
#endif // clang-format on
}

/**
 * @note Wii JP address: 0x8018F638
 * @note Size: 0x44
 */
ASM void PSMTX44Copy(register Mtx44 src, register Mtx44 dest)
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
ASM void PSMTX44Concat(register Mtx44 a, register Mtx44 b, register Mtx44 out)
{
#ifdef __MWERKS__ // clang-format off
	nofralloc;
	psq_l fp0, 0x0(a), 0, 0;
	psq_l fp2, 0x0(b), 0, 0;
	ps_muls0 fp6, fp2, fp0;
	psq_l fp3, 0x10(b), 0, 0;
	psq_l fp4, 0x20(b), 0, 0;
	ps_madds1 fp6, fp3, fp0, fp6;
	psq_l fp1, 0x8(a), 0, 0;
	psq_l fp5, 0x30(b), 0, 0;
	ps_madds0 fp6, fp4, fp1, fp6;
	psq_l fp0, 0x10(a), 0, 0;
	ps_madds1 fp6, fp5, fp1, fp6;
	psq_l fp1, 0x18(a), 0, 0;
	ps_muls0 fp8, fp2, fp0;
	ps_madds1 fp8, fp3, fp0, fp8;
	psq_l fp0, 0x20(a), 0, 0;
	ps_madds0 fp8, fp4, fp1, fp8;
	ps_madds1 fp8, fp5, fp1, fp8;
	psq_l fp1, 0x28(a), 0, 0;
	ps_muls0 fp10, fp2, fp0;
	ps_madds1 fp10, fp3, fp0, fp10;
	psq_l fp0, 0x30(a), 0, 0;
	ps_madds0 fp10, fp4, fp1, fp10;
	ps_madds1 fp10, fp5, fp1, fp10;
	psq_l fp1, 0x38(a), 0, 0;
	ps_muls0 fp12, fp2, fp0;
	psq_l fp2, 0x8(b), 0, 0;
	ps_madds1 fp12, fp3, fp0, fp12;
	psq_l fp0, 0x0(a), 0, 0;
	ps_madds0 fp12, fp4, fp1, fp12;
	psq_l fp3, 0x18(b), 0, 0;
	ps_madds1 fp12, fp5, fp1, fp12;
	psq_l fp1, 0x8(a), 0, 0;
	ps_muls0 fp7, fp2, fp0;
	psq_l fp4, 0x28(b), 0, 0;
	ps_madds1 fp7, fp3, fp0, fp7;
	psq_l fp5, 0x38(b), 0, 0;
	ps_madds0 fp7, fp4, fp1, fp7;
	psq_l fp0, 0x10(a), 0, 0;
	ps_madds1 fp7, fp5, fp1, fp7;
	psq_l fp1, 0x18(a), 0, 0;
	ps_muls0 fp9, fp2, fp0;
	psq_st fp6, 0x0(out), 0, 0;
	ps_madds1 fp9, fp3, fp0, fp9;
	psq_l fp0, 0x20(a), 0, 0;
	ps_madds0 fp9, fp4, fp1, fp9;
	psq_st fp8, 0x10(out), 0, 0;
	ps_madds1 fp9, fp5, fp1, fp9;
	psq_l fp1, 0x28(a), 0, 0;
	ps_muls0 fp11, fp2, fp0;
	psq_st fp10, 0x20(out), 0, 0;
	ps_madds1 fp11, fp3, fp0, fp11;
	psq_l fp0, 0x30(a), 0, 0;
	ps_madds0 fp11, fp4, fp1, fp11;
	psq_st fp12, 0x30(out), 0, 0;
	ps_madds1 fp11, fp5, fp1, fp11;
	psq_l fp1, 0x38(a), 0, 0;
	ps_muls0 fp13, fp2, fp0;
	psq_st fp7, 0x8(out), 0, 0;
	ps_madds1 fp13, fp3, fp0, fp13;
	psq_st fp9, 0x18(out), 0, 0;
	ps_madds0 fp13, fp4, fp1, fp13;
	psq_st fp11, 0x28(out), 0, 0;
	ps_madds1 fp13, fp5, fp1, fp13;
	psq_st fp13, 0x38(out), 0, 0;
	blr;
#endif // clang-format on
}

/**
 * @note Wii JP address: 0x8018F780
 * @note Size: 0x2C0
 */
// NONMATCHING: the inlined PSMTX44Identity is the loop version here; retail's is the psq_st and stfs stores
u32 C_MTX44Inverse(Mtx44 src, Mtx44 inverse)
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
			f32 magnitude = fabs(work[row][col]);
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
