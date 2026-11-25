#include <global.h>
#include <math.h>
#include <dolphin/mtx/mtx.h>
#include <dolphin/mtx/vec.h>
#include <sysdolphin/mtx.h>
#include <sysdolphin/identity_mtx.h>

#define HSD_ASSERT(line, cond) ((cond) ? ((void)0) : __assert("mtx.c", line, #cond))

#define EPSILON 1e-10F
#define HALF_PI 1.5707963267948966F
#define MIN_NORMAL_FLOAT 1.17549435e-38F
#define SCALE_RECIPROCAL_EPSILON 1.1920929e-07F

Mtx HSD_identityMtx = {
	{ 1.0F, 0.0F, 0.0F, 0.0F },
	{ 0.0F, 1.0F, 0.0F, 0.0F },
	{ 0.0F, 0.0F, 1.0F, 0.0F },
};

static HSD_ObjAllocData mtx_alloc_data;
static HSD_ObjAllocData vec_alloc_data;

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline float mtx_safe_recip(float value)
{
	if (value >= 0.0F)
		return 1.0F / (SCALE_RECIPROCAL_EPSILON + value);
	else
		return 1.0F / (value - SCALE_RECIPROCAL_EPSILON);
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline float mtx_atan2f(float y, float x)
{
	if (x == 0.0F) {
		if (y >= 0.0F) {
			return HALF_PI;
		}
		return -HALF_PI;
	}
	return (float)atan2(y, x);
}

static inline void mtx_concat_row(float out[4], Mtx src, float x, float y, float z, float translation)
{
	int column;

	for (column = 0; column < 3; column++) {
		out[column] = x * src[0][column] + y * src[1][column] + z * src[2][column];
	}
	out[3] = x * src[0][3] + y * src[1][3] + z * src[2][3] + translation;
}

/**
 * @note Address: 0x801CEC78
 * @note Size: 0x430
 */
BOOL HSD_MtxInverseConcat(Mtx inv, Mtx src, Mtx dest)
{
	Mtx m;
	float det;
	float inverse00, inverse01;
	float inverse10, inverse11;
	float inverse20, inverse21;
	float inverse02, inverse12, inverse22;
	float inverse03, inverse13, inverse23;

	det = inv[0][0] * inv[1][1] * inv[2][2] + inv[0][1] * inv[1][2] * inv[2][0] + inv[0][2] * inv[1][0] * inv[2][1]
	    - inv[2][0] * inv[1][1] * inv[0][2] - inv[1][0] * inv[0][1] * inv[2][2] - inv[0][0] * inv[2][1] * inv[1][2];

	if (det == 0.0F) {
		return FALSE;
	}

	det = 1.0F / det;

	inverse00 = ((inv[1][1] * inv[2][2]) - (inv[2][1] * inv[1][2])) * det;
	inverse01 = (-((inv[0][1] * inv[2][2]) - (inv[2][1] * inv[0][2]))) * det;
	inverse02 = ((inv[0][1] * inv[1][2]) - (inv[1][1] * inv[0][2])) * det;
	inverse10 = (-((inv[1][0] * inv[2][2]) - (inv[2][0] * inv[1][2]))) * det;
	inverse11 = ((inv[0][0] * inv[2][2]) - (inv[2][0] * inv[0][2])) * det;
	inverse12 = (-((inv[0][0] * inv[1][2]) - (inv[1][0] * inv[0][2]))) * det;
	inverse20 = ((inv[1][0] * inv[2][1]) - (inv[2][0] * inv[1][1])) * det;
	inverse21 = (-((inv[0][0] * inv[2][1]) - (inv[2][0] * inv[0][1]))) * det;
	inverse22 = ((inv[0][0] * inv[1][1]) - (inv[1][0] * inv[0][1])) * det;

	inverse03 = -inverse00 * inv[0][3] - inverse01 * inv[1][3] - inverse02 * inv[2][3];
	inverse13 = -inverse10 * inv[0][3] - inverse11 * inv[1][3] - inverse12 * inv[2][3];
	inverse23 = -inverse20 * inv[0][3] - inverse21 * inv[1][3] - inverse22 * inv[2][3];

	if (inv == dest || src == dest) {
		// keep the input intact while calculating an in-place result
		mtx_concat_row(m[0], src, inverse00, inverse01, inverse02, inverse03);
		mtx_concat_row(m[1], src, inverse10, inverse11, inverse12, inverse13);
		mtx_concat_row(m[2], src, inverse20, inverse21, inverse22, inverse23);
		PSMTXCopy(m, dest);
	} else {
		mtx_concat_row(dest[0], src, inverse00, inverse01, inverse02, inverse03);
		mtx_concat_row(dest[1], src, inverse10, inverse11, inverse12, inverse13);
		mtx_concat_row(dest[2], src, inverse20, inverse21, inverse22, inverse23);
	}
	return TRUE;
}

/**
 * @note Address: 0x801CF0A8
 * @note Size: 0x320
 */
void HSD_MtxGetRotation(Mtx m, Vec* vec)
{
	float x_length;
	float y_length;
	float z_length;
	float sin_y;
	float angle_y;

	x_length = dolsqrtf(m[0][0] * m[0][0] + m[1][0] * m[1][0] + m[2][0] * m[2][0]);
	if (x_length < MIN_NORMAL_FLOAT) {
		goto degenerate;
	}
	y_length = dolsqrtf(m[0][1] * m[0][1] + m[1][1] * m[1][1] + m[2][1] * m[2][1]);
	if (y_length < MIN_NORMAL_FLOAT) {
		goto degenerate;
	}
	z_length = dolsqrtf(m[0][2] * m[0][2] + m[1][2] * m[1][2] + m[2][2] * m[2][2]);
	if (z_length < MIN_NORMAL_FLOAT) {
		goto degenerate;
	}

	sin_y = -m[2][0];
	sin_y /= x_length;

	if (sin_y >= 1.0F) {
		angle_y = HALF_PI;
	} else if (sin_y <= -1.0F) {
		angle_y = -HALF_PI;
	} else {
		angle_y = (float)asin(sin_y);
	}

	vec->y = angle_y;

	if ((float)cos(vec->y) >= MIN_NORMAL_FLOAT) {
		float z_axis_z = m[2][2] / z_length;
		float y_axis_z = m[2][1] / y_length;

		vec->x = mtx_atan2f(y_axis_z, z_axis_z);
		vec->z = mtx_atan2f(m[1][0], m[0][0]);
		return;
	}

	vec->x = mtx_atan2f(m[0][1], m[1][1]);
	vec->z = 0.0F;
	return;

degenerate:
	vec->x = 0.0F;
	vec->y = 0.0F;
	vec->z = 0.0F;
}

/**
 * @note Address: 0x801CF3C8
 * @note Size: 0x338
 */
void HSD_MtxGetRotationMtx(Mtx src, Mtx dst, char primary, char secondary)
{
	Vec x, y, z;
	switch (primary) {
	case 'x':
	case 'X':
		x.x = src[0][0];
		x.y = src[1][0];
		x.z = src[2][0];
		PSVECNormalize(&x, &x);
		switch (secondary) {
		case 'z':
		case 'Z':
			z.x = src[0][2];
			z.y = src[1][2];
			z.z = src[2][2];
			PSVECNormalize(&z, &z);
			PSVECCrossProduct(&z, &x, &y);
			PSVECCrossProduct(&x, &y, &z);
			break;
		default:
			y.x = src[0][1];
			y.y = src[1][1];
			y.z = src[2][1];
			PSVECNormalize(&y, &y);
			PSVECCrossProduct(&x, &y, &z);
			PSVECCrossProduct(&z, &x, &y);
			break;
		}
		break;
	case 'y':
	case 'Y':
		y.x = src[0][1];
		y.y = src[1][1];
		y.z = src[2][1];
		PSVECNormalize(&y, &y);
		switch (secondary) {
		case 'x':
		case 'X':
			x.x = src[0][0];
			x.y = src[1][0];
			x.z = src[2][0];
			PSVECNormalize(&x, &x);
			PSVECCrossProduct(&x, &y, &z);
			PSVECCrossProduct(&y, &z, &x);
			break;
		default:
			z.x = src[0][2];
			z.y = src[1][2];
			z.z = src[2][2];
			PSVECNormalize(&z, &z);
			PSVECCrossProduct(&y, &z, &x);
			PSVECCrossProduct(&x, &y, &z);
			break;
		}
		break;
	default:
		z.x = src[0][2];
		z.y = src[1][2];
		z.z = src[2][2];
		PSVECNormalize(&z, &z);
		switch (secondary) {
		case 'y':
		case 'Y':
			y.x = src[0][1];
			y.y = src[1][1];
			y.z = src[2][1];
			PSVECNormalize(&y, &y);
			PSVECCrossProduct(&y, &z, &x);
			PSVECCrossProduct(&z, &x, &y);
			break;
		default:
			x.x = src[0][0];
			x.y = src[1][0];
			x.z = src[2][0];
			PSVECNormalize(&x, &x);
			PSVECCrossProduct(&z, &x, &y);
			PSVECCrossProduct(&y, &z, &x);
			break;
		}
		break;
	}
	dst[0][0] = x.x;
	dst[1][0] = x.y;
	dst[2][0] = x.z;
	dst[0][1] = y.x;
	dst[1][1] = y.y;
	dst[2][1] = y.z;
	dst[0][2] = z.x;
	dst[1][2] = z.y;
	dst[2][2] = z.z;
	dst[0][3] = 0.0F;
	dst[1][3] = 0.0F;
	dst[2][3] = 0.0F;
}

/**
 * @note Address: 0x801CF700
 * @note Size: 0x1C
 */
void HSD_MtxGetTranslate(Mtx mat, Vec* vec)
{
	vec->x = mat[0][3];
	vec->y = mat[1][3];
	vec->z = mat[2][3];
}

/**
 * @note Address: 0x801CF71C
 * @note Size: 0x4D8
 */
void HSD_MtxGetScale(Mtx m, Vec* out)
{
	float square;
	float inverse_length;
	double sign;
	Vec x, y, z, projection;
	x.x    = m[0][0];
	x.y    = m[1][0];
	x.z    = m[2][0];
	square = PSVECSquareMag(&x);
	if (square > EPSILON) {
		inverse_length = dolsqrtf(1.0F / square);
		out->x         = 1.0F / inverse_length;
		PSVECScale(&x, &x, inverse_length);
		y.x = m[0][1];
		y.y = m[1][1];
		y.z = m[2][1];
		PSVECScale(&x, &projection, PSVECDotProduct(&x, &y));
		PSVECSubtract(&y, &projection, &y);
		square = PSVECSquareMag(&y);
		if (square > EPSILON) {
			inverse_length = dolsqrtf(1.0F / square);
			out->y         = 1.0F / inverse_length;
			PSVECScale(&y, &y, inverse_length);
			z.x = m[0][2];
			z.y = m[1][2];
			z.z = m[2][2];
			PSVECScale(&y, &projection, PSVECDotProduct(&y, &z));
			PSVECSubtract(&z, &projection, &z);
			PSVECScale(&x, &projection, PSVECDotProduct(&x, &z));
			PSVECSubtract(&z, &projection, &z);
			square = PSVECSquareMag(&z);
			if (square > EPSILON) {
				out->z = dolsqrtf(square);
				PSVECCrossProduct(&y, &z, &projection);
				if (PSVECDotProduct(&x, &projection) < 0.0) {
					sign = -1.0;
					out->x *= sign;
					out->y *= sign;
					out->z *= sign;
				}
			} else {
				out->z = 0.0F;
			}
		} else {
			out->y = 0.0F;
			z.x    = m[0][2];
			z.y    = m[1][2];
			z.z    = m[2][2];
			PSVECScale(&x, &projection, PSVECDotProduct(&x, &z));
			PSVECSubtract(&z, &projection, &z);
			square = PSVECSquareMag(&z);
			if (square > EPSILON)
				out->z = dolsqrtf(square);
			else
				out->z = 0.0F;
		}
	} else {
		out->x = 0.0F;
		y.x    = m[0][1];
		y.y    = m[1][1];
		y.z    = m[2][1];
		square = PSVECSquareMag(&y);
		if (square > EPSILON) {
			inverse_length = dolsqrtf(1.0F / square);
			out->y         = 1.0F / inverse_length;
			PSVECScale(&y, &y, inverse_length);
			z.x = m[0][2];
			z.y = m[1][2];
			z.z = m[2][2];
			PSVECScale(&y, &projection, PSVECDotProduct(&y, &z));
			PSVECSubtract(&z, &projection, &z);
			out->z = PSVECMag(&z);
		} else {
			out->y = 0.0F;
			z.x    = m[0][2];
			z.y    = m[1][2];
			z.z    = m[2][2];
			out->z = PSVECMag(&z);
		}
	}
}

/**
 * @note Address: 0x801CFBF4
 * @note Size: 0x13C
 */
void HSD_MkRotationMtx(Mtx out, Vec* rotation)
{
	float sinX;
	float cosX;
	float sinY;
	float cosY;
	float sinZ;
	float cosZ;
	float sinXsinY;
	float cosXsinY;

	sinX = (float)sin(rotation->x);
	cosX = (float)cos(rotation->x);
	sinY = (float)sin(rotation->y);
	cosY = (float)cos(rotation->y);
	sinZ = (float)sin(rotation->z);
	cosZ = (float)cos(rotation->z);

	sinXsinY      = sinX * sinY;
	out[0][0] = cosY * cosZ;
	out[1][0] = cosY * sinZ;
	out[2][0] = -sinY;
	cosXsinY      = cosX * sinY;
	out[0][1] = (cosZ * sinXsinY) - (cosX * sinZ);
	out[1][1] = (sinZ * sinXsinY) + (cosX * cosZ);
	out[2][1] = sinX * cosY;
	out[0][2] = (cosZ * cosXsinY) + (sinX * sinZ);
	out[1][2] = (sinZ * cosXsinY) - (sinX * cosZ);
	out[2][2] = cosX * cosY;
	out[0][3] = 0;
	out[1][3] = 0;
	out[2][3] = 0;
}

/**
 * @note Address: 0x801CFD30
 * @note Size: 0x2D8
 */
void HSD_MtxSRT(Mtx m, Vec* scale, Vec* rotation, Vec* translation, Vec* parent_scale)
{
	Vec sine, cosine;
	float scale_x;
	float scale_y_on_x;
	float scale_z_on_x;
	float scale_x_on_y;
	float scale_y;
	float scale_z_on_y;
	float scale_x_on_z;
	float scale_y_on_z;
	float scale_z;

	sine.x = (float)sin(rotation->x);
	cosine.x = (float)cos(rotation->x);
	sine.y = (float)sin(rotation->y);
	cosine.y = (float)cos(rotation->y);
	sine.z = (float)sin(rotation->z);
	cosine.z = (float)cos(rotation->z);

	scale_x = scale_x_on_y = scale_x_on_z = scale->x;
	scale_y_on_x = scale_y = scale_y_on_z = scale->y;
	scale_z_on_x = scale_z_on_y = scale_z = scale->z;

	if (parent_scale != NULL) {
		float inverse_x = mtx_safe_recip(parent_scale->x);
		float inverse_y = mtx_safe_recip(parent_scale->y);
		float inverse_z = mtx_safe_recip(parent_scale->z);

		// compensate each off-diagonal term for the ratio of parent scales
		scale_y_on_x *= parent_scale->y * inverse_x;
		scale_z_on_x *= parent_scale->z * inverse_x;
		scale_x_on_y *= parent_scale->x * inverse_y;
		scale_z_on_y *= parent_scale->z * inverse_y;
		scale_x_on_z *= parent_scale->x * inverse_z;
		scale_y_on_z *= parent_scale->y * inverse_z;
	}

	m[0][0] = cosine.z * (scale_x * cosine.y);
	m[1][0] = sine.z * (scale_x_on_y * cosine.y);
	m[2][0] = -scale_x_on_z * sine.y;
	m[0][1] = scale_y_on_x * ((cosine.z * (sine.x * sine.y)) - (cosine.x * sine.z));
	m[1][1] = scale_y * ((sine.z * (sine.x * sine.y)) + (cosine.x * cosine.z));
	m[2][1] = cosine.y * (scale_y_on_z * sine.x);
	m[0][2] = scale_z_on_x * ((cosine.z * (cosine.x * sine.y)) + (sine.x * sine.z));
	m[1][2] = scale_z_on_y * ((sine.z * (cosine.x * sine.y)) - (sine.x * cosine.z));
	m[2][2] = cosine.y * (scale_z * cosine.x);
	m[0][3] = translation->x;
	m[1][3] = translation->y;
	m[2][3] = translation->z;
}

/**
 * @note Address: 0x801D0008
 * @note Size: 0x1AC
 */
void HSD_MtxSRTQuat(Mtx m, Vec* scale, Quaternion* rotation, Vec* translation, Vec* parent_scale)
{
	Mtx temp;
	PSMTXScale(m, scale->x, scale->y, scale->z);
	PSMTXQuat(temp, rotation);
	if (parent_scale != 0) {
		float inverse_x = mtx_safe_recip(parent_scale->x);
		float inverse_y = mtx_safe_recip(parent_scale->y);
		float inverse_z = mtx_safe_recip(parent_scale->z);
		temp[0][1] *= parent_scale->y * inverse_x;
		temp[0][2] *= parent_scale->y * inverse_x;
		temp[1][0] *= parent_scale->x * inverse_y;
		temp[1][2] *= parent_scale->z * inverse_y;
		temp[2][0] *= parent_scale->x * inverse_z;
		temp[2][1] *= parent_scale->y * inverse_z;
	}
	PSMTXConcat(temp, m, m);
	m[0][3] = translation->x;
	m[1][3] = translation->y;
	m[2][3] = translation->z;
}

/**
 * @note Address: 0x801D01B4
 * @note Size: 0xC4
 */
void HSD_MtxScaledAdd(Mtx scaled, Mtx addend, Mtx out, float scale)
{
	int row, column;

	for (row = 0; row < 3; row++) {
		for (column = 0; column < 4; column++) {
			out[row][column] = addend[row][column] + scale * scaled[row][column];
		}
	}
}

/**
 * @note Address: 0x801D0278
 * @note Size: 0xE0
 */
void HSD_VecGetOrthogonal(Vec* src, Vec* dst)
{
	Vec axis;
	if (src->x <= src->y) {
		if (src->x <= src->z) {
			axis.x = 1.0F;
			axis.y = 0.0F;
			axis.z = 0.0F;
			PSVECCrossProduct(&axis, src, dst);
		} else {
			axis.x = 0.0F;
			axis.y = 0.0F;
			axis.z = 1.0F;
			PSVECCrossProduct(&axis, src, dst);
		}
	} else {
		if (src->y <= src->z) {
			axis.x = 0.0F;
			axis.y = 1.0F;
			axis.z = 0.0F;
			PSVECCrossProduct(&axis, src, dst);
		} else {
			axis.x = 0.0F;
			axis.y = 0.0F;
			axis.z = 1.0F;
			PSVECCrossProduct(&axis, src, dst);
		}
	}
}

/**
 * @note Address: 0x801D0358
 * @note Size: 0x4C
 */
Vec* HSD_VecAlloc(void)
{
	void* vec = HSD_ObjAlloc(&vec_alloc_data);

	HSD_ASSERT(0x38C, vec);

	return vec;
}

/**
 * @note Address: 0x801D03A4
 * @note Size: 0x34
 */
void HSD_VecFree(Vec* vec)
{
	if (vec != NULL) {
		HSD_ObjFree(&vec_alloc_data, vec);
	}
}

/**
 * @note Address: 0x801D03D8
 * @note Size: 0x4C
 */
void* HSD_MtxAlloc(void)
{
	void* mtx;

	mtx = HSD_ObjAlloc(&mtx_alloc_data);
	HSD_ASSERT(0x3AB, mtx);
	return mtx;
}

/**
 * @note Address: 0x801D0424
 * @note Size: 0x34
 */
void HSD_MtxFree(void* mtx)
{
	if (mtx != NULL) {
		HSD_ObjFree(&mtx_alloc_data, mtx);
	}
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
HSD_ObjAllocData* HSD_VecGetAllocData(void)
{
	return &vec_alloc_data;
}

/**
 * @note Address: 0x801D0458
 * @note Size: 0x30
 */
void HSD_VecInitAllocData(void)
{
	HSD_ObjAllocInit(&vec_alloc_data, sizeof(Vec), 4);
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
HSD_ObjAllocData* HSD_MtxGetAllocData(void)
{
	return &mtx_alloc_data;
}

/**
 * @note Address: 0x801D0488
 * @note Size: 0x30
 */
void HSD_MtxInitAllocData(void)
{
	HSD_ObjAllocInit(&mtx_alloc_data, sizeof(Mtx), 4);
}
