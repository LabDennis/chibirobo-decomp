#include "types.h"
#include "Dolphin/mtx.h"

// MSL
double sqrt(double x);
double sin(double x);
double acos(double x);

// The SDK's quat.c defines these ahead of C_QUATMtx. The game never calls them, so the linker
// strips them, but their float literals are pooled first: that is why retail's .sdata2 holds
// 1.0f before 0.0f (C_QUATNormalize's 0.00001f is stripped with it).

void C_QUATAdd(const Quaternion* p, const Quaternion* q, Quaternion* r)
{
	r->x = p->x + q->x;
	r->y = p->y + q->y;
	r->z = p->z + q->z;
	r->w = p->w + q->w;
}

void C_QUATSubtract(const Quaternion* p, const Quaternion* q, Quaternion* r)
{
	r->x = p->x - q->x;
	r->y = p->y - q->y;
	r->z = p->z - q->z;
	r->w = p->w - q->w;
}

void C_QUATMultiply(const Quaternion* p, const Quaternion* q, Quaternion* pq)
{
	Quaternion* r;
	Quaternion pqTmp;

	if (p == pq || q == pq) {
		r = &pqTmp;
	} else {
		r = pq;
	}

	r->w = p->w * q->w - p->x * q->x - p->y * q->y - p->z * q->z;
	r->x = p->w * q->x + p->x * q->w + p->y * q->z - p->z * q->y;
	r->y = p->w * q->y + p->y * q->w + p->z * q->x - p->x * q->z;
	r->z = p->w * q->z + p->z * q->w + p->x * q->y - p->y * q->x;

	if (r == &pqTmp) {
		*pq = pqTmp;
	}
}

void C_QUATScale(const Quaternion* q, Quaternion* r, f32 scale)
{
	r->x = q->x * scale;
	r->y = q->y * scale;
	r->z = q->z * scale;
	r->w = q->w * scale;
}

f32 C_QUATDotProduct(const Quaternion* p, const Quaternion* q)
{
	return q->x * p->x + q->y * p->y + q->z * p->z + q->w * p->w;
}

void C_QUATNormalize(const Quaternion* src, Quaternion* unit)
{
	f32 mag;

	mag = (src->x * src->x) + (src->y * src->y) + (src->z * src->z) + (src->w * src->w);
	if (mag >= 0.00001f) {
		mag     = 1.0f / (f32)sqrt(mag);
		unit->x = src->x * mag;
		unit->y = src->y * mag;
		unit->z = src->z * mag;
		unit->w = src->w * mag;
	} else {
		unit->x = unit->y = unit->z = unit->w = 0.0f;
	}
}

void C_QUATInverse(const Quaternion* src, Quaternion* inv)
{
	f32 mag, norminv;

	mag = (src->x * src->x) + (src->y * src->y) + (src->z * src->z) + (src->w * src->w);
	if (mag == 0.0f) {
		mag = 1.0f;
	}

	norminv = 1.0f / mag;
	inv->x  = -src->x * norminv;
	inv->y  = -src->y * norminv;
	inv->z  = -src->z * norminv;
	inv->w  = src->w * norminv;
}

void C_QUATMtx(Quaternion* r, const Mtx m)
{
	f32 tr, s, scale;
	s32 i, j, k;
	s32 nxt[3] = { 1, 2, 0 };
	f32 q[3];

	tr = m[0][0] + m[1][1] + m[2][2];
	if (tr > 0.0f) {
		s     = (f32)sqrt(tr + 1.0f);
		r->w  = s * 0.5f;
		scale = 0.5f / s;
		r->x  = (m[2][1] - m[1][2]) * scale;
		r->y  = (m[0][2] - m[2][0]) * scale;
		r->z  = (m[1][0] - m[0][1]) * scale;
	} else {
		i = 0;
		if (m[1][1] > m[0][0]) {
			i = 1;
		}
		if (m[2][2] > m[i][i]) {
			i = 2;
		}
		j = nxt[i];
		k = nxt[j];

		s    = (f32)sqrt((m[i][i] - (m[j][j] + m[k][k])) + 1.0f);
		q[i] = s * 0.5f;
		if (s != 0.0f) {
			s = 0.5f / s;
		}

		r->w = (m[k][j] - m[j][k]) * s;
		q[j] = (m[i][j] + m[j][i]) * s;
		q[k] = (m[i][k] + m[k][i]) * s;

		r->x = q[0];
		r->y = q[1];
		r->z = q[2];
	}
}

void C_QUATSlerp(const Quaternion* p, const Quaternion* q, Quaternion* r, f32 t)
{
	f32 theta, sin_th, cos_th, tp, tq;

	cos_th = p->x * q->x + p->y * q->y + p->z * q->z + p->w * q->w;
	tq     = 1.0f;

	if (cos_th < 0.0f) {
		cos_th = -cos_th;
		tq     = -tq;
	}

	if (cos_th <= 0.99999f) {
		theta  = (f32)acos(cos_th);
		sin_th = (f32)sin(theta);
		tp     = (f32)sin((1.0f - t) * theta) / sin_th;
		tq *= (f32)sin(t * theta) / sin_th;
	} else {
		tp = 1.0f - t;
		tq = tq * t;
	}

	r->x = tp * p->x + tq * q->x;
	r->y = tp * p->y + tq * q->y;
	r->z = tp * p->z + tq * q->z;
	r->w = tp * p->w + tq * q->w;
}
