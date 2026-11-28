#include "Dolphin/mtx.h"
#include "math.h"

/**
 * @note Address: 0x8016BF4C
 * @note Size: 0x18C
 */
void C_MTXLookAt(Mtx m, const Vec* camPos, const Vec* camUp, const Vec* target)
{

	Vec vLook, vRight, vUp;

	vLook.x = camPos->x - target->x;
	vLook.y = camPos->y - target->y;
	vLook.z = camPos->z - target->z;
	PSVECNormalize(&vLook, &vLook);

	PSVECCrossProduct(camUp, &vLook, &vRight);
	PSVECNormalize(&vRight, &vRight);

	PSVECCrossProduct(&vLook, &vRight, &vUp);

	m[0][0] = vRight.x;
	m[0][1] = vRight.y;
	m[0][2] = vRight.z;
	m[0][3] = -(camPos->x * vRight.x + camPos->y * vRight.y + camPos->z * vRight.z);

	m[1][0] = vUp.x;
	m[1][1] = vUp.y;
	m[1][2] = vUp.z;
	m[1][3] = -(camPos->x * vUp.x + camPos->y * vUp.y + camPos->z * vUp.z);

	m[2][0] = vLook.x;
	m[2][1] = vLook.y;
	m[2][2] = vLook.z;
	m[2][3] = -(camPos->x * vLook.x + camPos->y * vLook.y + camPos->z * vLook.z);
}

/**
 * @note Address: 0x8016C0D8
 * @note Size: 0x94
 */
// NONMATCHING: the instruction stream matches retail. The strict report's remaining
// difference is that retail resolves 0.0f, 1.0f and 0.5f through the preceding MTX
// paired-single literal pool, while this separate scalar object carries its own copies.
void C_MTXLightFrustum(Mtx m, f32 arg1, f32 arg2, f32 arg3, f32 arg4, f32 arg5, f32 arg6, f32 arg7, f32 arg8, f32 arg9)
{
	f32 tmp = 1.0f / (arg4 - arg3);
	m[0][0] = ((2 * arg5) * tmp) * arg6;
	m[0][1] = 0.0f;
	m[0][2] = (((arg4 + arg3) * tmp) * arg6) - arg8;
	m[0][3] = 0.0f;
	tmp     = 1.0f / (arg1 - arg2);
	m[1][0] = 0.0f;
	m[1][1] = ((2 * arg5) * tmp) * arg7;
	m[1][2] = (((arg1 + arg2) * tmp) * arg7) - arg9;
	m[1][3] = 0.0f;
	m[2][0] = 0.0f;
	m[2][1] = 0.0f;
	m[2][2] = -1.0f;
	m[2][3] = 0.0f;
}

/**
 * @note Address: 0x8016C16C
 * @note Size: 0xCC
 */
// NONMATCHING: same instruction stream as retail; only the shared 0.0f/1.0f/0.5f
// literal ownership described at C_MTXLightFrustum differs in the strict report.
void C_MTXLightPerspective(Mtx m, f32 fovY, f32 aspect, f32 scaleS, f32 scaleT, f32 transS, f32 transT)
{
	f32 angle;
	f32 cot;

	angle = fovY * 0.5f;
	angle = MTXDegToRad(angle);

	cot = 1.0f / (f32)tan(angle);

	m[0][0] = (cot / aspect) * scaleS;
	m[0][1] = 0.0f;
	m[0][2] = -transS;
	m[0][3] = 0.0f;

	m[1][0] = 0.0f;
	m[1][1] = cot * scaleT;
	m[1][2] = -transT;
	m[1][3] = 0.0f;

	m[2][0] = 0.0f;
	m[2][1] = 0.0f;
	m[2][2] = -1.0f;
	m[2][3] = 0.0f;
}

/**
 * @note Address: 0x8016C238
 * @note Size: 0x88
 */
// NONMATCHING: same instruction stream as retail; only the shared 0.0f/1.0f
// literal ownership described at C_MTXLightFrustum differs in the strict report.
void C_MTXLightOrtho(Mtx m, f32 t, f32 b, f32 l, f32 r, f32 scaleS, f32 scaleT, f32 transS, f32 transT)
{
	f32 tmp;

	tmp     = 1.0f / (r - l);
	m[0][0] = (2.0f * tmp * scaleS);
	m[0][1] = 0.0f;
	m[0][2] = 0.0f;
	m[0][3] = ((-(r + l) * tmp) * scaleS) + transS;

	tmp     = 1.0f / (t - b);
	m[1][0] = 0.0f;
	m[1][1] = (2.0f * tmp) * scaleT;
	m[1][2] = 0.0f;
	m[1][3] = ((-(t + b) * tmp) * scaleT) + transT;

	m[2][0] = 0.0f;
	m[2][1] = 0.0f;
	m[2][2] = 0.0f;
	m[2][3] = 1.0f;
}
