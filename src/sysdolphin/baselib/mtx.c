#include <global.h>
#include <math.h>
#include <dolphin/mtx/mtx.h>
#include <dolphin/mtx/vec.h>
#include <sysdolphin/mtx.h>
#include <sysdolphin/identity_mtx.h>

#define HSD_ASSERT(line, cond) ((cond) ? ((void)0) : __assert("mtx.c", line, #cond))

#define EPSILON 1e-10F

#define FLT_EPSILON 1.1920929e-07F

// not in our vec.h yet
void PSVECScale(Vec* src, Vec* dst, f32 scale);

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
static inline float mtx_sqrtf(float value)
{
	volatile float y;
	if (value > 0.0f) {
		double guess = __frsqrte((double)value);
		guess        = 0.5 * guess * (3.0 - guess * guess * value);
		guess        = 0.5 * guess * (3.0 - guess * guess * value);
		guess        = 0.5 * guess * (3.0 - guess * guess * value);
		y            = (float)(value * guess);
		return y;
	}
	return value;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline float mtx_safe_recip(float value)
{
	if (value >= 0.0F)
		return 1.0F / (FLT_EPSILON + value);
	else
		return 1.0F / (value - FLT_EPSILON);
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline float calcVal(float x, float y)
{
	if (x == 0.0F) {
		if (y >= 0) {
			return 1.5707963267948966F;
		} else {
			return -1.5707963267948966F;
		}
	} else {
		return (float)atan2(y, x);
	}
}

/**
 * @note Address: 0x801CEC78
 * @note Size: 0x430
 */
BOOL HSD_MtxInverseConcat(Mtx inv, Mtx src, Mtx dest)
{
	Mtx m;
	float det;
	float temp1;
	float temp2;
	float temp3;
	float temp4;
	float temp5;
	float temp6;
	float temp7;
	float temp8;
	float temp9;
	float temp10;
	float temp11;
	float temp12;

	det = inv[0][0] * inv[1][1] * inv[2][2] + inv[0][1] * inv[1][2] * inv[2][0] + inv[0][2] * inv[1][0] * inv[2][1]
	    - inv[2][0] * inv[1][1] * inv[0][2] - inv[1][0] * inv[0][1] * inv[2][2] - inv[0][0] * inv[2][1] * inv[1][2];

	if (det == 0.0F) {
		return 0;
	} else {
		det    = 1.0f / det;
		temp1  = ((inv[1][1] * inv[2][2]) - (inv[2][1] * inv[1][2])) * det;
		temp2  = (-((inv[0][1] * inv[2][2]) - (inv[2][1] * inv[0][2]))) * det;
		temp7  = ((inv[0][1] * inv[1][2]) - (inv[1][1] * inv[0][2])) * det;
		temp3  = (-((inv[1][0] * inv[2][2]) - (inv[2][0] * inv[1][2]))) * det;
		temp4  = ((inv[0][0] * inv[2][2]) - (inv[2][0] * inv[0][2])) * det;
		temp8  = (-((inv[0][0] * inv[1][2]) - (inv[1][0] * inv[0][2]))) * det;
		temp5  = ((inv[1][0] * inv[2][1]) - (inv[2][0] * inv[1][1])) * det;
		temp6  = (-((inv[0][0] * inv[2][1]) - (inv[2][0] * inv[0][1]))) * det;
		temp9  = ((inv[0][0] * inv[1][1]) - (inv[1][0] * inv[0][1])) * det;
		temp10 = -temp1 * inv[0][3] - temp2 * inv[1][3] - temp7 * inv[2][3];
		temp11 = -temp3 * inv[0][3] - temp4 * inv[1][3] - temp8 * inv[2][3];
		temp12 = -temp5 * inv[0][3] - temp6 * inv[1][3] - temp9 * inv[2][3];

		if (inv == dest || src == dest) {
			m[0][0] = temp1 * src[0][0] + temp2 * src[1][0] + temp7 * src[2][0];
			m[0][1] = temp1 * src[0][1] + temp2 * src[1][1] + temp7 * src[2][1];
			m[0][2] = temp1 * src[0][2] + temp2 * src[1][2] + temp7 * src[2][2];
			m[0][3] = temp1 * src[0][3] + temp2 * src[1][3] + temp7 * src[2][3] + temp10;
			m[1][0] = temp3 * src[0][0] + temp4 * src[1][0] + temp8 * src[2][0];
			m[1][1] = temp3 * src[0][1] + temp4 * src[1][1] + temp8 * src[2][1];
			m[1][2] = temp3 * src[0][2] + temp4 * src[1][2] + temp8 * src[2][2];
			m[1][3] = temp3 * src[0][3] + temp4 * src[1][3] + temp8 * src[2][3] + temp11;
			m[2][0] = temp5 * src[0][0] + temp6 * src[1][0] + temp9 * src[2][0];
			m[2][1] = temp5 * src[0][1] + temp6 * src[1][1] + temp9 * src[2][1];
			m[2][2] = temp5 * src[0][2] + temp6 * src[1][2] + temp9 * src[2][2];
			m[2][3] = temp5 * src[0][3] + temp6 * src[1][3] + temp9 * src[2][3] + temp12;

			PSMTXCopy(m, dest);
		} else {
			dest[0][0] = temp1 * src[0][0] + temp2 * src[1][0] + temp7 * src[2][0];
			dest[0][1] = temp1 * src[0][1] + temp2 * src[1][1] + temp7 * src[2][1];
			dest[0][2] = temp1 * src[0][2] + temp2 * src[1][2] + temp7 * src[2][2];
			dest[0][3] = temp1 * src[0][3] + temp2 * src[1][3] + temp7 * src[2][3] + temp10;
			dest[1][0] = temp3 * src[0][0] + temp4 * src[1][0] + temp8 * src[2][0];
			dest[1][1] = temp3 * src[0][1] + temp4 * src[1][1] + temp8 * src[2][1];
			dest[1][2] = temp3 * src[0][2] + temp4 * src[1][2] + temp8 * src[2][2];
			dest[1][3] = temp3 * src[0][3] + temp4 * src[1][3] + temp8 * src[2][3] + temp11;
			dest[2][0] = temp5 * src[0][0] + temp6 * src[1][0] + temp9 * src[2][0];
			dest[2][1] = temp5 * src[0][1] + temp6 * src[1][1] + temp9 * src[2][1];
			dest[2][2] = temp5 * src[0][2] + temp6 * src[1][2] + temp9 * src[2][2];
			dest[2][3] = temp5 * src[0][3] + temp6 * src[1][3] + temp9 * src[2][3] + temp12;
		}
	}
	return 1;
}

/**
 * @note Address: 0x801CF0A8
 * @note Size: 0x320
 */
void HSD_MtxGetRotation(Mtx m, Vec* vec)
{
	float length0;
	float length1;
	float length2;
	float testVal_1;
	float val_01;

	length0 = mtx_sqrtf(m[0][0] * m[0][0] + m[1][0] * m[1][0] + m[2][0] * m[2][0]);
	if (!(length0 < 1.17549435e-38F)) {
		length1 = mtx_sqrtf(m[0][1] * m[0][1] + m[1][1] * m[1][1] + m[2][1] * m[2][1]);
		if (!(length1 < 1.17549435e-38F)) {
			length2 = mtx_sqrtf(m[0][2] * m[0][2] + m[1][2] * m[1][2] + m[2][2] * m[2][2]);
			if (!(length2 < 1.17549435e-38F)) {
				testVal_1 = -m[2][0];
				testVal_1 /= length0;

				if (testVal_1 >= 1.0f) {
					val_01 = 1.5707963267948966F;
				} else if (testVal_1 <= -1) {
					val_01 = -1.5707963267948966F;
				} else {
					val_01 = (float)asin(testVal_1);
				}

				vec->y = val_01;

				if ((float)cos(vec->y) >= 1.17549435e-38F) {
					float testVal_2_pre = m[2][2] / length2;
					float testVal_3_pre = m[2][1] / length1;

					vec->x = calcVal(testVal_2_pre, testVal_3_pre);
					vec->z = calcVal(m[0][0], m[1][0]);
					return;
				}

				vec->x = calcVal(m[1][1], m[0][1]);
				vec->z = 0;
				return;
			}
		}
	}

	vec->x = 0;
	vec->y = 0;
	vec->z = 0;
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
		inverse_length = mtx_sqrtf(1.0F / square);
		out->x         = 1.0F / inverse_length;
		PSVECScale(&x, &x, inverse_length);
		y.x = m[0][1];
		y.y = m[1][1];
		y.z = m[2][1];
		PSVECScale(&x, &projection, PSVECDotProduct(&x, &y));
		PSVECSubtract(&y, &projection, &y);
		square = PSVECSquareMag(&y);
		if (square > EPSILON) {
			inverse_length = mtx_sqrtf(1.0F / square);
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
				out->z = mtx_sqrtf(square);
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
				out->z = mtx_sqrtf(square);
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
			inverse_length = mtx_sqrtf(1.0F / square);
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
void HSD_MkRotationMtx(Mtx arg0, Vec* arg1)
{
	float sinX;
	float cosX;
	float sinY;
	float cosY;
	float sinZ;
	float cosZ;
	float temp1;
	float temp2;

	sinX = (float)sin(arg1->x);
	cosX = (float)cos(arg1->x);
	sinY = (float)sin(arg1->y);
	cosY = (float)cos(arg1->y);
	sinZ = (float)sin(arg1->z);
	cosZ = (float)cos(arg1->z);

	temp1      = sinX * sinY;
	arg0[0][0] = cosY * cosZ;
	arg0[1][0] = cosY * sinZ;
	arg0[2][0] = -sinY;
	temp2      = cosX * sinY;
	arg0[0][1] = (cosZ * temp1) - (cosX * sinZ);
	arg0[1][1] = (sinZ * temp1) + (cosX * cosZ);
	arg0[2][1] = sinX * cosY;
	arg0[0][2] = (cosZ * temp2) + (sinX * sinZ);
	arg0[1][2] = (sinZ * temp2) - (sinX * cosZ);
	arg0[2][2] = cosX * cosY;
	arg0[0][3] = 0;
	arg0[1][3] = 0;
	arg0[2][3] = 0;
}

/**
 * @note Address: 0x801CFD30
 * @note Size: 0x2D8
 */
void HSD_MtxSRT(Mtx m, Vec* vec1, Vec* vec2, Vec* vec3, Vec* vec4)
{
	// NONMATCHING: register allocation only; retail keeps cosZ in f1 and vec4->x in f3
	float sinX, cosX, sinY, cosY, sinZ, cosZ;
	float vec1x_2;
	float vec1y_2;
	float vec1z_2;
	float vec1x_1;
	float vec1y_1;
	float vec1z_1;
	float vec1x;
	float vec1y;
	float vec1z;

	sinX = (float)sin(vec2->x);
	cosX = (float)cos(vec2->x);
	sinY = (float)sin(vec2->y);
	cosY = (float)cos(vec2->y);
	sinZ = (float)sin(vec2->z);
	cosZ = (float)cos(vec2->z);

	vec1x_2 = vec1x_1 = vec1x = vec1->x;
	vec1y_2 = vec1y_1 = vec1y = vec1->y;
	vec1z_2 = vec1z_1 = vec1z = vec1->z;

	if (vec4 != 0) {
		float temp1 = (vec4->x >= 0.0F ? 1.0F / (FLT_EPSILON + vec4->x) : 1.0F / (vec4->x - FLT_EPSILON));
		float temp2 = (vec4->y >= 0.0F ? 1.0F / (FLT_EPSILON + vec4->y) : 1.0F / (vec4->y - FLT_EPSILON));
		float temp3 = (vec4->z >= 0.0F ? 1.0F / (FLT_EPSILON + vec4->z) : 1.0F / (vec4->z - FLT_EPSILON));

		vec1y_2 *= vec4->y * temp1;
		vec1z_2 *= vec4->z * temp1;
		vec1x_1 *= vec4->x * temp2;
		vec1z_1 *= vec4->z * temp2;
		vec1x *= vec4->x * temp3;
		vec1y *= vec4->y * temp3;
	}

	m[0][0] = cosZ * (vec1x_2 * cosY);
	m[1][0] = sinZ * (vec1x_1 * cosY);
	m[2][0] = -vec1x * sinY;
	m[0][1] = vec1y_2 * ((cosZ * (sinX * sinY)) - (cosX * sinZ));
	m[1][1] = vec1y_1 * ((sinZ * (sinX * sinY)) + (cosX * cosZ));
	m[2][1] = cosY * (vec1y * sinX);
	m[0][2] = vec1z_2 * ((cosZ * (cosX * sinY)) + (sinX * sinZ));
	m[1][2] = vec1z_1 * ((sinZ * (cosX * sinY)) - (sinX * cosZ));
	m[2][2] = cosY * (vec1z * cosX);
	m[0][3] = vec3->x;
	m[1][3] = vec3->y;
	m[2][3] = vec3->z;
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
void HSD_MtxScaledAdd(Mtx arg0, Mtx arg1, Mtx arg2, float arg3)
{
	float* arr0 = (&arg0[0][0]);
	float* arr1 = (&arg1[0][0]);
	float* arr2 = (&arg2[0][0]);

	*arr2++ = *arr1++ + (arg3 * *arr0++);
	*arr2++ = *arr1++ + (arg3 * *arr0++);
	*arr2++ = *arr1++ + (arg3 * *arr0++);
	*arr2++ = *arr1++ + (arg3 * *arr0++);

	*arr2++ = *arr1++ + (arg3 * *arr0++);
	*arr2++ = *arr1++ + (arg3 * *arr0++);
	*arr2++ = *arr1++ + (arg3 * *arr0++);
	*arr2++ = *arr1++ + (arg3 * *arr0++);

	*arr2++ = *arr1++ + (arg3 * *arr0++);
	*arr2++ = *arr1++ + (arg3 * *arr0++);
	*arr2++ = *arr1++ + (arg3 * *arr0++);
	*arr2++ = *arr1++ + (arg3 * *arr0++);
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
void HSD_VecFree(Vec* arg0)
{
	if (arg0 != 0) {
		HSD_ObjFree(&vec_alloc_data, arg0);
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
void HSD_MtxFree(void* arg0)
{
	if (arg0 != 0) {
		HSD_ObjFree(&mtx_alloc_data, arg0);
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
