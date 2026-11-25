#include <math.h>
#include <dolphin/mtx/mtxtypes.h>

#define M_PI_2 1.5707963267948966

/**
 * @note Address: N/A
 * @note Size: 0x144
 */
s32 QuatSetUp(Vec* axis, Quaternion* q, f32 angle)
{
	f32 len;
	f32 half_angle;
	f32 inv_len;
	f32 s;

	len = sqrt(axis->x * axis->x + axis->y * axis->y + axis->z * axis->z);
	if (__fabsf(len) < 1.1754944E-38F) {
		return 0;
	}
	inv_len    = 1.0F / len;
	half_angle = 0.5F * angle;
	q->w       = (f32)cos(half_angle);
	s          = (f32)sin(half_angle);
	q->x       = s * (inv_len * axis->x);
	q->y       = s * (inv_len * axis->y);
	q->z       = s * (inv_len * axis->z);

	return 1;
}

/**
 * @note Address: 0x801D3E04
 * @note Size: 0x148
 */
s32 EulerToQuat(Vec* euler, Quaternion* q)
{
	f32 cx;
	f32 cy;
	f32 cz;
	f32 sx;
	f32 sy;
	f32 sz;
	f32 cc;
	f32 ss;

	cx = (f32)cos((f32)(0.5F * euler->x));
	cy = (f32)cos((f32)(0.5F * euler->y));
	cz = (f32)cos((f32)(0.5F * euler->z));
	sx = (f32)sin((f32)(0.5F * euler->x));
	sy = (f32)sin((f32)(0.5F * euler->y));
	sz = (f32)sin((f32)(0.5F * euler->z));

	ss   = sy * sz;
	cc   = cy * cz;
	q->w = cx * cc + sx * ss;
	q->x = sx * cc - cx * ss;
	q->y = cz * (cx * sy) + sz * (sx * cy);
	q->z = sz * (cx * cy) - cz * (sx * sy);

	return 1;
}

/**
 * @note Address: 0x801D3F4C
 * @note Size: 0x2AC
 */
s32 QuatSlerp(Quaternion* p, Quaternion* q, Quaternion* out, f32 t)
{
	f32 cosom;
	f32 t2;
	f32 theta;
	f32 sinom;
	f32 sp;
	f32 sq;

	cosom = p->x * q->x + p->y * q->y + p->z * q->z + p->w * q->w;

	if ((1.0F + cosom) > 1e-10F) {
		if ((1.0F - cosom) > 1e-10F) {
			theta = (f32)acos(cosom);
			sinom = (f32)sin(theta);
			sp    = (f32)sin((f32)((1.0F - t) * theta)) / sinom;
			sq    = (f32)sin((f32)(t * theta)) / sinom;
		} else {
			sq = t;
			sp = (f32)(1.0 - (f64)t);
		}
		out->x = sp * p->x + sq * q->x;
		out->y = sp * p->y + sq * q->y;
		out->z = sp * p->z + sq * q->z;
		out->w = sp * p->w + sq * q->w;
	} else {
		out->x = -p->y;
		out->y = p->x;
		out->z = -p->w;
		out->w = p->z;

		if (t < 0.5F) {
			sp     = (f32)sin((f32)(M_PI_2 * (1.0F - (2.0F * t))));
			sq     = (f32)sin((f32)(M_PI_2 * (2.0F * t)));
			out->x = sp * p->x + sq * q->x;
			out->y = sp * p->y + sq * q->y;
			out->z = sp * p->z + sq * q->z;
			out->w = sp * p->w + sq * q->w;
		} else {
			t -= 0.5F;
			t2     = 2.0F * t;
			sp     = (f32)sin((f32)(M_PI_2 * (1.0F - t2)));
			sq     = (f32)sin((f32)(M_PI_2 * t2));
			out->x = sp * p->x + sq * q->x;
			out->y = sp * p->y + sq * q->y;
			out->z = sp * p->z + sq * q->z;
			out->w = sp * p->w + sq * q->w;
		}
	}

	return 1;
}
