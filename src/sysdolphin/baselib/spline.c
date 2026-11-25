#include <sysdolphin/spline.h>
#include <math.h>

#define ABS(x) ((x) < 0.0F ? -(x) : (x))

/**
 * @note Address: 0x801CE238
 * @note Size: 0x60
 */
f32 splGetHelmite(f32 fterm, f32 time, f32 p0, f32 p1, f32 d0, f32 d1)
{
	f32 _3t2_T2;
	f32 _2t3_T3;
	f32 t3_T2;
	f32 t2_T;
	f32 t2;
	f32 _1_T2;

	_1_T2   = time * time;
	t2      = fterm * fterm;
	t2_T    = _1_T2 * fterm;
	t3_T2   = t2 * (_1_T2 * time);
	_2t3_T3 = 2.0F * t3_T2 * fterm;
	_3t2_T2 = 3.0F * _1_T2 * t2;

	return (d1 * (t3_T2 - t2_T))
	     + ((d0 * (time + ((t3_T2 - t2_T) - t2_T))) + ((p0 * (1.0F + (_2t3_T3 - _3t2_T2))) + (p1 * (-_2t3_T3 + _3t2_T2))));
}

static inline void splGetCardinalPoint(Vec* p, Vec* cp, f32 tension, f32 u)
{
	f32 u2   = u * u;
	f32 u3   = u2 * u;
	f32 car0 = tension * (-u3 + 2.0F * u2 - u);
	f32 car1 = ((2.0F - tension) * u3) + ((tension - 3.0F) * u2) + 1.0F;
	f32 car2 = ((tension - 2.0F) * u3) + ((3.0F - (2.0F * tension)) * u2) + (tension * u);
	f32 car3 = tension * (u3 - u2);

	p->x = (cp[0].x * car0) + (cp[1].x * car1) + (cp[2].x * car2) + (cp[3].x * car3);
	p->y = (cp[0].y * car0) + (cp[1].y * car1) + (cp[2].y * car2) + (cp[3].y * car3);
	p->z = (cp[0].z * car0) + (cp[1].z * car1) + (cp[2].z * car2) + (cp[3].z * car3);
}

static void splGetBSplinePoint(Vec* p, Vec* cp, f32 u)
{
	f32 u2   = u * u;
	f32 u3   = u2 * u;
	f32 u_1  = 1.0F - u;
	f32 k1_6 = (1.0F / 6.0F);
	f32 b0   = k1_6 * u_1 * u_1 * u_1;
	f32 b1   = k1_6 * (4.0F + (3.0F * u3 - 6.0F * u2));
	f32 b2   = k1_6 * (3.0F * (-u3 + u2 + u) + 1.0F);
	f32 b3   = k1_6 * u3;

	p->x = (cp[0].x * b0) + (cp[1].x * b1) + (cp[2].x * b2) + (cp[3].x * b3);
	p->y = (cp[0].y * b0) + (cp[1].y * b1) + (cp[2].y * b2) + (cp[3].y * b3);
	p->z = (cp[0].z * b0) + (cp[1].z * b1) + (cp[2].z * b2) + (cp[3].z * b3);
}

static inline void splGetBezierPoint(Vec* p, Vec* cp, f32 u)
{
	f32 u_1  = 1.0F - u;
	f32 u2   = u * u;
	f32 u_12 = u_1 * u_1;
	f32 bez0 = u_12 * u_1;
	f32 bez1 = 3.0F * u * u_12;
	f32 bez2 = 3.0F * u2 * u_1;
	f32 bez3 = u2 * u;

	p->x = (cp[0].x * bez0) + (cp[1].x * bez1) + (cp[2].x * bez2) + (cp[3].x * bez3);
	p->y = (cp[0].y * bez0) + (cp[1].y * bez1) + (cp[2].y * bez2) + (cp[3].y * bez3);
	p->z = (cp[0].z * bez0) + (cp[1].z * bez1) + (cp[2].z * bez2) + (cp[3].z * bez3);
}

/**
 * @note Address: 0x801CE298
 * @note Size: 0x490
 */
void splGetSplinePoint(Vec* p, HSD_Spline* spline, f32 u)
{
	Vec* cp;
	s16 idx;
	if (u < 0.0F || u > 1.0F) {
		return;
	}

	if (u < 1.0F) {
		f32 t = (u * (spline->numcv - 1));
		idx   = t;
		t     = t - (f32)idx;
		switch (spline->type) {
		case 0:
			cp   = &spline->cv[idx];
			p->x = (t * (cp[1].x - cp[0].x)) + cp[0].x;
			p->y = (t * (cp[1].y - cp[0].y)) + cp[0].y;
			p->z = (t * (cp[1].z - cp[0].z)) + cp[0].z;
			return;
		case 1:
			cp = &spline->cv[idx * 3];
			splGetBezierPoint(p, cp, t);
			return;
		case 2:
			cp = &spline->cv[idx];
			splGetBSplinePoint(p, cp, t);
			return;
		case 3: {
			cp = &spline->cv[idx];
			splGetCardinalPoint(p, cp, spline->tension, t);
			return;
		}
		}
	} else {
		idx = spline->numcv - 1;
		switch (spline->type) {
		case 0:
			*p = spline->cv[idx];
			return;
		case 1:
			*p = spline->cv[idx * 3];
			return;
		case 2:
			cp = &spline->cv[idx] - 1;
			splGetBSplinePoint(p, cp, 1.0F);
			return;
		case 3:
			cp = &spline->cv[idx];
			*p = cp[1];
			return;
		}
	}
}

static f32 spl_ArcLengthPolySqrt(const f32 coeffs[5], f32 t)
{
	f32 t2     = t * t;
	f32 t3     = t2 * t;
	f32 t4     = t3 * t;
	f32 result = (coeffs[0] * t4) + (coeffs[1] * t3) + (coeffs[2] * t2) + (coeffs[3] * t) + coeffs[4];

	if ((result < 0.0F) && (result > -0.001F)) {
		result = 0.0F;
	}

	return dolsqrtf(result);
}

/**
 * @note Address: 0x801CE728
 * @note Size: 0x308
 */
f32 splArcLength(const f32 coeffs[5], f32 start, f32 end)
{
	f32 dx  = (end - start) / 8.0F;
	f32 t   = start + dx;
	f32 sum = 0.0F;
	s32 i;

	for (i = 2; i <= 8; i++) {
		if (!(i & 1)) {
			sum += 4.0F * spl_ArcLengthPolySqrt(coeffs, t);
		} else {
			sum += 2.0F * spl_ArcLengthPolySqrt(coeffs, t);
		}
		t += dx;
	}

	return (dx * (sum + spl_ArcLengthPolySqrt(coeffs, start) + spl_ArcLengthPolySqrt(coeffs, end))) / 3.0F;
}

/**
 * @note Address: 0x801CEA30
 * @note Size: 0x200
 */
f32 splArcLengthGetParameter(HSD_Spline* spl, f32 arg1)
{
	s32 idx   = 0;
	f32 start = 0.0F;
	f32 end   = 1.0F;
	f32 result;

	if (arg1 <= start) {
		return start;
	}

	if (arg1 >= end) {
		return end;
	}

	while (spl->segLength[idx + 1] < arg1) {
		idx++;
	}

	switch (spl->type) {
	case 0: {
		result = (arg1 - spl->segLength[idx]) / (spl->segLength[idx + 1] - spl->segLength[idx]);
	} break;
	case 1:
	case 2:
	case 3: {
		f32 var_f22 = spl->totalLength * (arg1 - spl->segLength[idx]);

		while (ABS(start - end) >= 0.00001F) {
			const f32* coeffs = spl->segPoly[idx];
			f32 simpsons;

			result   = (start + end) / 2.0F;
			simpsons = splArcLength(coeffs, start, result);
			if (var_f22 < (0.00001F + simpsons)) {
				end = result;
			} else {
				start = result;
				var_f22 -= simpsons;
			}
		}
	} break;
	}

	return (result + idx) / (spl->numcv - 1.0F);
}

/**
 * @note Address: 0x801CEC30
 * @note Size: 0x48
 */
void splArcLengthPoint(Vec* vec, HSD_Spline* spline, f32 param)
{
	splGetSplinePoint(vec, spline, splArcLengthGetParameter(spline, param));
}
